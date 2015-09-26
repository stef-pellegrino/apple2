/*
 * Apple // emulator for *nix
 *
 * This software package is subject to the GNU General Public License
 * version 2 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * THERE ARE NO WARRANTIES WHATSOEVER.
 *
 */

#include "common.h"

// Keep these in sync with the Java side
enum {
    CRASH_JAVA=0,
    CRASH_NULL_DEREF,
    CRASH_STACKCALL_OVERFLOW,
    CRASH_STACKBUF_OVERFLOW,
    // MOAR!
};

#include <jni.h>

static volatile int __attribute__((noinline)) _crash_null_deref(void) {
    static volatile uintptr_t *ptr = NULL;
    while ((ptr+1)) {
        *ptr++ = 0xAA;
    }
    return (int)ptr[0];
}

static volatile int (*funPtr0)(void) = NULL;
static volatile int __attribute__((noinline)) _crash_stackcall_overflow(void) {
    if (funPtr0) {
        funPtr0();
        funPtr0 = NULL;
    } else {
        funPtr0 = &_crash_stackcall_overflow;
        funPtr0();
    }
    return getpid();
}

static volatile int (*funPtr1)(unsigned int) = NULL;
static volatile int __attribute__((noinline)) _crash_stackbuf_overflow0(unsigned int smashSize) {
    volatile char buf[32];
    memset((char *)buf, 0x55, smashSize);
    return (int)&buf[0];
}

static volatile int __attribute__((noinline)) _crash_stackbuf_overflow(void) {
    static volatile unsigned int smashSize = 0;
    while (1) {
        if (funPtr1) {
            funPtr1(smashSize);
            funPtr1 = NULL;
        } else {
            funPtr1 = &_crash_stackbuf_overflow0;
            funPtr1(smashSize);
        }

        smashSize += 32;
        if (!smashSize) {
            break;
        }
    }
    return getpid();
}

void Java_org_deadc0de_apple2ix_Apple2CrashHandler_nativePerformCrash(JNIEnv *env, jclass cls, jint crashType) {
#warning FIXME TODO ... we should turn off test codepaths in release build =D
    LOG("... performing crash of type : %d", crashType);

    switch (crashType) {
        case CRASH_NULL_DEREF:
            _crash_null_deref();
            break;

        case CRASH_STACKCALL_OVERFLOW:
            _crash_stackcall_overflow();
            break;

        case CRASH_STACKBUF_OVERFLOW:
            _crash_stackbuf_overflow();
            break;

        default:
            // unknown crasher, just abort ...
            abort();
            break;
    }
}

#define _JAVA_CRASH_NAME "/jcrash.txt" // this should match the Java side
#define _HALF_PAGE_SIZE (PAGE_SIZE>>1)

void Java_org_deadc0de_apple2ix_Apple2CrashHandler_nativeOnUncaughtException(JNIEnv *env, jclass cls, jstring jhome, jstring jstr) {
    RELEASE_ERRLOG("Uncaught Java Exception ...");

    // Write to /data/data/org.deadc0de.apple2ix.basic/jcrash.txt
    const char *home = (*env)->GetStringUTFChars(env, jhome, NULL);
    char *q = (char *)home;
    char buf[_HALF_PAGE_SIZE] = { 0 };
    const char *p0 = &buf[0];
    char *p = (char *)p0;
    while (*q && (p-p0 < _HALF_PAGE_SIZE-1)) {
        *p++ = *q++;
    }
    (*env)->ReleaseStringUTFChars(env, jhome, home);
    q = &_JAVA_CRASH_NAME[0];
    while (*q && (p-p0 < _HALF_PAGE_SIZE-1)) {
        *p++ = *q++;
    }

    int fd = TEMP_FAILURE_RETRY(open(buf, (O_CREAT|O_APPEND|O_WRONLY), (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)));
    if (fd == -1) {
        RELEASE_ERRLOG("OOPS, could not create/write to java crash file");
        return;
    }

    const char *str = (*env)->GetStringUTFChars(env, jstr, NULL);
    jsize len = (*env)->GetStringUTFLength(env, jstr);
    TEMP_FAILURE_RETRY(write(fd, str, len));
    (*env)->ReleaseStringUTFChars(env, jstr, str);

    TEMP_FAILURE_RETRY(fsync(fd));
    TEMP_FAILURE_RETRY(close(fd));
}

