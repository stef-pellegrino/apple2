/* 
 * Apple // emulator for Linux: Lexer for debugger
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

/*  ASSUMPTIONS:
 *	flex version 2.5.2 or later
 *
 */

%{

/* process includes only the second time we parse this file. */
#include "common.h"

#define debugtext yytext

YY_BUFFER_STATE buffer = 0;

/*
  d{is} {lc1|lc2} {/bank/}{addr} {+}{len}
  m{em} {lc1|lc2} {/bank/}{addr} {+}{len}
  a{scii} {lc1|lc2} {/bank/}{addr} {+}{len}
  r{egs}
  <addr> {lc1|lc2} : <byteseq>
  bload <file> <addrs>
  bsave <file> /<bank>/<addrs> <len>
  (s{tep} | n{ext}) {len}
  f{inish}
  u{ntil}
  g{o} {addr}
  sea{rch} {lc1|lc2} <bytes>
  (b{reak} | w{atch}) {addr}
  (b{reak} | w{atch}) {addr}
  b{reak} op <byte>
  (c{lear} | i{gnore}) {num}
  c{lear} op <byte>
  key <byte>
  sta{tus}
  l{ang}
  dr{ive}
  vm
  fr{esh}
  (? | h{elp})
*/

%}


DEC	[0-9]
HEX	[0-9a-fA-F]
WS	[\n\r\t" "]
CHAR	[^\n\r\t" "]
BOS	^
EOS	{WS}*\0
BANK	\/0?[01]\/
ADDRS	[0-9a-fA-F]+


%%



{BOS}(me?m?|as?c?i?i?){WS}+{ADDRS}{WS}\+?+{HEX}+{EOS} {
    /* mem <addrs> <len> */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);
    dump_mem(arg1, arg2, 0, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}+{BANK}{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* mem /<bank>/<addrs> <len> */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);
    dump_mem(arg1, arg2, 0, do_ascii, arg3);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}+{ADDRS}{EOS} {
    /* mem <addrs> */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    dump_mem(arg1, 256, 0, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}+{BANK}{ADDRS}{EOS} {
    /* mem /<bank>/<addrs> */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);
    dump_mem(arg1, 256, 0, do_ascii, arg3);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}+\+{HEX}+{EOS} {
    /* mem +<len> */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (*debugtext != '+') ++debugtext;
    ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    dump_mem(cpu65_current.pc, arg1, 0, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){EOS} {
    /* dump mem from current location */
    int do_ascii = 0;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    dump_mem(cpu65_current.pc, 256, 0, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}*(lc?1|lc?2){WS}+{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* dump mem from lc<bank> <addrs> <len> */
    int do_ascii = 0;
    int lc;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;

    lc = strtol(debugtext, &debugtext, 10);
    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);

    dump_mem(arg1, arg2, lc, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}*(lc?1|lc?2){WS}+{BANK}{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* dump mem from lc<bank> /<bank>/<addrs> <len> */
    int do_ascii = 0;
    int lc;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */
    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);

    dump_mem(arg1, arg2, lc, do_ascii, arg3);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}*(lc?1|lc?2){WS}+{ADDRS}{EOS} {
    /* dump mem from lc<bank> <addrs> */
    int do_ascii = 0;
    int lc;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;

    lc = strtol(debugtext, &debugtext, 10);
    arg1 = strtol(debugtext, &debugtext, 16);

    dump_mem(arg1, 256, lc, do_ascii, -1);
    return MEM;
}

{BOS}(me?m?|as?c?i?i?){WS}*(lc?1|lc?2){WS}+{BANK}{ADDRS}{EOS} {
    /* dump mem from lc<bank> /<bank>/<addrs> */
    int do_ascii = 0;
    int lc;

    if (tolower(debugtext[0]) == 'a')
	do_ascii = 1;
    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */
    arg1 = strtol(debugtext, &debugtext, 16);

    dump_mem(arg1, 256, lc, do_ascii, arg3);
    return MEM;
}

{BOS}di?s?{WS}+{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* disassemble at <addrs> <len> */
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);

    disasm(arg1, arg2, 0, -1);
    return DIS;
}

{BOS}di?s?{WS}+{BANK}{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* disassemble at /<bank>/<addrs> <len> */
    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);

    disasm(arg1, arg2, 0, arg3);
    return DIS;
}

{BOS}di?s?{WS}+{ADDRS}{EOS} {
    /* disassemble at <addrs> */
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = 256;
    if ((arg1 < 0) || (arg1 > 65535)) arg1 = cpu65_current.pc;

    disasm(arg1, arg2, 0, -1);
    return DIS;
}

{BOS}di?s?{WS}+{BANK}{ADDRS}{EOS} {
    /* disassemble at /<bank>/<addrs> */
    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = 256;
    if ((arg1 < 0) || (arg1 > 65535)) arg1 = cpu65_current.pc;

    disasm(arg1, arg2, 0, arg3);
    return DIS;
}

{BOS}di?s?{WS}+\+{HEX}+{EOS} {
    /* disassemble current location +<len> */
    while (*debugtext != '+') ++debugtext;
    ++debugtext;

    arg1 = strtol(debugtext, &debugtext, 16);
    disasm(cpu65_current.pc, arg1, 0, -1);
    return DIS;
}

{BOS}di?s?{EOS} {
    /* disassemble current location */
    disasm(cpu65_current.pc, 256, 0, -1);
    return DIS;
}

{BOS}di?s?{WS}*(lc?1|lc?2){WS}+{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* disassemble language<bank> <addr> <len> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;

    lc = strtol(debugtext, &debugtext, 10);
    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);
    
    disasm(arg1, arg2, lc, -1);
    return DIS;
}

{BOS}di?s?{WS}*(lc?1|lc?2){WS}+{BANK}{ADDRS}{WS}+\+?{HEX}+{EOS} {
    /* disassemble language<bank> /<bank>/<addr> <len> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);
    arg2 = strtol(debugtext, &debugtext, 16);
    
    disasm(arg1, arg2, lc, arg3);
    return DIS;
}

{BOS}di?s?{WS}*(lc?1|lc?2){WS}+{ADDRS}{EOS} {
    /* disassemble language<bank> <addr> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;

    lc = strtol(debugtext, &debugtext, 10);
    arg1 = strtol(debugtext, &debugtext, 16);

    disasm(arg1, 256, lc, -1);
    return DIS;
}

{BOS}di?s?{WS}*(lc?1|lc?2){WS}+{BANK}{ADDRS}{EOS} {
    /* disassemble language<bank> /<bank>/<addr> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */

    arg1 = strtol(debugtext, &debugtext, 16);

    disasm(arg1, 256, lc, arg3);
    return DIS;
}

{BOS}re?g?s?{EOS} {
    /* show cpu state */
    show_regs();
    return REGS;
}

{BOS}{ADDRS}{WS}*\:{WS}*{HEX}+{EOS} {
    /* set memory <addr> : <hex string> */
    arg1 = strtol(debugtext, &debugtext, 16);

    while (*debugtext != ':') ++debugtext; ++debugtext;
    while (isspace(*debugtext)) ++debugtext;

    set_mem(arg1, debugtext);
    return SETMEM;
}

{BOS}{ADDRS}{WS}*(lc1|lc2)\:{WS}*{HEX}+{EOS} {
    /* set LC memory <addr> lc1|lc2 : <hex string> */
    int lc;

    arg1 = strtol(debugtext, &debugtext, 16);

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);
    ++debugtext; while (isspace(*debugtext)) ++debugtext;

    set_lc_mem(arg1, lc, debugtext);
    return SETMEM;
}

{BOS}bload{WS}+{CHAR}+{WS}+{ADDRS}{EOS} {
    /* bload <file> <addr> */
    FILE *fp = NULL;
    char *ptr = NULL;
    char name[128];
    int len = -1;

    while (!isspace(*debugtext)) ++debugtext;
    while (isspace(*debugtext)) ++debugtext;
    ptr = debugtext;
    while (!isspace(*debugtext)) ++debugtext;
    len = debugtext-ptr;

    /* filename */
    strncpy(name, ptr, len);
    name[len] = '\0';

    /* bload addr */
    while (isspace(*debugtext)) ++debugtext;
    arg1 = strtol(debugtext, (char**)NULL, 16);

    fp = fopen(name, "r");
    if (fp == NULL) {
	sprintf(second_buf[num_buffer_lines++], "problem: %s", name);
	perror(name);
	return BLOAD;
    }

    bload(fp, name, arg1);
    fclose(fp);
    return BLOAD;
}

{BOS}(st?e?p?|ne?x?t?){EOS} {
    /* step / step next instruction */
    if (*debugtext == 'n') step_next = 1;

    do_step_or_next(1);
    return STEP;
}

{BOS}(st?e?p?|ne?x?t?){WS}+{HEX}+{EOS} {
    /* step / step next <n> instructions */
    if (*debugtext == 'n') step_next = 1;

    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, (char**)NULL, 16);
    if ((arg1 < 1) || (arg1 > 255)) arg1 = 255;

    do_step_or_next(arg1);
    return STEP;
}

{BOS}fi?n?i?s?h?{EOS} {
    int step_frame = 1;
    unsigned char op;

    /* step until finished with curent stack frame */
    while ((c_mygetch(0) == -1) && !at_haltpt()) {
	op = get_current_opcode();

	if (op == 0x20) ++step_frame;	/* JSR */
	if (op == 0x60) --step_frame;	/* RTS */
	
	if (!step_frame) break;				/* finished */
	begin_cpu_step();
    }
    end_cpu_step();			/* print location */
    return FINISH;
}

{BOS}un?t?i?l?{EOS} {
    /* step until PC == next instruction.  good for finishing backward
       loops */
    unsigned char op;
    int delta=0;
    op = get_current_opcode();
	
    switch (opcodes[op].mode)
    {
    case addr_implied:
    case addr_accumulator:
	delta = 1;
        break;
    case addr_immediate:
    case addr_zeropage:
    case addr_zeropage_x:
    case addr_zeropage_y:
    case addr_indirect:
    case addr_indirect_x:
    case addr_indirect_y:
    case addr_relative:
        delta = 2;
        break;
    case addr_absolute:
    case addr_absolute_x:
    case addr_absolute_y:
    case addr_j_indirect:
    case addr_j_indirect_x:
	delta = 3;
        break; 
    }

    arg1 = cpu65_current.pc + delta;

    while ((cpu65_current.pc != arg1) && !at_haltpt() && (c_mygetch(0) == -1))
	begin_cpu_step();
    end_cpu_step();			/* print location */
    return UNTIL;
}

{BOS}go?{WS}+{HEX}+{EOS} {
    /* jump to addrs and run while remaining in debugger console */
    while (!isspace(*debugtext)) ++debugtext;

    /* DANGEROUS! */
    cpu65_current.pc = strtol(debugtext, (char**)NULL, 16);
    while (!at_haltpt() && (c_mygetch(0) == -1))
	begin_cpu_step();
    end_cpu_step();			/* print location */
    return GO;
}

{BOS}go?{EOS} {
    /* run while remaining in debugger console */
    while (!at_haltpt() && (c_mygetch(0) == -1))
	begin_cpu_step();
    end_cpu_step();			/* print location */
    return GO;
}

{BOS}wa?t?c?h?{EOS} {
    /* set watchpoint */
    set_halt(watchpoints, cpu65_current.pc);
    return WATCH;
}

{BOS}wa?t?c?h?{WS}+{HEX}+{EOS} {
    /* set watchpoint */
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, (char**)NULL, 16);
    if ((arg1 < 0) || (arg1 > 65535)) {
	sprintf(second_buf[num_buffer_lines++], "invalid address");
	return WATCH;
    }

    set_halt(watchpoints, arg1);
    return WATCH;
}


{BOS}br?e?a?k?{EOS} {
    /* set breakpoint */
    set_halt(breakpoints, cpu65_current.pc);
    return BREAK;
}

{BOS}br?e?a?k?{WS}+{HEX}+{EOS} {
    /* set breakpoint */
    while (!isspace(*debugtext)) ++debugtext;

    arg1 = strtol(debugtext, (char**)NULL, 16);
    if ((arg1 < 0) || (arg1 > 65535)) {
	sprintf(second_buf[num_buffer_lines++], "invalid address");
	return BREAK;
    }

    set_halt(breakpoints, arg1);
    return BREAK;
}

{BOS}br?e?a?k?{WS}*op{WS}+{HEX}+{EOS} {
    /* set breakpoint */
    while (!(*debugtext == 'p')) ++debugtext;
    ++debugtext;

    arg1 = strtol(debugtext, (char**)NULL, 16);
    if ((arg1 < 0) || (arg1 > 0xFF)) {
	sprintf(second_buf[num_buffer_lines++], "invalid opcode");
	return BREAK;
    }

    set_halt_opcode((unsigned char)arg1);
    return BREAK;
}

{BOS}br65c02{EOS} {
    /* set opcode breakpoints on 65c02 instructions */
    set_halt_65c02();
    return BREAK;
}

{BOS}ig?n?o?r?e?{EOS} {
    /* ignore everything */
    clear_halt(watchpoints, 0);
    sprintf(second_buf[num_buffer_lines++], "ignored all");
    return IGNORE;
}

{BOS}ig?n?o?r?e?({WS}+{DEC}+)+{EOS} {
    /* ignore <watchpt> ... */

    while (!isspace(*debugtext)) ++debugtext;

    while (*debugtext) {
	arg1 = strtol(debugtext, &debugtext, 10);
	if ((arg1 < 1) || (arg1 > MAX_BRKPTS)) {
	    sprintf(second_buf[num_buffer_lines++], "invalid watchpoint");
	    return IGNORE;
	}
	clear_halt(watchpoints, arg1);
	sprintf(second_buf[num_buffer_lines++], "ignored %d", arg1);
    }
    return IGNORE;
}


{BOS}cl?e?a?r?{EOS} {
    /* clear everything */
    clear_halt(breakpoints, 0);
    sprintf(second_buf[num_buffer_lines++], "cleared all");
    return CLEAR;
}

{BOS}cl?e?a?r?({WS}+{DEC}+)+{EOS} {
    /* clear <breakpt> ... */
    while (!isspace(*debugtext)) ++debugtext;

    while (*debugtext) {
	arg1 = strtol(debugtext, &debugtext, 10);
	if ((arg1 < 1) || (arg1 > MAX_BRKPTS)) {
	    sprintf(second_buf[num_buffer_lines++], "invalid breakpoint");
	    return CLEAR;
	}
	clear_halt(breakpoints, arg1);
	sprintf(second_buf[num_buffer_lines++], "cleared %d", arg1);
    }
    return CLEAR;
}

{BOS}cl?e?a?r?{WS}*op({WS}+{HEX}+)+{EOS} {
    /* clear <opcode breakpt> ... */
    while (!(*debugtext == 'p')) ++debugtext;
    ++debugtext;

    while (*debugtext) {
	arg1 = strtol(debugtext, &debugtext, 16);
	if ((arg1 < 0) || (arg1 > 255)) {
	    sprintf(second_buf[num_buffer_lines++], "invalid opcode");
	    return CLEAR;
	}
	clear_halt_opcode((unsigned char)arg1);
	sprintf(second_buf[num_buffer_lines++], "cleared opcode %02X",
		(unsigned char)arg1);
    }
    return CLEAR;
}

{BOS}cl65c02{EOS} {
    /* clear 65c02 ... */
    clear_halt_65c02();
    return CLEAR;
}

{BOS}stat?u?s?{EOS} {
    /* show breakpoints and watchpoints */
    show_breakpts();
    return STATUS;
}

{BOS}opc?o?d?e?s?{EOS} {
    /* show opcode breakpoints */
    show_opcode_breakpts();
    return OPCODES;
}

{BOS}sea?r?c?h?{WS}+{HEX}+{EOS} {
    /* search main memory for <bytes> */
    while (!isspace(*debugtext)) ++debugtext;
    while (isspace(*debugtext)) ++debugtext;

    search_mem(debugtext, 0, -1);

    return SEARCH;
}

{BOS}sea?r?c?h?{WS}+{BANK}{WS}+{HEX}+{EOS} {
    /* search memory<bank> for <bytes> */
    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */
    while (isspace(*debugtext)) ++debugtext;

    search_mem(debugtext, 0, arg3);

    return SEARCH;
}

{BOS}sea?r?c?h?{WS}*(lc?1|lc?2){WS}+{HEX}+{EOS} {
    /* search LC<bank> for <bytes> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (!isspace(*debugtext)) ++debugtext;
    while (isspace(*debugtext)) ++debugtext;

    search_mem(debugtext, lc, -1);

    return SEARCH;
}

{BOS}sea?r?c?h?{WS}*(lc?1|lc?2){WS}+{BANK}{WS}+{HEX}+{EOS} {
    /* search LC<bank> memory<bank> for <bytes> */
    int lc;

    while (tolower(*debugtext) != 'l') ++debugtext; ++debugtext;
    if (tolower(*debugtext) == 'c') ++debugtext;
    lc = strtol(debugtext, &debugtext, 10);

    while (*debugtext != '/') ++debugtext;
    ++debugtext;	/* after / */
    arg3 = strtol(debugtext, &debugtext, 10);
    ++debugtext;	/* after / */
    while (isspace(*debugtext)) ++debugtext;

    search_mem(debugtext, lc, arg3);

    return SEARCH;
}

{BOS}key{WS}+{HEX}+{EOS} {
    /* send key code to emulator */
    unsigned char key;

    while (!isspace(*debugtext)) ++debugtext;
    while (isspace(*debugtext)) ++debugtext;
    key = (unsigned char) strtol(debugtext, &debugtext, 16);

    apple_ii_64k[0][0xC000] = key;
    apple_ii_64k[1][0xC000] = key;
}

{BOS}la?n?g?{EOS} {
    /* display language card settings */
    show_lc_info();
    return LC;
}

{BOS}dri?v?e?{EOS} {
    /* show disk settings */
    show_disk_info();
    return DRIVE;
}

{BOS}vm?{EOS} {
    /* show other VM softswitch settings */
    show_misc_info();
    return VM;
}

{BOS}fre?s?h?{EOS} {
    /* refresh the screen */
    clear_debugger_screen();
}

{BOS}(\?|he?l?p?){EOS} {
    display_help();
    return HELP;
}

{BOS}bsave{WS}+{CHAR}+{WS}+{BANK}{ADDRS}{WS}+{HEX}+{EOS} {
    /* bsave <file> /<bank>/<addr> <len> */
    /* save memory dump to file */
    FILE *fp = NULL;
    char *ptr = NULL;
    int len, start, len2, bank;

    while (!isspace(*debugtext)) ++debugtext;
    while (isspace(*debugtext)) ++debugtext;

    /* copy file name */
    ptr = debugtext;
    while (!isspace(*debugtext)) ++debugtext;
    len = debugtext - ptr;
    strncpy(temp, ptr, len);
    temp[len] = '\0';

    /* get bank info */
    while (*debugtext != '/') ++debugtext;
    ++debugtext;
    bank = strtol(debugtext, &debugtext, 10);
    ++debugtext;

    /* extract start and len */
    start = strtol(debugtext, &debugtext, 16);
    len2 = strtol(debugtext, &debugtext, 16);

    fp = fopen(temp, "w");	/* try to open file for writing */
    if (fp == NULL) {
	sprintf(second_buf[num_buffer_lines++], "problem: %s", temp);
	perror(temp);
	return BSAVE;
    }
    len = fwrite(apple_ii_64k[bank]+start, 1, len2, fp);
    if (len < len2) {
	sprintf(second_buf[num_buffer_lines++], "problem: %s", temp);
	perror(temp);
	fclose(fp);
	return BSAVE;
    }

    sprintf(second_buf[num_buffer_lines++], "bsaved: %s", temp);
    fclose(fp);
    return BSAVE;
}

{BOS}log{EOS} {
    /* log debugger output to file - not implemented */
    return LOG;
}

{BOS}save{EOS} {
    /* save apple2 state to a .img file - not implemented I'd like to
     * be compatible with the applePC emulator's .img format. anyone
     * have documentation on this?  -ASC
     **/
    return SAVE;
}


\n	/* ignore newlines */

.	/* ignore extraneous characters */


%%


int yywrap () {
    return 1;
}

/* initialize the buffer - needed each time through */
void init_lex (char *str, int size) {
    if (buffer) yy_delete_buffer(buffer);
    buffer = yy_scan_buffer(str, size);

    if (!buffer) {	/* oops */
	video_shutdown();
	printf("lex buffer not big enough\n");
	exit(1);
    }	       
}