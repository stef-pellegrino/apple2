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

#include "testcommon.h"

#ifdef HAVE_OPENSSL
#include <openssl/sha.h>
#else
#error "these tests require OpenSSL libraries"
#endif

static bool test_do_reboot = true;

extern unsigned char joy_button0;

static void testdisplay_setup(void *arg) {
    test_common_setup();
    apple_ii_64k[0][MIXSWITCH_ADDR] = 0x00;
    apple_ii_64k[0][WATCHPOINT_ADDR] = 0x00;
    joy_button0 = 0xff; // OpenApple
    if (test_do_reboot) {
        cpu65_interrupt(ResetSig);
    }
}

static void testdisplay_teardown(void *arg) {
}

static void sha1_to_str(const uint8_t * const md, char *buf) {
    int i=0;
    for (int j=0; j<SHA_DIGEST_LENGTH; j++, i+=2) {
        sprintf(buf+i, "%02X", md[j]);
    }
    sprintf(buf+i, "%c", '\0');
}

TEST test_boot_disk() {
    char *disk = strdup("./disks/testdisplay1.dsk.gz");
    if (c_new_diskette_6(0, disk, 0)) {
        int len = strlen(disk);
        disk[len-3] = '\0';
        ASSERT(!c_new_diskette_6(0, disk, 0));
    }
    free(disk);

    BOOT_TO_DOS();

    PASS();
}

// ----------------------------------------------------------------------------
// TEXT

TEST test_40col_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("RUN TESTNORMALTEXT\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("51E5960115380C64351ED00A2ACAB0EB67970249");

    PASS();
}

TEST test_80col_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("PR#3\rRUN TESTNORMALTEXT\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("ED9CE59F41A51A5ABB1617383A411455677A78E3");

    PASS();
}

TEST test_40col_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("RUN TESTINVERSETEXT\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("20957B960C3C0DE0ABEE0058A08C0DDA24AB31D8");

    PASS();
}

TEST test_80col_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("PR#3\rRUN TESTINVERSETEXT\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("037798F4BCF740D0A7CBF7CDF5FC5D1B0B3C77A2");

    PASS();
}

// ----------------------------------------------------------------------------
// LORES
//
// 2014/04/05 NOTE : Tests may be successful but graphics display appears to be somewhat b0rken
//
// NOTE : These tests assume standard color mode (not b/w or interpolated)
//

TEST test_lores_with_80col() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rPR#3\rRUN TESTLORES\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("7B642FF04DE03142A2CE1062C28A4D92E492EDDC");

    PASS();
}

TEST test_lores_with_40col() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rHOME\rLOAD TESTLORES\rLIST\rLIST\rRUN\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("D7DC78F5718B4CF8716614E79ADABCAB919FCE5D");

    PASS();
}

TEST test_lores_40colmix_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rHOME\rLOAD TESTLORES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("9097A6AE967E4501B40C7CD7EEE115B8C478B345");

    PASS();
}

TEST test_lores_40colmix_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rINVERSE\rHOME\rLOAD TESTLORES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("5256E8B96CB04F48324B587ECCCF8A435077B5DE");

    PASS();
}

TEST test_lores_80colmix_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rPR#3\rLOAD TESTLORES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("9D5D5382B0A18A71DC135CAD51BEA2665ADB5FB2");

    PASS();
}

TEST test_lores_80colmix_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("TEXT\rINVERSE\rPR#3\rLOAD TESTLORES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    if (test_do_reboot) {
        // HACK FIXME TODO -- softswitch settings appear to be initially screwy on reboot, so we start, abort, and then start again ...
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("7936E87BE1F920AACD43268DB288746528E89959");
    } else {
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("7936E87BE1F920AACD43268DB288746528E89959");
    }

    PASS();
}

// ----------------------------------------------------------------------------
// HIRES
//
// NOTE : These tests assume standard color mode (not b/w or interpolated)
//

TEST test_hires_with_80col() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);

    test_type_input("PR#3\rRUN TESTHIRES_2\r");
    c_debugger_go();

    if (test_do_reboot) {
        // HACK FIXME TODO -- softswitch settings appear to be initially screwy on reboot, so we start, abort, and then start again ...
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("8EF89A5E0501191847E9907416309B33D4B48713");
    } else {
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("1A5DD96B7E3538C2C3625A37653E013E3998F825");
    }

    PASS();
}

TEST test_hires_with_40col() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("RUN TESTHIRES_2\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("1A5DD96B7E3538C2C3625A37653E013E3998F825");

    PASS();
}

TEST test_hires_40colmix_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("POKE 7986,127\rRUN TESTHIRES\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("37F41F74EB23F8812498F732E6DA34A0EBC4D68A");

    PASS();
}

TEST test_hires_40colmix_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("INVERSE\rPOKE 7986,127\rRUN TESTHIRES\r");
    c_debugger_go();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
    ASSERT_SHA("253D1823F5DAC0300B46B3D49C04CD59CC70076F");

    PASS();
}

TEST test_hires_80colmix_normal() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("PR#3\rLOAD TESTHIRES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    if (test_do_reboot) {
        // HACK FIXME TODO -- softswitch settings appear to be initially screwy on reboot, so we start, abort, and then start again ...
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("A8CEF48107EDD6A0E727302259BEBC34B8700C1D");
    } else {
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("032BD68899749265EB2934A76A35D7068642824B");
    }

    PASS();
}

TEST test_hires_80colmix_inverse() {
    BOOT_TO_DOS();

    ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] != TEST_FINISHED);
    test_type_input("INVERSE\rPR#3\rLOAD TESTHIRES\rLIST\rLIST\rPOKE 7986,127\rRUN\r");
    c_debugger_go();

    if (test_do_reboot) {
        // HACK FIXME TODO -- softswitch settings appear to be initially screwy on reboot, so we start, abort, and then start again ...
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("984672DD673E9EC3C2CD5CD03D1D79FD0F3D626A");
    } else {
        ASSERT(apple_ii_64k[0][WATCHPOINT_ADDR] == TEST_FINISHED);
        ASSERT_SHA("FAFBB65013DA3D5173487C3F434C36A7C04DE92E");
    }

    PASS();
}

TEST test_80col_lores() {
    BOOT_TO_DOS();

    // this graphic looks wrong compared to the manual ...
    SKIPm("mode needs to be properly implemented");
}

TEST test_80col_hires() {
    BOOT_TO_DOS();

    // double-hires graphics are in-flux ...
    SKIPm("mode needs to be properly implemented");
}

// ----------------------------------------------------------------------------
// Test Suite

extern void cpu_thread(void *dummyptr);

GREATEST_SUITE(test_suite_display) {

    GREATEST_SET_SETUP_CB(testdisplay_setup, NULL);
    GREATEST_SET_TEARDOWN_CB(testdisplay_teardown, NULL);

    srandom(time(NULL));

    test_common_init(/*cputhread*/true);

    pthread_mutex_lock(&interface_mutex);

    // TESTS --------------------------

    RUN_TESTp(test_boot_disk);

    // text modes

    RUN_TESTp(test_40col_normal);
    test_do_reboot = false;
    RUN_TESTp(test_40col_normal);
    test_do_reboot = true;

    RUN_TESTp(test_40col_normal);
    test_do_reboot = false;
    RUN_TESTp(test_40col_normal);
    test_do_reboot = true;

    RUN_TESTp(test_80col_normal);
    test_do_reboot = false;
    RUN_TESTp(test_80col_normal);
    test_do_reboot = true;

    RUN_TESTp(test_40col_inverse);
    test_do_reboot = false;
    RUN_TESTp(test_40col_inverse);
    test_do_reboot = true;

    RUN_TESTp(test_80col_inverse);
    test_do_reboot = false;
    RUN_TESTp(test_80col_inverse);
    test_do_reboot = true;

    // lores

    RUN_TEST(test_lores_with_80col);
    test_do_reboot = false;
    RUN_TEST(test_lores_with_80col);
    test_do_reboot = true;

    RUN_TEST(test_lores_with_40col);
    test_do_reboot = false;
    RUN_TEST(test_lores_with_40col);
    test_do_reboot = true;

    RUN_TEST(test_lores_40colmix_normal);
    test_do_reboot = false;
    RUN_TEST(test_lores_40colmix_normal);
    test_do_reboot = true;

    RUN_TEST(test_lores_40colmix_inverse);
    test_do_reboot = false;
    RUN_TEST(test_lores_40colmix_inverse);
    test_do_reboot = true;

    RUN_TEST(test_lores_80colmix_normal);
    test_do_reboot = false;
    RUN_TEST(test_lores_80colmix_normal);
    test_do_reboot = true;

    RUN_TEST(test_lores_80colmix_inverse);
    test_do_reboot = false;
    RUN_TEST(test_lores_80colmix_inverse);
    test_do_reboot = true;

    // hires

    // HACK FIXME TODO : there appears to be bugs with various 80col graphics modes ...

    RUN_TESTp(test_hires_with_80col);
    test_do_reboot = false;
    RUN_TESTp(test_hires_with_80col);
    test_do_reboot = true;

    RUN_TEST(test_hires_with_40col);
    test_do_reboot = false;
    RUN_TEST(test_hires_with_40col);
    test_do_reboot = true;

    RUN_TEST(test_hires_40colmix_normal);
    test_do_reboot = false;
    RUN_TEST(test_hires_40colmix_normal);
    test_do_reboot = true;

    RUN_TEST(test_hires_40colmix_inverse);
    test_do_reboot = false;
    RUN_TEST(test_hires_40colmix_inverse);
    test_do_reboot = true;

    RUN_TESTp(test_hires_80colmix_normal);
    test_do_reboot = false;
    RUN_TESTp(test_hires_80colmix_normal);
    test_do_reboot = true;

    RUN_TESTp(test_hires_80colmix_inverse);
    test_do_reboot = false;
    RUN_TESTp(test_hires_80colmix_inverse);
    test_do_reboot = true;

    // double-lo/hi

    RUN_TEST(test_80col_lores);
    test_do_reboot = false;
    RUN_TEST(test_80col_lores);
    test_do_reboot = true;

    RUN_TEST(test_80col_hires);
    test_do_reboot = false;
    RUN_TEST(test_80col_hires);
    test_do_reboot = true;

    // ...
    c_eject_6(0);
    pthread_mutex_unlock(&interface_mutex);
}

SUITE(test_suite_display);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(test_suite_display);
    GREATEST_MAIN_END();
}

