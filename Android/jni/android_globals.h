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

extern unsigned long android_deviceSampleRateHz;
extern unsigned long android_monoBufferSubmitSizeSamples;
extern unsigned long android_stereoBufferSubmitSizeSamples;

// architectures

extern bool android_armArch;
extern bool android_armArchV7A;
extern bool android_arm64Arch;

extern bool android_x86;
extern bool android_x86_64;

// vector instructions availability

extern bool android_armNeonEnabled;
extern bool android_x86SSSE3Enabled;

