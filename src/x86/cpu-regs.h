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

#ifndef _CPU_REGS_H_
#define _CPU_REGS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cpu.h"

// Virtual machine is an Apple ][ (not an NES, etc...)
#define APPLE2_VM 1

#define X_Reg           %bl             /* 6502 X register in %bl  */
#define Y_Reg           %bh             /* 6502 Y register in %bh  */
#define A_Reg           %cl             /* 6502 A register in %cl  */
#define F_Reg           %ch             /* 6502 flags in %ch       */
#define SP_Reg_L        %dl             /* 6502 Stack pointer low  */
#define SP_Reg_H        %dh             /* 6502 Stack pointer high */
#define PC_Reg          %si             /* 6502 Program Counter    */
#define PC_Reg_L        %sil            /* 6502 PC low             */
#define PC_Reg_H        %sih            /* 6502 PC high            */
#define EffectiveAddr   %di             /* Effective address       */
#define EffectiveAddr_L %dil            /* Effective address low   */
#define EffectiveAddr_H %dih            /* Effective address high   */

#define X86_CF_Bit 0x0                  /* x86 carry               */
#define X86_AF_Bit 0x4                  /* x86 adj (nybble carry)  */

#define RestoreAltZP \
    /* Apple //e set stack point to ALTZP (or not) */ \
    movLQ   SN(base_stackzp), _XAX; \
    subLQ   SN(base_vmem), _XAX; \
    orLQ    _XAX, SP_Reg_X;

#ifdef __LP64__
#   define SZ_PTR           8
#   define ROR_BIT          63
// x86_64 registers
#   define _XBP             %rbp        /* x86_64 base pointer     */
#   define _XSP             %rsp        /* x86_64 stack pointer    */
#   define _XAX             %rax        /* scratch                 */
#   define _XBX             %rbx        /* scratch2                */
#   define _X8              %r8
// full-length Apple ][ registers
#   define XY_Reg_X         %rbx        /* 6502 X&Y flags          */
#   define AF_Reg_X         %rcx        /* 6502 F&A flags          */
#   define SP_Reg_X         %rdx        /* 6502 Stack pointer      */
#   define PC_Reg_X         %rsi        /* 6502 Program Counter    */
#   define EffectiveAddr_X  %rdi        /* Effective address       */
// full-length assembly instructions
#   define addLQ            addq
#   define andLQ            andq
#   define callLQ           callq
#   define decLQ            decq
#   define leaLQ            leaq
#   define orLQ             orq
#   define movLQ            movq
#   define movzbLQ          movzbq
#   define movzwLQ          movzwq
#   define popaLQ           popaq
#   define popLQ            popq
#   define pushaLQ          pushaq
#   define pushfLQ          pushfq
#   define pushLQ           pushq
#   define rorLQ            rorq
#   define shlLQ            shlq
#   define shrLQ            shrq
#   define subLQ            subq
#   define testLQ           testq
#   define xorLQ            xorq
#else
#   define SZ_PTR           4
#   define ROR_BIT          31
// x86 registers
#   define _XBP             %ebp        /* x86 base pointer        */
#   define _XSP             %esp        /* x86 stack pointer       */
#   define _XAX             %eax        /* scratch                 */
#   define _XBX             %ebx        /* scratch2                */
// full-length Apple ][ registers
#   define XY_Reg_X         %ebx        /* 6502 X&Y flags          */
#   define AF_Reg_X         %ecx        /* 6502 F&A flags          */
#   define SP_Reg_X         %edx        /* 6502 Stack pointer      */
#   define PC_Reg_X         %esi        /* 6502 Program Counter    */
#   define EffectiveAddr_X  %edi        /* Effective address       */
// full-length assembly instructions
#   define addLQ            addl
#   define andLQ            andl
#   define callLQ           calll
#   define decLQ            decl
#   define leaLQ            leal
#   define orLQ             orl
#   define movLQ            movl
#   define movzbLQ          movzbl
#   define movzwLQ          movzwl
#   define popaLQ           popal
#   define popLQ            popl
#   define pushaLQ          pushal
#   define pushfLQ          pushfl
#   define pushLQ           pushl
#   define rorLQ            rorl
#   define shlLQ            shll
#   define shrLQ            shrl
#   define subLQ            subl
#   define testLQ           testl
#   define xorLQ            xorl
#endif

/* Symbol naming issues */
#ifdef NO_UNDERSCORES
#define         SN(foo) foo
#define         SNX(foo, INDEX, SCALE) foo(,INDEX,SCALE)
#define         SNX_PROLOGUE(foo)
#define         E(foo)          .globl foo; .balign 16; foo##:
#define         CALL(foo) foo
#else /* !NO_UNDERSCORES */
#if defined(__APPLE__)
#   warning "2014/06/22 -- Apple's clang appears to not like certain manipulations of %_h register values (for example %ah, %ch) that are valid on *nix ... and it creates bizarre bytecode
#   define APPLE_ASSEMBLER_IS_BROKEN 1
#   define         SN(foo) _##foo(%rip)
#   define         SNX(foo, INDEX, SCALE) (_X8,INDEX,SCALE)
#   ifdef __LP64__
#       define     SNX_PROLOGUE(foo)  leaLQ   _##foo(%rip), _X8;
#   else
#       error "Building 32bit Darwin/x86 is not supported (unless you're a go-getter and make it supported)"
#   endif
#   define         E(foo)          .globl _##foo; .balign 4; _##foo##:
#else
#   define         SN(foo) _##foo
#   define         SNX(foo, INDEX, SCALE) _##foo(,INDEX,SCALE)
#   define         SNX_PROLOGUE(foo)
#   define         E(foo)          .globl _##foo; .balign 16; _##foo##:
#endif
#define         CALL(foo) _##foo
#endif /* !NO_UNDERSCORES */

#endif // whole file
