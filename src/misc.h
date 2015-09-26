/*
 * Apple // emulator for Linux: Miscellaneous defines
 *
 * Copyright 1994 Alexander Jean-Claude Bottema
 * Copyright 1995 Stephen Lee
 * Copyright 1997, 1998 Aaron Culliney
 * Copyright 1998, 1999, 2000 Michael Deutschmann
 *
 * This software package is subject to the GNU General Public License
 * version 2 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * THERE ARE NO WARRANTIES WHATSOEVER.
 *
 */

#ifndef _MISC_H_
#define _MISC_H_

// top installation directory
extern const char *data_dir;

// global ref to CLI args
extern char **argv;
extern int argc;

// start emulator (CPU, audio, and video)
void emulator_start(void);

// shutdown emulator in preparation for app exit
void emulator_shutdown(void);

//
// Crash handling ...
//

typedef struct CrashHandler_s {

    /**
     * Initialize crash handler (if available)
     */
    void (*init)(const char *dumpDir);

    /**
     * Shutdown crash handler (if available)
     */
    void (*shutdown)(void);

    /**
     * Processes a crash dump (assuming this is a non-crashing contest).
     * Returns success value. On failure, the outputFile may contain the reason processing failed
     */
    bool (*processCrash)(const char *crash, const char *symbolsPath, const FILE *outputFile);

} CrashHandler_s;

extern CrashHandler_s *crashHandler;

#endif
