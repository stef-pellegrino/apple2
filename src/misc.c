/*
 * Apple // emulator for Linux: Miscellaneous support
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

#include "common.h"

/* ----------------------------------
    internal apple2 variables
   ---------------------------------- */

extern uint8_t apple_iie_rom[32768];

bool emulator_shutting_down = false;
bool do_logging = true; // also controlled by NDEBUG
FILE *error_log = NULL;

int sound_volume = 2;
bool is_headless = false;
color_mode_t color_mode = COLOR;
const char *data_dir = NULL;

__attribute__((constructor(CTOR_PRIORITY_FIRST)))
static void _init_common() {
    error_log = stderr;
#if defined(CONFIG_DATADIR)
    data_dir = strdup(CONFIG_DATADIR PATH_SEPARATOR PACKAGE_NAME);
#elif defined(ANDROID)
    // data_dir is set up in JNI nativeOnCreate()
#elif !defined(__APPLE__)
#error "Specify a CONFIG_DATADIR and PACKAGE_NAME"
#endif
}

GLUE_BANK_READ(read_ram_bank,base_d000_rd)
GLUE_BANK_MAYBEWRITE(write_ram_bank,base_d000_wrt)
GLUE_BANK_READ(read_ram_lc,base_e000_rd)
GLUE_BANK_MAYBEWRITE(write_ram_lc,base_e000_wrt)

GLUE_BANK_READ(iie_read_ram_default,base_ramrd)
GLUE_BANK_WRITE(iie_write_ram_default,base_ramwrt)
GLUE_BANK_READ(iie_read_ram_text_page0,base_textrd)
GLUE_BANK_WRITE(iie_write_screen_hole_text_page0,base_textwrt)
GLUE_BANK_READ(iie_read_ram_hires_page0,base_hgrrd)
GLUE_BANK_WRITE(iie_write_screen_hole_hires_page0,base_hgrwrt)

GLUE_BANK_READ(iie_read_ram_zpage_and_stack,base_stackzp)
GLUE_BANK_WRITE(iie_write_ram_zpage_and_stack,base_stackzp)

GLUE_BANK_READ(iie_read_slot3,base_c3rom)
GLUE_BANK_MAYBEREAD(iie_read_slot4,base_c4rom)
GLUE_BANK_MAYBEREAD(iie_read_slot5,base_c5rom)
GLUE_BANK_READ(iie_read_slotx,base_cxrom)

uint32_t softswitches;

const uint8_t *base_vmem = apple_ii_64k[0];
uint8_t *base_ramrd;
uint8_t *base_ramwrt;
uint8_t *base_textrd;
uint8_t *base_textwrt;
uint8_t *base_hgrrd;
uint8_t *base_hgrwrt;

uint8_t *base_stackzp;
uint8_t *base_d000_rd;
uint8_t *base_e000_rd;
uint8_t *base_d000_wrt;
uint8_t *base_e000_wrt;

uint8_t *base_c3rom;
uint8_t *base_c4rom;
uint8_t *base_c5rom;
uint8_t *base_cxrom;

/* -------------------------------------------------------------------------
    c_debug_illegal_bcd - illegal BCD (decimal mode) computation
   ------------------------------------------------------------------------- */
GLUE_C_READ(debug_illegal_bcd)
{
    RELEASE_LOG("Illegal/undefined BCD operation encountered, debug break on c_debug_illegal_bcd to debug...");
    return 0;
}

/* -------------------------------------------------------------------------
    c_set_altchar() - set alternate character set
   ------------------------------------------------------------------------- */
void c_set_altchar()
{
    video_loadfont(0x40,0x20,mousetext_glyphs,1);
    video_loadfont(0x60,0x20,lcase_glyphs,2);

    video_redraw();
}

/* -------------------------------------------------------------------------
    c_set_primary_char() - set primary character set
   ------------------------------------------------------------------------- */
void c_set_primary_char()
{
    video_loadfont(0x40,0x40,ucase_glyphs,3);

    video_redraw();
}


/* -------------------------------------------------------------------------
    c_initialize_font():     Initialize ROM character table to primary char set
   ------------------------------------------------------------------------- */
void c_initialize_font()
{
    video_loadfont(0x00,0x40,ucase_glyphs,2);
    video_loadfont(0x40,0x40,ucase_glyphs,3);
    video_loadfont(0x80,0x40,ucase_glyphs,0);
    video_loadfont(0xC0,0x20,ucase_glyphs,0);
    video_loadfont(0xE0,0x20,lcase_glyphs,0);
    video_redraw();
}



/* -------------------------------------------------------------------------
    c_initialize_tables()
   ------------------------------------------------------------------------- */

void c_initialize_tables() {

    int i;

    /* reset everything */
    for (i = 0; i < 0x10000; i++)
    {
        cpu65_vmem_r[i] = iie_read_ram_default;
        cpu65_vmem_w[i] = iie_write_ram_default;
    }

    /* language card read/write area */
    for (i = 0xD000; i < 0xE000; i++)
    {
        {
            cpu65_vmem_w[i] =
                write_ram_bank;
            cpu65_vmem_r[i] =
                read_ram_bank;
        }
    }

    for (i = 0xE000; i < 0x10000; i++)
    {
        {
            cpu65_vmem_w[i] =
                write_ram_lc;
            cpu65_vmem_r[i] =
                read_ram_lc;
        }
    }

    /* done common initialization */

    /* initialize zero-page, //e specific */
    for (i = 0; i < 0x200; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_ram_zpage_and_stack;
        cpu65_vmem_w[i] =
            iie_write_ram_zpage_and_stack;
    }

    /* initialize first text & hires page, which are specially bank switched
     *
     * video_reset() substitutes it's own hooks for all visible write locations
     * affect the display, leaving our write-functions in place only at the
     * `screen holes', hence the name.
     */
    for (i = 0x400; i < 0x800; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_ram_text_page0;
        cpu65_vmem_w[i] =
            iie_write_screen_hole_text_page0;
    }

    for (i = 0x2000; i < 0x4000; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_ram_hires_page0;
        cpu65_vmem_w[i] =
            iie_write_screen_hole_hires_page0;
    }

    /* softswich rom */
    for (i = 0xC000; i < 0xC100; i++)
    {
        cpu65_vmem_r[i] =
            read_unmapped_softswitch;
        cpu65_vmem_w[i] =
            write_unmapped_softswitch;
    }

    /* slot rom defaults */
    for (i = 0xC100; i < 0xD000; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_ram_default;
        cpu65_vmem_w[i] =
            ram_nop;
    }

    /* keyboard data and strobe (READ) */
    for (i = 0xC000; i < 0xC010; i++)
    {
        cpu65_vmem_r[i] =
            read_keyboard;
    }

    for (i = 0xC010; i < 0xC020; i++)
    {
        cpu65_vmem_r[i] =
            cpu65_vmem_w[i] =
                read_keyboard_strobe;
    }

    /* RDBNK2 switch */
    cpu65_vmem_r[0xC011] =
        iie_check_bank;

    /* RDLCRAM switch */
    cpu65_vmem_r[0xC012] =
        iie_check_lcram;

    /* 80STORE switch */
    cpu65_vmem_w[0xC000] = iie_80store_off;
    cpu65_vmem_w[0xC001] = iie_80store_on;
    cpu65_vmem_r[0xC018] = iie_check_80store;

    /* RAMRD switch */
    cpu65_vmem_w[0xC002] = iie_ramrd_main;
    cpu65_vmem_w[0xC003] = iie_ramrd_aux;
    cpu65_vmem_r[0xC013] = iie_check_ramrd;

    /* RAMWRT switch */
    cpu65_vmem_w[0xC004] = iie_ramwrt_main;
    cpu65_vmem_w[0xC005] = iie_ramwrt_aux;
    cpu65_vmem_r[0xC014] = iie_check_ramwrt;

    /* ALTZP switch */
    cpu65_vmem_w[0xC008] = iie_altzp_main;
    cpu65_vmem_w[0xC009] = iie_altzp_aux;
    cpu65_vmem_r[0xC016] = iie_check_altzp;

    /* 80COL switch */
    cpu65_vmem_w[0xC00C] = iie_80col_off;
    cpu65_vmem_w[0xC00D] = iie_80col_on;
    cpu65_vmem_r[0xC01F] = iie_check_80col;

    /* ALTCHAR switch */
    cpu65_vmem_w[0xC00E] = iie_altchar_off;
    cpu65_vmem_w[0xC00F] = iie_altchar_on;
    cpu65_vmem_r[0xC01E] = iie_check_altchar;

    /* SLOTC3ROM switch */
    cpu65_vmem_w[0xC00A] = iie_c3rom_internal;
    cpu65_vmem_w[0xC00B] = iie_c3rom_peripheral;
    cpu65_vmem_r[0xC017] = iie_check_c3rom;

    /* SLOTCXROM switch */
    cpu65_vmem_w[0xC006] = iie_cxrom_peripheral;
    cpu65_vmem_w[0xC007] = iie_cxrom_internal;
    cpu65_vmem_r[0xC015] = iie_check_cxrom;

    /* RDVBLBAR switch */
    cpu65_vmem_r[0xC019] = iie_check_vbl;

    /* TEXT switch */
    cpu65_vmem_r[0xC050] =
        cpu65_vmem_w[0xC050] =
            read_switch_graphics;
    cpu65_vmem_r[0xC051] =
        cpu65_vmem_w[0xC051] =
            read_switch_text;

    cpu65_vmem_r[0xC01A] =
        iie_check_text;

    /* MIXED switch */
    cpu65_vmem_r[0xC052] =
        cpu65_vmem_w[0xC052] =
            read_switch_no_mixed;
    cpu65_vmem_r[0xC053] =
        cpu65_vmem_w[0xC053] =
            read_switch_mixed;

    cpu65_vmem_r[0xC01B] =
        iie_check_mixed;

    /* PAGE2 switch */
    cpu65_vmem_r[0xC054] =
        cpu65_vmem_w[0xC054] =
            iie_page2_off;

    cpu65_vmem_r[0xC01C] =
        iie_check_page2;

    /* PAGE2 switch */
    cpu65_vmem_r[0xC055] =
        cpu65_vmem_w[0xC055] =
            iie_page2_on;

    /* HIRES switch */
    cpu65_vmem_r[0xC01D] =
        iie_check_hires;
    cpu65_vmem_r[0xC056] =
        cpu65_vmem_w[0xC056] =
            iie_hires_off;
    cpu65_vmem_r[0xC057] =
        cpu65_vmem_w[0xC057] =
            iie_hires_on;

    /* game I/O switches */
    cpu65_vmem_r[0xC061] =
        cpu65_vmem_r[0xC069] =
            read_button0;
    cpu65_vmem_r[0xC062] =
        cpu65_vmem_r[0xC06A] =
            read_button1;
    cpu65_vmem_r[0xC063] =
        cpu65_vmem_r[0xC06B] =
            read_button2;
    cpu65_vmem_r[0xC064] =
        cpu65_vmem_r[0xC06C] =
            read_gc0;
    cpu65_vmem_r[0xC065] =
        cpu65_vmem_r[0xC06D] =
            read_gc1;
    cpu65_vmem_r[0xC066] =
        iie_read_gc2;
    cpu65_vmem_r[0xC067] =
        iie_read_gc3;

    for (i = 0xC070; i < 0xC080; i++)
    {
        cpu65_vmem_r[i] =
            cpu65_vmem_w[i] =
                read_gc_strobe;
    }

    /* IOUDIS switch & read_gc_strobe */
    cpu65_vmem_w[0xC07E] =
        iie_ioudis_on;
    cpu65_vmem_w[0xC07F] =
        iie_ioudis_off;                 // HACK FIXME TODO : double-check this stuff against AWin...
    cpu65_vmem_r[0xC07E] =
        iie_check_ioudis;
    cpu65_vmem_r[0xC07F] =
        iie_check_dhires;

    /* Annunciator */
    for (i = 0xC058; i <= 0xC05D; i++) {
        cpu65_vmem_w[i] = cpu65_vmem_r[i] = iie_annunciator;
    }

    /* DHIRES */
    cpu65_vmem_w[0xC05E] =
        cpu65_vmem_r[0xC05E] =
            iie_dhires_on;
    cpu65_vmem_w[0xC05F] =
        cpu65_vmem_r[0xC05F] =
            iie_dhires_off;

    /* language card softswitches */
    cpu65_vmem_r[0xC080] = cpu65_vmem_w[0xC080] =
                               cpu65_vmem_r[0xC084] = cpu65_vmem_w[0xC084] = iie_c080;
    cpu65_vmem_r[0xC081] = cpu65_vmem_w[0xC081] =
                               cpu65_vmem_r[0xC085] = cpu65_vmem_w[0xC085] = iie_c081;
    cpu65_vmem_r[0xC082] = cpu65_vmem_w[0xC082] =
                               cpu65_vmem_r[0xC086] = cpu65_vmem_w[0xC086] = lc_c082;
    cpu65_vmem_r[0xC083] = cpu65_vmem_w[0xC083] =
                               cpu65_vmem_r[0xC087] = cpu65_vmem_w[0xC087] = iie_c083;

    cpu65_vmem_r[0xC088] = cpu65_vmem_w[0xC088] =
                               cpu65_vmem_r[0xC08C] = cpu65_vmem_w[0xC08C] = iie_c088;
    cpu65_vmem_r[0xC089] = cpu65_vmem_w[0xC089] =
                               cpu65_vmem_r[0xC08D] = cpu65_vmem_w[0xC08D] = iie_c089;
    cpu65_vmem_r[0xC08A] = cpu65_vmem_w[0xC08A] =
                               cpu65_vmem_r[0xC08E] = cpu65_vmem_w[0xC08E] = lc_c08a;
    cpu65_vmem_r[0xC08B] = cpu65_vmem_w[0xC08B] =
                               cpu65_vmem_r[0xC08F] = cpu65_vmem_w[0xC08F] = iie_c08b;

    /* slot i/o area */
    for (i = 0xC100; i < 0xC300; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_slotx;             /* slots 1 & 2 (x) */
    }

    for (i = 0xC300; i < 0xC400; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_slot3;             /* slot 3 (80col) */
    }

    for (i = 0xC400; i < 0xC500; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_slot4;             /* slot 4 - MB or Phasor */
    }

    for (i = 0xC500; i < 0xC600; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_slot5;             /* slot 5 - MB #2 */
    }

    for (i = 0xC600; i < 0xC800; i++)
    {
        cpu65_vmem_r[i] =
            iie_read_slotx;             /* slots 6 - 7 (x) */
    }

    for (i = 0xC800; i < 0xD000; i++)
    {
        cpu65_vmem_r[i] = iie_read_slot_expansion;
    }
    cpu65_vmem_w[0xCFFF] = iie_read_slot_expansion;

    video_reset();

    // Peripheral card slot initializations ...

    // HACK TODO FIXME : this needs to be tied to the UI/configuration system (once we have more/conflicting options)

#ifdef AUDIO_ENABLED
    mb_io_initialize(4, 5); /* Mockingboard(s) and/or Phasor in slots 4 & 5 */
#endif
    disk_io_initialize(6); /* Put a Disk ][ Controller in slot 6 */
}

/* -------------------------------------------------------------------------
    c_initialize_apple_ii_memory()
   ------------------------------------------------------------------------- */

void c_initialize_apple_ii_memory()
{
    int i;

    for (i = 0; i < 0x10000; i++)
    {
        apple_ii_64k[0][i] = 0;
        apple_ii_64k[1][i] = 0;
    }

    // Stripe words of main memory on machine reset ...
    // NOTE: cracked version of J---- will lock up without this
    for (i = 0; i < 0xC000;) {
        apple_ii_64k[0][i++] = 0xFF;
        apple_ii_64k[0][i++] = 0xFF;
        i += 2;
    }

    for (i = 0; i < 8192; i++)
    {
        language_card[0][i] = language_card[1][i] = 0;
    }

    for (i = 0; i < 8192; i++)
    {
        language_banks[0][i] = language_banks[1][i] = 0;
    }

    /* load the rom from 0xC000, slot rom main, internal rom aux */
    for (i = 0xC000; i < 0x10000; i++)
    {
        apple_ii_64k[0][i] = apple_iie_rom[i - 0xC000];
        apple_ii_64k[1][i] = apple_iie_rom[i - 0x8000];
    }

    for (i = 0; i < 0x1000; i++)
    {
        language_banks[0][i] = apple_iie_rom[i + 0x1000];
        language_banks[1][i] = apple_iie_rom[i + 0x5000];
    }

    for (i = 0; i < 0x2000; i++)
    {
        language_card[0][i] = apple_iie_rom[i + 0x2000];
        language_card[1][i] = apple_iie_rom[i + 0x6000];
    }

    apple_ii_64k[0][0xC000] = 0x00;
    apple_ii_64k[1][0xC000] = 0x00;
}

/* -------------------------------------------------------------------------
    void c_initialize_sound_hooks()
   ------------------------------------------------------------------------- */

void c_initialize_sound_hooks()
{
#ifdef AUDIO_ENABLED
    speaker_setVolumeZeroToTen(sound_volume);
    MB_SetVolumeZeroToTen(sound_volume);
#endif
    for (int i = 0xC030; i < 0xC040; i++)
    {
        cpu65_vmem_r[i] = cpu65_vmem_w[i] =
#ifdef AUDIO_ENABLED
            (sound_volume > 0) ? speaker_toggle :
#endif
            ram_nop;
    }
}

void c_disable_sound_hooks()
{
    for (int i = 0xC030; i < 0xC040; i++)
    {
        cpu65_vmem_r[i] = ram_nop;
    }
}

/* -------------------------------------------------------------------------
    c_initialize_iie_switches
   ------------------------------------------------------------------------- */
void c_initialize_iie_switches() {

    base_stackzp = apple_ii_64k[0];
    base_d000_rd = apple_ii_64k[0];
    base_d000_wrt = language_banks[0] - 0xD000;
    base_e000_rd = apple_ii_64k[0];
    base_e000_wrt = language_card[0] - 0xE000;

    base_ramrd = apple_ii_64k[0];
    base_ramwrt = apple_ii_64k[0];
    base_textrd = apple_ii_64k[0];
    base_textwrt = apple_ii_64k[0];
    base_hgrrd = apple_ii_64k[0];
    base_hgrwrt= apple_ii_64k[0];

    base_c3rom = apple_ii_64k[1];               /* c3rom internal */
    base_c4rom = apple_ii_64k[1];               /* c4rom internal */
    base_c5rom = apple_ii_64k[1];               /* c5rom internal */
    base_cxrom = apple_ii_64k[0];               /* cxrom peripheral */
}

/* -------------------------------------------------------------------------
    void c_initialize_vm()
   ------------------------------------------------------------------------- */
void c_initialize_vm() {
    c_initialize_font();                /* font already read in */
    c_initialize_apple_ii_memory();     /* read in rom memory */
    c_initialize_tables();              /* read/write memory jump tables */
    c_initialize_sound_hooks();         /* sound system */
    c_init_6();                         /* drive ][, slot 6 */
    c_initialize_iie_switches();        /* set the //e softswitches */
    c_joystick_reset();                 /* reset joystick */
}

#if !TESTING && !defined(__APPLE__) && !defined(ANDROID)
extern void *cpu_thread(void *dummyptr);

static void _shutdown_threads(void) {
    LOG("Emulator waiting for other threads to clean up...");
#if !__linux__
#warning FIXME TODO ideally we have a more deterministic thread waiting routine ...
    sleep(2); // =P
#else
    do {
        DIR *dir = opendir("/proc/self/task");
        if (!dir) {
            ERRLOG("Cannot open /proc/self/task !");
            break;
        }

        int thread_count = 0;
        struct dirent *d = NULL;
        while ((d = readdir(dir)) != NULL) {
            if (strncmp(".", d->d_name, 2) == 0) {
                // ignore
            } else if (strncmp("..", d->d_name, 3) == 0) {
                // ignore
            } else {
                ++thread_count;
            }
        }

        closedir(dir);

        assert(thread_count >= 1 && "there must at least be one thread =P");
        if (thread_count == 1) {
            break;
        }

        static struct timespec ts = { .tv_sec=0, .tv_nsec=33333333 };
        nanosleep(&ts, NULL); // 30Hz framerate
    } while (1);
#endif
}

int main(int _argc, char **_argv) {
    argc = _argc;
    argv = _argv;

    load_settings(); // user prefs
#ifdef INTERFACE_CLASSIC
    c_keys_set_key(kF8); // show credits before emulation start
#endif

    timing_startCPU();
    video_main_loop();

    assert(emulator_shutting_down && "emulator is properly shutting down");

    LOG("Emulator waiting for CPU thread clean up...");
    if (pthread_join(cpu_thread_id, NULL)) {
        ERRLOG("OOPS: pthread_join of CPU thread ...");
    }

    _shutdown_threads();

    return 0;
}
#endif

