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

#include "greatest.h"
#include "testcommon.h"
#include "common.h"

#define fC C_Flag_6502  // [C]arry
#define fZ Z_Flag_6502  // [Z]ero
#define fI I_Flag_6502  // [I]nterrupt
#define fD D_Flag_6502  // [D]ecimal
#define fB B_Flag_6502  // [B]reak
#define fX X_Flag_6502  // [X]tra (reserved)...
#define fV V_Flag_6502  // o[V]erflow
#define fN N_Flag_6502  // [N]egative

#define TEST_LOC 0x1f82
#define TEST_LOC_LO 0x82

#define RW_NONE 0x0
#define RW_READ 0x1
#define RW_WRITE 0x2

#define MSG_FLAGS0 "A:%02X op %02X = %02X : %s (emul2ix = %02X : %s)"

#define HEADER0() \
    uint8_t result = 0x0; \
    uint8_t flags = 0x0; \
    char buf0[MSG_SIZE]; \
    char buf1[MSG_SIZE]; \
    char msgbuf[MSG_SIZE];

#define VERIFY_FLAGS() \
    flags_to_string(flags, buf0); \
    flags_to_string(cpu65_current.f, buf1); \
    snprintf(msgbuf, MSG_SIZE, MSG_FLAGS0, regA, val, result, buf0, cpu65_current.a, buf1); \
    ASSERTm(msgbuf, cpu65_current.f == flags);

static void testcpu_setup(void *arg) {

    //reinitialize();

    cpu65_cycles_to_execute = 1;

    cpu65_current.pc = TEST_LOC;
    cpu65_current.a = 0x0;
    cpu65_current.x = 0x0;
    cpu65_current.y = 0x0;
    cpu65_current.f = 0x0;
    cpu65_current.sp = 0xff;

    cpu65_debug.ea = 0xffff;
    cpu65_debug.d = 0xff;
    cpu65_debug.rw = 0xff;
    cpu65_debug.opcode = 0xff;
    cpu65_debug.opcycles = 0xff;

    // clear ZP & stack memory
    memset(apple_ii_64k, 0x0, 0x200);

    // clear prog memory and absolute addressing test locations
    memset(((void*)apple_ii_64k)+TEST_LOC, 0x0, 0x300);
}

static void testcpu_teardown(void *arg) {
    // ...
}

static void testcpu_breakpoint(void *arg) {
    fprintf(GREATEST_STDOUT, "set breakpoint on testcpu_breakpoint to check for problems...\n");
}

static void testcpu_set_opcode3(uint8_t op, uint8_t val, uint8_t arg1) {
    apple_ii_64k[0][TEST_LOC+0] = op;
    apple_ii_64k[0][TEST_LOC+1] = val;
    apple_ii_64k[0][TEST_LOC+2] = arg1;
}

static void testcpu_set_opcode2(uint8_t op, uint8_t val) {
    uint8_t arg1 = (uint8_t)random();
    testcpu_set_opcode3(op, val, arg1);
}

static void testcpu_set_opcode1(uint8_t op) {
    uint8_t val = (uint8_t)random();
    uint8_t arg1 = (uint8_t)random();
    testcpu_set_opcode3(op, val, arg1);
}

static bool check_skip_illegal_bcd(uint8_t bcd0, uint8_t bcd1) {
    if (bcd0 >= 0xA0) {
        return true;
    }
    if ((bcd0 & 0x0f) >= 0x0A) {
        return true;
    }

    if (bcd1 >= 0xA0) {
        return true;
    }
    if ((bcd1 & 0x0f) >= 0x0A) {
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
// ADC instructions

static void logic_ADC_dec(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {

    // componentize
    uint8_t x_lo = 0x0;
    uint8_t x_hi = 0x0;

    uint8_t a_lo = (((uint8_t)_a) & 0x0f);
    uint8_t b_lo = (((uint8_t)_b) & 0x0f);

    uint8_t a_hi = (((uint8_t)_a) & 0xf0)>>4;
    uint8_t b_hi = (((uint8_t)_b) & 0xf0)>>4;

    uint8_t carry = (*flags & fC) ? 1 : 0;
    *flags &= ~ fC;

    // BCD add
    x_lo = a_lo + b_lo + carry;
    carry = 0;
    if (x_lo > 9) {
        x_lo += 6;
        carry = (x_lo>>4); // +1 or +2
        x_lo &= 0x0f;
    }
    x_hi = a_hi + b_hi + carry;
    if (x_hi > 9) {
        *flags |= fC;
        x_hi += 6;
    }

    // merge result
    x_hi <<= 4;
    *result = x_hi | x_lo;

    // flags
    if (*result == 0x00) {
        *flags |= fZ;
    }

    if (*result & 0x80) {
        *flags |= fN;
    }
}

static void logic_ADC(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {

    if (*flags & fD) {
        logic_ADC_dec(_a, _b, result, flags);
        return;
    }

    int8_t a = (int8_t)_a;
    int8_t b = (int8_t)_b;
    bool is_neg = (a < 0);
    bool is_negb = (b < 0);

    int8_t carry = (*flags & fC) ? 1 : 0;
    *flags &= ~fC;

    int8_t res = a + b + carry;

    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    int32_t res32 = (uint8_t)a+(uint8_t)b+(uint8_t)carry;
    if (res32 & 0x00000100) {
        *flags |= fC;
    }

    if ( is_neg && is_negb ) {
        if (a < res) {
            *flags |= fV;
        }
    }
    if ( !is_neg && !is_negb ) {
        if (a > res) {
            *flags |= fV;
        }
    }

    *result = (uint8_t)(res & 0xff);
}

TEST test_ADC_imm(uint8_t regA, uint8_t val, bool decimal, bool carry) {
    HEADER0();

    flags |= decimal ? (fD) : 0x00;
    flags |= carry   ? (fC) : 0x00;

    if (decimal && check_skip_illegal_bcd(regA, val)) {
        // NOTE : FIXME TODO skip undocumented/illegal BCD
        SKIPm("Z");
    }

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode2(0x69, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f |= decimal ? (fD) : 0x00;
    cpu65_current.f |= carry   ? (fC) : 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    snprintf(msgbuf, MSG_SIZE, MSG_FLAGS0, regA, val, result, buf0, cpu65_current.a, buf1);
    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x69);
    ASSERT(cpu65_debug.opcycles  == (decimal ? 3 : 2));

    PASS();
}

TEST test_ADC_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode2(0x65, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x65);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_ADC_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    uint8_t idx = arg0 + regX;

    testcpu_set_opcode2(0x75, arg0);

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x75);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_ADC_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode3(0x6d, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x6d);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_ADC_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode3(0x7d, lobyte, hibyte);

    uint8_t cycle_count = 0;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x7d);

    cycle_count += 4;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_ADC_abs_y(uint8_t regA, uint8_t val, uint8_t regY, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode3(0x79, lobyte, hibyte);

    uint8_t cycle_count = 0;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x02;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0x02);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x79);

    cycle_count += 4;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_ADC_ind_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode2(0x61, arg0);

    uint8_t idx_lo = arg0 + regX;
    uint8_t idx_hi = idx_lo+1;
    uint16_t addrs = lobyte | (hibyte<<8);

    apple_ii_64k[0][idx_lo] = lobyte;
    apple_ii_64k[0][idx_hi] = hibyte;
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x15;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x15);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x61);

    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_ADC_ind_y(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regY, uint8_t val_zp0, uint8_t val_zp1) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode2(0x71, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = val_zp0;
    apple_ii_64k[0][idx1] = val_zp1;

    uint8_t cycle_count = 0;
    uint16_t addrs = val_zp0 | (val_zp1<<8);
    addrs += (uint8_t)regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)val_zp1) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x84;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x84);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x71);
    cycle_count += 5;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_ADC_ind_zpage(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_ADC(regA, val, &result, &flags);

    testcpu_set_opcode2(0x72, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = lobyte;
    apple_ii_64k[0][idx1] = hibyte;

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x14;
    cpu65_current.y  = 0x85;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x14);
    ASSERT(cpu65_current.y       == 0x85);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x72);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

// ----------------------------------------------------------------------------
// AND instructions

static void logic_AND(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;
    uint8_t b = (uint8_t)_b;

    uint8_t res = a & b;
    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    *result = res;
}

TEST test_AND_imm(uint8_t regA, uint8_t val) {
    HEADER0();

    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x29, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    snprintf(msgbuf, MSG_SIZE, MSG_FLAGS0, regA, val, result, buf0, cpu65_current.a, buf1);
    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x29);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_AND_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x25, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x25);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_AND_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x35, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x35);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_AND_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode3(0x2d, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x2d);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_AND_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode3(0x3d, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x3d);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_AND_abs_y(uint8_t regA, uint8_t val, uint8_t regY, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode3(0x39, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x02;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0x02);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x39);
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_AND_ind_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x21, arg0);

    uint8_t idx_lo = arg0 + regX;
    uint8_t idx_hi = idx_lo+1;
    uint16_t addrs = lobyte | (hibyte<<8);

    apple_ii_64k[0][idx_lo] = lobyte;
    apple_ii_64k[0][idx_hi] = hibyte;
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x15;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x15);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x21);

    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_AND_ind_y(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regY, uint8_t val_zp0, uint8_t val_zp1) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x31, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = val_zp0;
    apple_ii_64k[0][idx1] = val_zp1;

    uint8_t cycle_count = 5;
    uint16_t addrs = val_zp0 | (val_zp1<<8);
    addrs += (uint8_t)regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)val_zp1) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x84;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x84);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x31);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_AND_ind_zpage(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_AND(regA, val, &result, &flags);

    testcpu_set_opcode2(0x32, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = lobyte;
    apple_ii_64k[0][idx1] = hibyte;

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x14;
    cpu65_current.y  = 0x85;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x14);
    ASSERT(cpu65_current.y       == 0x85);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x32);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

// ----------------------------------------------------------------------------
// ASL instructions

static void logic_ASL(/*uint8_t*/int _a, uint8_t *result, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;

    *flags |= (a & 0x80) ? fC : 0;
    uint8_t res = a<<1;

    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    *result = res;
}

TEST test_ASL_acc(uint8_t regA) {
    uint8_t val = 0xff;
    HEADER0();

    logic_ASL(regA, &result, &flags);

    testcpu_set_opcode1(0x0a);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x0a);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_ASL_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();
    logic_ASL(val, &result, &flags);

    testcpu_set_opcode2(0x06, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(apple_ii_64k[0][arg0] == result);

    ASSERT(cpu65_current.a  == regA);
    ASSERT(cpu65_current.pc == TEST_LOC+2);
    ASSERT(cpu65_current.x  == 0x03);
    ASSERT(cpu65_current.y  == 0x04);
    ASSERT(cpu65_current.sp == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0x06);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

TEST test_ASL_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();
    logic_ASL(val, &result, &flags);

    testcpu_set_opcode2(0x16, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(apple_ii_64k[0][idx] == result);

    ASSERT(cpu65_current.a   == regA); 
    ASSERT(cpu65_current.pc  == TEST_LOC+2);
    ASSERT(cpu65_current.x   == regX);
    ASSERT(cpu65_current.y   == 0x05);
    ASSERT(cpu65_current.sp  == 0x81);
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0x16);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_ASL_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_ASL(val, &result, &flags);

    testcpu_set_opcode3(0x0e, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(apple_ii_64k[0][addrs] == result); 

    ASSERT(cpu65_current.a  == regA); 
    ASSERT(cpu65_current.pc == TEST_LOC+3);
    ASSERT(cpu65_current.x  == 0xf4);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0x0e);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_ASL_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_ASL(val, &result, &flags);

    testcpu_set_opcode3(0x1e, lobyte, hibyte);

    uint8_t cycle_count = 6;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(apple_ii_64k[0][addrs] == result); 

    ASSERT(cpu65_current.a  == regA); 
    ASSERT(cpu65_current.pc == TEST_LOC+3);
    ASSERT(cpu65_current.x  == regX);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0x1e);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// ----------------------------------------------------------------------------
// Branch instructions

TEST test_BCC(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fC : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0x90;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x90);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BCS(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fC : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (!flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0xB0;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0xB0);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BEQ(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fZ : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (!flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0xF0;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0xF0);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BNE(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fZ : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0xD0;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0xD0);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BMI(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fN : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (!flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0x30;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x30);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BPL(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fN : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0x10;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x10);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BRA(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fN : 0;

    uint8_t cycle_count = 3;
    uint16_t newpc = addrs+2+off;
    if ((newpc&0xFF00) != (addrs&0xFF00)) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs+0] = 0x80;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x80);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BVC(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fV : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0x50;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x50);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_BVS(int8_t off, bool flag, uint16_t addrs) {
    HEADER0();

    cpu65_current.pc = addrs;
    flags |= flag ? fV : 0;

    uint8_t cycle_count = 2;
    uint16_t newpc = 0xffff;
    if (!flag) {
        newpc = addrs+2;
    } else {
        newpc = addrs+2+off;
        ++cycle_count;
        if ((newpc&0xFF00) != (addrs&0xFF00)) {
            ++cycle_count;
        }
    }

    apple_ii_64k[0][addrs+0] = 0x70;
    apple_ii_64k[0][addrs+1] = off;
    apple_ii_64k[0][addrs+2] = (uint8_t)random();

    cpu65_current.a  = 0xed;
    cpu65_current.x  = 0xde;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = flags;

    cpu65_run();

    ASSERT(cpu65_current.pc == newpc);
    ASSERT(cpu65_current.a  == 0xed); 
    ASSERT(cpu65_current.x  == 0xde);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);
    ASSERT(cpu65_current.f  == flags);

    ASSERT(cpu65_debug.ea        == addrs+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == 0);
    ASSERT(cpu65_debug.opcode    == 0x70);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// ----------------------------------------------------------------------------
// BIT instructions

TEST test_BIT_imm(uint8_t regA, uint8_t val) {
    HEADER0();

    if ((regA & val) == 0x0) {
        flags = fZ;
    }

    testcpu_set_opcode2(0x89, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc == TEST_LOC+2);
    ASSERT(cpu65_current.a  == regA);
    ASSERT(cpu65_current.x  == 0x03);
    ASSERT(cpu65_current.y  == 0x04);
    ASSERT(cpu65_current.sp == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x89);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

static void logic_BIT(uint8_t regA, uint8_t val, uint8_t *flags) {
    if ((regA & val) == 0x0) {
        *flags |= fZ;
    }
    if (val & 0x80) {
        *flags |= fN;
    }
    if (val & 0x40) {
        *flags |= fV;
    }
}

TEST test_BIT_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_BIT(regA, val, &flags);

    testcpu_set_opcode2(0x24, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x24);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_BIT_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_BIT(regA, val, &flags);

    testcpu_set_opcode2(0x34, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc == TEST_LOC+2);
    ASSERT(cpu65_current.a  == regA);
    ASSERT(cpu65_current.x  == regX);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x34);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_BIT_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_BIT(regA, val, &flags);

    testcpu_set_opcode3(0x2c, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc == TEST_LOC+3);
    ASSERT(cpu65_current.a  == regA);
    ASSERT(cpu65_current.x  == 0xf4);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x2c);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_BIT_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_BIT(regA, val, &flags);

    testcpu_set_opcode3(0x3c, lobyte, hibyte);

    uint8_t cycle_count = 0;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc == TEST_LOC+3);
    ASSERT(cpu65_current.a  == regA);
    ASSERT(cpu65_current.x  == regX);
    ASSERT(cpu65_current.y  == 0x05);
    ASSERT(cpu65_current.sp == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x3c);

    cycle_count += 4;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

// ----------------------------------------------------------------------------
// BRK operand (and IRQ handling)

TEST test_BRK() {
    testcpu_set_opcode1(0x00);

    ASSERT(apple_ii_64k[0][0x1ff] != 0x1f);
    ASSERT(apple_ii_64k[0][0x1fe] != TEST_LOC_LO+2);

    cpu65_current.a = 0x02;
    cpu65_current.x = 0x03;
    cpu65_current.y = 0x04;
    cpu65_current.f = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc     == 0xc3fa);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.f      == (fB|fX|fI));
    ASSERT(cpu65_current.sp     == 0xfc);

    ASSERT(apple_ii_64k[0][0x1ff] == 0x1f);
    ASSERT(apple_ii_64k[0][0x1fe] == TEST_LOC_LO+2);
    ASSERT(apple_ii_64k[0][0x1fd] == cpu65_flags_encode[B_Flag|X_Flag]);

    ASSERT(cpu65_debug.ea       == 0xfffe);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0x0);
    ASSERT(cpu65_debug.opcycles == (7));

    PASS();
}

TEST test_IRQ() {
    // NOTE : not an opcode
    testcpu_set_opcode1(0xea/*NOP*/); // Implementation NOTE: first an instruction, then reset is handled

    cpu65_cycles_to_execute = 3;
    cpu65_interrupt(IRQGeneric);

    ASSERT(apple_ii_64k[0][0x1ff] != 0x1f);
    ASSERT(apple_ii_64k[0][0x1fe] != TEST_LOC_LO+1);

    cpu65_current.a = 0x02;
    cpu65_current.x = 0x03;
    cpu65_current.y = 0x04;
    cpu65_current.f = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc     == 0xc3fd);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.f      == (fB|fX|fI|fZ)); // Implementation NOTE : Z set by 2nd BIT instruction at C3FA
    ASSERT(cpu65_current.sp     == 0xfc);

    ASSERT(apple_ii_64k[0][0x1ff] == 0x1f);
    ASSERT(apple_ii_64k[0][0x1fe] == TEST_LOC_LO+1);
    ASSERT(apple_ii_64k[0][0x1fd] == cpu65_flags_encode[X_Flag]);

    ASSERT(cpu65_debug.ea       == 0xc015);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_READ);
    ASSERT(cpu65_debug.opcode   == 0x2c);
    ASSERT(cpu65_debug.opcycles == (4));

    PASS();
}

// ----------------------------------------------------------------------------
// CLx operands

TEST test_CLC() {
    testcpu_set_opcode1(0x18);

    cpu65_current.a  = 0x02;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = fC;

    cpu65_run();

    ASSERT(cpu65_current.pc     == TEST_LOC+1);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.sp     == 0x81);
    ASSERT(cpu65_current.f      == 0x00);

    ASSERT(cpu65_debug.ea       == TEST_LOC);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0x18);
    ASSERT(cpu65_debug.opcycles == (2));

    PASS();
}

TEST test_CLD(uint8_t regA, uint8_t val) {
    testcpu_set_opcode1(0xd8);

    cpu65_current.a  = 0x02;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = fD;

    cpu65_run();

    ASSERT(cpu65_current.pc     == TEST_LOC+1);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.sp     == 0x81);
    ASSERT(cpu65_current.f      == 0x00);

    ASSERT(cpu65_debug.ea       == TEST_LOC);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0xd8);
    ASSERT(cpu65_debug.opcycles == (2));

    PASS();
}

TEST test_CLI(uint8_t regA, uint8_t val) {
    testcpu_set_opcode1(0x58);

    cpu65_current.a  = 0x02;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = fI;

    cpu65_run();

    ASSERT(cpu65_current.pc     == TEST_LOC+1);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.sp     == 0x81);
    ASSERT(cpu65_current.f      == 0x00);

    ASSERT(cpu65_debug.ea       == TEST_LOC);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0x58);
    ASSERT(cpu65_debug.opcycles == (2));

    PASS();
}

TEST test_CLV(uint8_t regA, uint8_t val) {
    testcpu_set_opcode1(0xb8);

    cpu65_current.a  = 0x02;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = fV;

    cpu65_run();

    ASSERT(cpu65_current.pc     == TEST_LOC+1);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.sp     == 0x81);
    ASSERT(cpu65_current.f      == 0x00);

    ASSERT(cpu65_debug.ea       == TEST_LOC);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0xb8);
    ASSERT(cpu65_debug.opcycles == (2));

    PASS();
}

// ----------------------------------------------------------------------------
// CMP instructions

static void logic_CMP(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;
    uint8_t b = (uint8_t)_b;

    uint8_t res = a - b;

    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }
    if (b <= a) {
        *flags |= fC;
    }
}

TEST test_CMP_imm(uint8_t regA, uint8_t val) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xC9, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xC9);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_CMP_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xc5, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xc5);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_CMP_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xd5, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xd5);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_CMP_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode3(0xcd, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xcd);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_CMP_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode3(0xdd, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xdd);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_CMP_abs_y(uint8_t regA, uint8_t val, uint8_t regY, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode3(0xd9, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x02;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x02);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xd9);
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_CMP_ind_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xc1, arg0);

    uint8_t idx_lo = arg0 + regX;
    uint8_t idx_hi = idx_lo+1;
    uint16_t addrs = lobyte | (hibyte<<8);

    apple_ii_64k[0][idx_lo] = lobyte;
    apple_ii_64k[0][idx_hi] = hibyte;
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x15;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x15);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xc1);

    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_CMP_ind_y(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regY, uint8_t val_zp0, uint8_t val_zp1) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xd1, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = val_zp0;
    apple_ii_64k[0][idx1] = val_zp1;

    uint8_t cycle_count = 5;
    uint16_t addrs = val_zp0 | (val_zp1<<8);
    addrs += (uint8_t)regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)val_zp1) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x84;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x84);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xd1);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// 65c02 : 0xD2
TEST test_CMP_ind_zpage(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_CMP(regA, val, &flags);

    testcpu_set_opcode2(0xd2, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = lobyte;
    apple_ii_64k[0][idx1] = hibyte;

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x14;
    cpu65_current.y  = 0x85;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x14);
    ASSERT(cpu65_current.y       == 0x85);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xd2);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

// ----------------------------------------------------------------------------
// CPx CPy instructions

TEST test_CPX_imm(uint8_t regX, uint8_t val) {
    HEADER0();
    uint8_t regA = 0xaa;

    logic_CMP(regX, val, &flags);

    testcpu_set_opcode2(0xe0, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xe0);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_CPX_zpage(uint8_t regX, uint8_t val, uint8_t arg0) {
    HEADER0();
    uint8_t regA = 0x55;

    logic_CMP(regX, val, &flags);

    testcpu_set_opcode2(0xe4, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xe4);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_CPX_abs(uint8_t regX, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    uint8_t regA = 0xAA;

    logic_CMP(regX, val, &flags);

    testcpu_set_opcode3(0xec, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xec);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_CPY_imm(uint8_t regY, uint8_t val) {
    HEADER0();
    uint8_t regA = 0xaa;

    logic_CMP(regY, val, &flags);

    testcpu_set_opcode2(0xc0, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x66;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x66);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xc0);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_CPY_zpage(uint8_t regY, uint8_t val, uint8_t arg0) {
    HEADER0();
    uint8_t regA = 0x55;

    logic_CMP(regY, val, &flags);

    testcpu_set_opcode2(0xc4, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x27;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x27);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xc4);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_CPY_abs(uint8_t regY, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    uint8_t regA = 0xAA;

    logic_CMP(regY, val, &flags);

    testcpu_set_opcode3(0xcc, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x7b;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x7b);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xcc);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

// ----------------------------------------------------------------------------
// DEA, DEX, DEY instructions

static void logic_DEx(/*uint8_t*/int _a, uint8_t *result, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;

    uint8_t res = a-1;
    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    *result = res;
}

TEST test_DEA(uint8_t regA, uint8_t val) {
    HEADER0();

    logic_DEx(regA, &result, &flags);

    testcpu_set_opcode1(0x3a);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == result);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0x3a);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_DEX(uint8_t regX, uint8_t val) {
    HEADER0();
    uint8_t regA = 0x12;

    logic_DEx(regX, &result, &flags);

    testcpu_set_opcode1(0xca);

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == result);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0xca);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_DEY(uint8_t regY, uint8_t val) {
    HEADER0();
    uint8_t regA = 0x12;

    logic_DEx(regY, &result, &flags);

    testcpu_set_opcode1(0x88);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x13;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x13);
    ASSERT(cpu65_current.y       == result);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0x88);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

// ----------------------------------------------------------------------------
// DEC instructions

TEST test_DEC_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_DEx(val, &result, &flags);

    testcpu_set_opcode2(0xc6, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(apple_ii_64k[0][arg0] == result);
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xc6);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

TEST test_DEC_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_DEx(val, &result, &flags);

    testcpu_set_opcode2(0xd6, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][idx]  == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xd6);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_DEC_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_DEx(val, &result, &flags);

    testcpu_set_opcode3(0xce, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][addrs] == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xce);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_DEC_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_DEx(val, &result, &flags);

    testcpu_set_opcode3(0xde, lobyte, hibyte);

    uint8_t cycle_count = 6;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][addrs] == result);
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xde);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// ----------------------------------------------------------------------------
// EOR instructions

static void logic_EOR(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;
    uint8_t b = (uint8_t)_b;

    uint8_t res = a ^ b;
    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    *result = res;
}

TEST test_EOR_imm(uint8_t regA, uint8_t val) {
    HEADER0();

    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x49, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    snprintf(msgbuf, MSG_SIZE, MSG_FLAGS0, regA, val, result, buf0, cpu65_current.a, buf1);
    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x49);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_EOR_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x45, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x45);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_EOR_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x55, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x55);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_EOR_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode3(0x4d, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x4d);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_EOR_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode3(0x5d, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x5d);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

TEST test_EOR_abs_y(uint8_t regA, uint8_t val, uint8_t regY, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode3(0x59, lobyte, hibyte);

    uint8_t cycle_count = 4;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x02;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0x02);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x59);
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_EOR_ind_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x41, arg0);

    uint8_t idx_lo = arg0 + regX;
    uint8_t idx_hi = idx_lo+1;
    uint16_t addrs = lobyte | (hibyte<<8);

    apple_ii_64k[0][idx_lo] = lobyte;
    apple_ii_64k[0][idx_hi] = hibyte;
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x15;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x15);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x41);

    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_EOR_ind_y(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regY, uint8_t val_zp0, uint8_t val_zp1) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x51, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = val_zp0;
    apple_ii_64k[0][idx1] = val_zp1;

    uint8_t cycle_count = 5;
    uint16_t addrs = val_zp0 | (val_zp1<<8);
    addrs += (uint8_t)regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)val_zp1) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x84;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x84);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x51);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// 65c02 : 0x52
TEST test_EOR_ind_zpage(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();
    logic_EOR(regA, val, &result, &flags);

    testcpu_set_opcode2(0x52, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = lobyte;
    apple_ii_64k[0][idx1] = hibyte;

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x14;
    cpu65_current.y  = 0x85;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x14);
    ASSERT(cpu65_current.y       == 0x85);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0x52);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

// ----------------------------------------------------------------------------
// INA, INX, INY instructions

static void logic_INx(/*uint8_t*/int _a, uint8_t *result, uint8_t *flags) {
    uint8_t a = (uint8_t)_a;

    uint8_t res = a+1;
    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    *result = res;
}

TEST test_INA(uint8_t regA, uint8_t val) {
    HEADER0();

    logic_INx(regA, &result, &flags);

    testcpu_set_opcode1(0x1a);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == result);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0x1a);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_INX(uint8_t regX, uint8_t val) {
    HEADER0();
    uint8_t regA = 0x31;

    logic_INx(regX, &result, &flags);

    testcpu_set_opcode1(0xe8);

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == result);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0xe8);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

TEST test_INY(uint8_t regY, uint8_t val) {
    HEADER0();
    uint8_t regA = 0x21;

    logic_INx(regY, &result, &flags);

    testcpu_set_opcode1(0xc8);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x13;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+1);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x13);
    ASSERT(cpu65_current.y       == result);
    ASSERT(cpu65_current.sp      == 0x80);

    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_NONE);
    ASSERT(cpu65_debug.opcode    == 0xc8);
    ASSERT(cpu65_debug.opcycles  == (2));

    PASS();
}

// ----------------------------------------------------------------------------
// INC instructions

TEST test_INC_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_INx(val, &result, &flags);

    testcpu_set_opcode2(0xe6, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(apple_ii_64k[0][arg0] == result);
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xe6);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

TEST test_INC_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_INx(val, &result, &flags);

    testcpu_set_opcode2(0xf6, arg0);

    uint8_t idx = arg0+regX;

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][idx]  == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xf6);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_INC_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_INx(val, &result, &flags);

    testcpu_set_opcode3(0xee, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][addrs] == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xee);
    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_INC_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_INx(val, &result, &flags);

    testcpu_set_opcode3(0xfe, lobyte, hibyte);

    uint8_t cycle_count = 6;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;
    cpu65_current.f  = 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.a       == regA);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(apple_ii_64k[0][addrs] == result);
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == result);
    ASSERT(cpu65_debug.rw        == (RW_READ|RW_WRITE));
    ASSERT(cpu65_debug.opcode    == 0xfe);
    ASSERT(cpu65_debug.opcycles  == cycle_count);

    PASS();
}

// ----------------------------------------------------------------------------
// NOP operand

TEST test_NOP() {
    testcpu_set_opcode1(0xea);

    cpu65_current.a  = 0x02;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;
    cpu65_current.f  = 0x55;

    cpu65_run();

    ASSERT(cpu65_current.pc     == TEST_LOC+1);
    ASSERT(cpu65_current.a      == 0x02);
    ASSERT(cpu65_current.x      == 0x03);
    ASSERT(cpu65_current.y      == 0x04);
    ASSERT(cpu65_current.f      == 0x55);
    ASSERT(cpu65_current.sp     == 0x80);

    ASSERT(cpu65_debug.ea       == TEST_LOC);
    ASSERT(cpu65_debug.d        == 0xff);
    ASSERT(cpu65_debug.rw       == RW_NONE);
    ASSERT(cpu65_debug.opcode   == 0xea);
    ASSERT(cpu65_debug.opcycles == (2));

    PASS();
}

// ----------------------------------------------------------------------------
// SBC instructions

static void logic_SBC_dec(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {

    // componentize
    uint8_t x_lo = 0x0;
    uint8_t x_hi = 0x0;

    uint8_t a_lo = (((uint8_t)_a) & 0x0f);
    uint8_t b_lo = (((uint8_t)_b) & 0x0f);

    uint8_t a_hi = (((uint8_t)_a) & 0xf0)>>4;
    uint8_t b_hi = (((uint8_t)_b) & 0xf0)>>4;

    uint8_t borrow = ((*flags & fC) == 0x0) ? 1 : 0;
    *flags |= fC;

    // BCD subtract
    x_lo = a_lo - b_lo - borrow;
    borrow = 0;
    if (x_lo > 9) {
        borrow = 1;
        x_lo -= 6;
        x_lo &= 0x0f;
    }
    x_hi = a_hi - b_hi - borrow;
    if (x_hi > 9) {
        *flags &= ~fC;
        x_hi -= 6;
    }

    // merge result
    x_hi <<= 4;
    *result = x_hi | x_lo;

    // flags
    if (*result == 0x00) {
        *flags |= fZ;
    }

    if (*result & 0x80) {
        *flags |= fN;
    }
}

static void logic_SBC(/*uint8_t*/int _a, /*uint8_t*/int _b, uint8_t *result, uint8_t *flags) {

    if (*flags & fD) {
        logic_SBC_dec(_a, _b, result, flags);
        return;
    }

    int8_t a = (int8_t)_a;
    int8_t b = (int8_t)_b;
    bool is_neg = (a < 0);
    bool is_negb = (b < 0);

    int8_t borrow = ((*flags & fC) == 0x0) ? 1 : 0;
    *flags |= fC;

    int8_t res = a - b - borrow;
    if ((res & 0xff) == 0x0) {
        *flags |= fZ;
    }
    if (res & 0x80) {
        *flags |= fN;
    }

    int32_t res32 = (uint8_t)a-(uint8_t)b-(uint8_t)borrow;
    if (res32 & 0x10000000) {
        *flags &= ~fC;
    }

    if ( !is_neg && is_negb ) {
        if (a > res) {
            *flags |= fV;
        }
    }
    if ( is_neg && !is_negb ) {
        if (a < res) {
            *flags |= fV;
        }
    }

    *result = (uint8_t)(res & 0xff);
}

TEST test_SBC_imm(uint8_t regA, uint8_t val, bool decimal, bool carry) {
    HEADER0();

    if (decimal && check_skip_illegal_bcd(regA, val)) {
        // NOTE : FIXME TODO skip undocumented/illegal BCD
        SKIPm("Z");
    }

    flags |= decimal ? (fD) : 0x00;
    flags |= carry   ? (fC) : 0x00;

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode2(0xe9, val);

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x33;
    cpu65_current.y  = 0x44;
    cpu65_current.sp = 0x88;
    cpu65_current.f |= decimal ? (fD) : 0x00;
    cpu65_current.f |= carry   ? (fC) : 0x00;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x33);
    ASSERT(cpu65_current.y       == 0x44);
    ASSERT(cpu65_current.sp      == 0x88);

    snprintf(msgbuf, MSG_SIZE, MSG_FLAGS0, regA, val, result, buf0, cpu65_current.a, buf1);
    ASSERTm(msgbuf, cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == TEST_LOC+1);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xe9);
    ASSERT(cpu65_debug.opcycles  == (decimal ? 3 : 2));

    PASS();
}

TEST test_SBC_zpage(uint8_t regA, uint8_t val, uint8_t arg0) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode2(0xe5, arg0);

    apple_ii_64k[0][arg0] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x03;
    cpu65_current.y  = 0x04;
    cpu65_current.sp = 0x80;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x03);
    ASSERT(cpu65_current.y       == 0x04);
    ASSERT(cpu65_current.sp      == 0x80);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == arg0);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xe5);
    ASSERT(cpu65_debug.opcycles  == (3));

    PASS();
}

TEST test_SBC_zpage_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    uint8_t idx = arg0 + regX;

    testcpu_set_opcode2(0xf5, arg0);

    apple_ii_64k[0][idx] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == idx);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xf5);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_SBC_abs(uint8_t regA, uint8_t val, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode3(0xed, lobyte, hibyte);

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0xf4;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0xf4);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xed);
    ASSERT(cpu65_debug.opcycles  == (4));

    PASS();
}

TEST test_SBC_abs_x(uint8_t regA, uint8_t val, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode3(0xfd, lobyte, hibyte);

    uint8_t cycle_count = 0;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regX;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x05;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x05);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xfd);

    cycle_count += 4;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_SBC_abs_y(uint8_t regA, uint8_t val, uint8_t regY, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode3(0xf9, lobyte, hibyte);

    uint8_t cycle_count = 0;
    uint16_t addrs = lobyte | (hibyte<<8);
    addrs = addrs + regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)hibyte) {
        ++cycle_count;
    }
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x02;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+3);
    ASSERT(cpu65_current.x       == 0x02);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xf9);

    cycle_count += 4;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_SBC_ind_x(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regX, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode2(0xe1, arg0);

    uint8_t idx_lo = arg0 + regX;
    uint8_t idx_hi = idx_lo+1;
    uint16_t addrs = lobyte | (hibyte<<8);

    apple_ii_64k[0][idx_lo] = lobyte;
    apple_ii_64k[0][idx_hi] = hibyte;
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = regX;
    cpu65_current.y  = 0x15;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == regX);
    ASSERT(cpu65_current.y       == 0x15);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xe1);

    ASSERT(cpu65_debug.opcycles  == (6));

    PASS();
}

TEST test_SBC_ind_y(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t regY, uint8_t val_zp0, uint8_t val_zp1) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode2(0xf1, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = val_zp0;
    apple_ii_64k[0][idx1] = val_zp1;

    uint8_t cycle_count = 0;
    uint16_t addrs = val_zp0 | (val_zp1<<8);
    addrs += (uint8_t)regY;
    if ((uint8_t)((addrs>>8)&0xff) != (uint8_t)val_zp1) {
        ++cycle_count;
    }

    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x84;
    cpu65_current.y  = regY;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x84);
    ASSERT(cpu65_current.y       == regY);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xf1);
    cycle_count += 5;
    ASSERT(cpu65_debug.opcycles == cycle_count);

    PASS();
}

TEST test_SBC_ind_zpage(uint8_t regA, uint8_t val, uint8_t arg0, uint8_t lobyte, uint8_t hibyte) {
    HEADER0();

    logic_SBC(regA, val, &result, &flags);

    testcpu_set_opcode2(0xf2, arg0);

    uint8_t idx0 = arg0;
    uint8_t idx1 = arg0+1;

    apple_ii_64k[0][idx0] = lobyte;
    apple_ii_64k[0][idx1] = hibyte;

    uint16_t addrs = lobyte | (hibyte<<8);
    apple_ii_64k[0][addrs] = val;

    cpu65_current.a  = regA;
    cpu65_current.x  = 0x14;
    cpu65_current.y  = 0x85;
    cpu65_current.sp = 0x81;

    cpu65_run();

    ASSERT(cpu65_current.pc      == TEST_LOC+2);
    ASSERT(cpu65_current.x       == 0x14);
    ASSERT(cpu65_current.y       == 0x85);
    ASSERT(cpu65_current.sp      == 0x81);

    ASSERT(cpu65_current.a == result); 
    VERIFY_FLAGS();

    ASSERT(cpu65_debug.ea        == addrs);
    ASSERT(cpu65_debug.d         == 0xff);
    ASSERT(cpu65_debug.rw        == RW_READ);
    ASSERT(cpu65_debug.opcode    == 0xf2);
    ASSERT(cpu65_debug.opcycles  == (5));

    PASS();
}

// ----------------------------------------------------------------------------
// Test Suite

static unsigned int testcounter = 0;

typedef int(*test_func_ptr0)(void);
typedef int(*test_func_ptr)(uint8_t x, ...);

typedef struct test_func_t {
    unsigned int id;
    char *name;
    void *func;
    UT_hash_handle hh;
} test_func_t;
static test_func_t *test_funcs = NULL;

#define A2_ADD_TEST(TEST) \
    do { \
        test_func_t *test_func = malloc(sizeof(test_func_t)); \
        test_func->id = testcounter; \
        test_func->name = strdup(#TEST); \
        test_func->func = TEST; \
        HASH_ADD_INT(test_funcs, id, test_func); \
        ++testcounter; \
    } while(0);

#define A2_REMOVE_TEST(TEST) \
    do { \
        HASH_DEL(test_funcs, TEST); \
        free(TEST->name); \
        free(TEST); \
    } while(0);

#define A2_RUN_TESTp(TEST, ...) RUN_TESTp( ((test_func_ptr)(TEST)), __VA_ARGS__)

GREATEST_SUITE(test_suite_cpu) {

    GREATEST_SET_SETUP_CB(testcpu_setup, NULL);
    GREATEST_SET_TEARDOWN_CB(testcpu_teardown, NULL);
    GREATEST_SET_BREAKPOINT_CB(testcpu_breakpoint, NULL);

    load_settings();
    sound_volume = 0;
    do_logging = false;// silence regular emulator logging

    c_initialize_firsttime();

    test_func_t *func=NULL, *tmp=NULL;

    // --------------------------------
    A2_ADD_TEST(test_BRK);
    A2_ADD_TEST(test_IRQ);
    A2_ADD_TEST(test_CLC);
    A2_ADD_TEST(test_CLD);
    A2_ADD_TEST(test_CLI);
    A2_ADD_TEST(test_CLV);
    A2_ADD_TEST(test_NOP);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);
        RUN_TEST(((test_func_ptr0)(func->func)));
        A2_REMOVE_TEST(func);
    }

    // ------------------------------------------------------------------------
    // Branch tests :
    // NOTE : these should be a comprehensive exercise of the branching logic

    greatest_info.flags = GREATEST_FLAG_SILENT_SUCCESS;
    A2_ADD_TEST(test_BCC);
    A2_ADD_TEST(test_BCS);
    A2_ADD_TEST(test_BEQ);
    A2_ADD_TEST(test_BNE);
    A2_ADD_TEST(test_BMI);
    A2_ADD_TEST(test_BPL);
    A2_ADD_TEST(test_BRA);
    A2_ADD_TEST(test_BVC);
    A2_ADD_TEST(test_BVS);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s (SILENCED OUTPUT) :\n", func->name);

        // test comprehensive logic in immediate mode (since no addressing to test) ...
        for (uint16_t addrs = 0x1f02; addrs < 0x2000; addrs+=0x80) {
            for (uint8_t flag = 0x00; flag < 0x02; flag++) {
                uint8_t off=0x00;
                do {
                    A2_RUN_TESTp( func->func, off, flag, addrs);
                } while (++off);
            }
        }

        fprintf(GREATEST_STDOUT, "...OK\n");
        A2_REMOVE_TEST(func);
    }
    greatest_info.flags = 0x0;

    // ------------------------------------------------------------------------
    // Immediate addressing mode tests :
    // NOTE : these should be a comprehensive exercise of the instruction logic

    greatest_info.flags = GREATEST_FLAG_SILENT_SUCCESS;
    A2_ADD_TEST(test_ADC_imm);
    A2_ADD_TEST(test_AND_imm);
    A2_ADD_TEST(test_BIT_imm);
    A2_ADD_TEST(test_CMP_imm);
    A2_ADD_TEST(test_CPX_imm);
    A2_ADD_TEST(test_CPY_imm);
    A2_ADD_TEST(test_DEA);
    A2_ADD_TEST(test_DEX);
    A2_ADD_TEST(test_DEY);
    A2_ADD_TEST(test_EOR_imm);
    A2_ADD_TEST(test_INA);
    A2_ADD_TEST(test_INX);
    A2_ADD_TEST(test_INY);
    A2_ADD_TEST(test_SBC_imm);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s (SILENCED OUTPUT) :\n", func->name);

        // test comprehensive logic in immediate mode (since no addressing to test) ...
        uint8_t regA=0x00;
        do {
            uint8_t val=0x00;
            do {
                A2_RUN_TESTp( func->func, regA, val, /*decimal*/false, /*carry*/false);
                A2_RUN_TESTp( func->func, regA, val, /*decimal*/ true, /*carry*/false);
                A2_RUN_TESTp( func->func, regA, val, /*decimal*/false, /*carry*/true);
                A2_RUN_TESTp( func->func, regA, val, /*decimal*/ true, /*carry*/true);
            } while (++val);
        } while (++regA);

        fprintf(GREATEST_STDOUT, "...OK\n");
        A2_REMOVE_TEST(func);
    }
    greatest_info.flags = 0x0;

    // ------------------------------------------------------------------------
    // Accumulator addressing mode tests :
    // NOTE : these should be a comprehensive exercise of the instruction logic

    greatest_info.flags = GREATEST_FLAG_SILENT_SUCCESS;
    A2_ADD_TEST(test_ASL_acc);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s (SILENCED OUTPUT) :\n", func->name);

        // test comprehensive logic in immediate mode (since no addressing to test) ...
        uint8_t regA=0x00;
        do {
            A2_RUN_TESTp( func->func, regA);
        } while (++regA);

        fprintf(GREATEST_STDOUT, "...OK\n");
        A2_REMOVE_TEST(func);
    }
    greatest_info.flags = 0x0;

    // ------------------------------------------------------------------------
    // Other addressing modes tests :
    // NOTE : unlike immediate-mode addressing tests above, these tests are not designed to be a comprehensive test of
    // instruction logic.  Rather--for clarity--they are designed to comprehensively test the addressing logic,
    // including all edge cases 

    // --------------------------------
    A2_ADD_TEST(test_ADC_zpage);
    A2_ADD_TEST(test_AND_zpage);
    A2_ADD_TEST(test_ASL_zpage);
    A2_ADD_TEST(test_BIT_zpage);
    A2_ADD_TEST(test_CMP_zpage);
    A2_ADD_TEST(test_CPX_zpage);
    A2_ADD_TEST(test_CPY_zpage);
    A2_ADD_TEST(test_DEC_zpage);
    A2_ADD_TEST(test_EOR_zpage);
    A2_ADD_TEST(test_INC_zpage);
    A2_ADD_TEST(test_SBC_zpage);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        uint8_t arg0 = 0x00;
        do {
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, arg0);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, arg0);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, arg0);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, arg0);
            ++arg0;
        } while (arg0);

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_zpage_x);
    A2_ADD_TEST(test_AND_zpage_x);
    A2_ADD_TEST(test_ASL_zpage_x);
    A2_ADD_TEST(test_BIT_zpage_x);
    A2_ADD_TEST(test_CMP_zpage_x);
    A2_ADD_TEST(test_DEC_zpage_x);
    A2_ADD_TEST(test_EOR_zpage_x);
    A2_ADD_TEST(test_INC_zpage_x);
    A2_ADD_TEST(test_SBC_zpage_x);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        for (uint8_t regX=0x42; regX>0x3F; regX+=0x40) {
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, /*arg0*/0x24, regX);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, /*arg0*/0x24, regX);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, /*arg0*/0x24, regX);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, /*arg0*/0x24, regX);
        }

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_abs);
    A2_ADD_TEST(test_AND_abs);
    A2_ADD_TEST(test_ASL_abs);
    A2_ADD_TEST(test_BIT_abs);
    A2_ADD_TEST(test_CMP_abs);
    A2_ADD_TEST(test_CPX_abs);
    A2_ADD_TEST(test_CPY_abs);
    A2_ADD_TEST(test_DEC_abs);
    A2_ADD_TEST(test_EOR_abs);
    A2_ADD_TEST(test_INC_abs);
    A2_ADD_TEST(test_SBC_abs);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        for (uint8_t lobyte=0xfd; lobyte>0xf0; lobyte++) {
            uint8_t hibyte = 0x1f;
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, lobyte, hibyte);
        }

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_abs_x);
    A2_ADD_TEST(test_AND_abs_x);
    A2_ADD_TEST(test_ASL_abs_x);
    A2_ADD_TEST(test_BIT_abs_x);
    A2_ADD_TEST(test_CMP_abs_x);
    A2_ADD_TEST(test_DEC_abs_x);
    A2_ADD_TEST(test_EOR_abs_x);
    A2_ADD_TEST(test_INC_abs_x);
    A2_ADD_TEST(test_SBC_abs_x);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        uint8_t hibyte = 0x1f;
        uint8_t lobyte = 0x20;
        for (uint8_t regX=0x50; regX>0x4f; regX+=0x30) {
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, regX, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, regX, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, regX, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, regX, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x24, /*val*/0x42, 0x20, 0xfe,   0xff); // wrap to zpage
        }

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_abs_y);
    A2_ADD_TEST(test_AND_abs_y);
    A2_ADD_TEST(test_CMP_abs_y);
    A2_ADD_TEST(test_EOR_abs_y);
    A2_ADD_TEST(test_SBC_abs_y);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        uint8_t hibyte = 0x1f;
        uint8_t lobyte = 0x20;
        for (uint8_t regY=0x50; regY>0x4f; regY+=0x30) {
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, regY, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, regY, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, regY, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, regY, lobyte, hibyte);
            A2_RUN_TESTp( func->func, /*A*/0x24, /*val*/0x42, 0x20, 0xfe,   0xff); // wrap to zpage
        }

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_ind_x);
    A2_ADD_TEST(test_AND_ind_x);
    A2_ADD_TEST(test_CMP_ind_x);
    A2_ADD_TEST(test_EOR_ind_x);
    A2_ADD_TEST(test_SBC_ind_x);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        uint8_t hibyte = 0x1f;
        for (uint8_t lobyte=0xfd; lobyte>0xf0; lobyte++) {
            for (uint8_t regX=0x42; regX>0x3F; regX+=0x40) {
                A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, /*arg0*/0x24, regX, lobyte, hibyte);
                A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, /*arg0*/0x24, regX, lobyte, hibyte);
                A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, /*arg0*/0x24, regX, lobyte, hibyte);
                A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, /*arg0*/0x24, regX, lobyte, hibyte);
            }
        }

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_ind_y);
    A2_ADD_TEST(test_AND_ind_y);
    A2_ADD_TEST(test_CMP_ind_y);
    A2_ADD_TEST(test_EOR_ind_y);
    A2_ADD_TEST(test_SBC_ind_y);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, /*arg0*/0x24, /*regY*/0x10, /*val_zp0*/0x22, /*val_zp1*/0x1f);
        A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, /*arg0*/0x24, /*regY*/0x80, /*val_zp0*/0x80, /*val_zp1*/0x1f);
        A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, /*arg0*/0x24, /*regY*/0xAA, /*val_zp0*/0xAA, /*val_zp1*/0x1f);
        A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, /*arg0*/0x24, /*regY*/0x80, /*val_zp0*/0x90, /*val_zp1*/0xff);

        A2_REMOVE_TEST(func);
    }

    // --------------------------------
    A2_ADD_TEST(test_ADC_ind_zpage);
    A2_ADD_TEST(test_AND_ind_zpage);
    A2_ADD_TEST(test_CMP_ind_zpage);
    A2_ADD_TEST(test_EOR_ind_zpage);
    A2_ADD_TEST(test_SBC_ind_zpage);
    HASH_ITER(hh, test_funcs, func, tmp) {
        fprintf(GREATEST_STDOUT, "\n%s :\n", func->name);

        // test addressing is working ...
        for (uint8_t lobyte=0xfd; lobyte>0xf0; lobyte++) {
            uint8_t hibyte = 0x1f;
            A2_RUN_TESTp( func->func, /*A*/0x0f, /*val*/0x0f, /*arg0*/0x00, /*lobyte*/0x33, /*hibyte*/0x1f);
            A2_RUN_TESTp( func->func, /*A*/0x7f, /*val*/0x7f, /*arg0*/0x7f, /*lobyte*/0x33, /*hibyte*/0x1f);
            A2_RUN_TESTp( func->func, /*A*/0xaa, /*val*/0x55, /*arg0*/0xAB, /*lobyte*/0x33, /*hibyte*/0x1f);
            A2_RUN_TESTp( func->func, /*A*/0x00, /*val*/0xff, /*arg0*/0xff, /*lobyte*/0x33, /*hibyte*/0x1f);
        }

        A2_REMOVE_TEST(func);
    }
}
