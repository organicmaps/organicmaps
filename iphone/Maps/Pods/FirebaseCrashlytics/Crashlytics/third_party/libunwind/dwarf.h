/* libunwind - a platform-independent unwind library
   Copyright (c) 2003-2005 Hewlett-Packard Development Company, L.P.
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#pragma once

//
#define DWARF_EXTENDED_LENGTH_FLAG (0xffffffff)
#define DWARF_CIE_ID_CIE_FLAG (0)

// Exception Handling Pointer Encoding constants
#define DW_EH_PE_VALUE_MASK (0x0F)
#define DW_EH_PE_RELATIVE_OFFSET_MASK (0x70)

// Register Definitions
#define DW_EN_MAX_REGISTER_NUMBER (120)

enum {
  DW_EH_PE_ptr = 0x00,
  DW_EH_PE_uleb128 = 0x01,
  DW_EH_PE_udata2 = 0x02,
  DW_EH_PE_udata4 = 0x03,
  DW_EH_PE_udata8 = 0x04,
  DW_EH_PE_signed = 0x08,
  DW_EH_PE_sleb128 = 0x09,
  DW_EH_PE_sdata2 = 0x0A,
  DW_EH_PE_sdata4 = 0x0B,
  DW_EH_PE_sdata8 = 0x0C,

  DW_EH_PE_absptr = 0x00,
  DW_EH_PE_pcrel = 0x10,
  DW_EH_PE_textrel = 0x20,
  DW_EH_PE_datarel = 0x30,
  DW_EH_PE_funcrel = 0x40,
  DW_EH_PE_aligned = 0x50,
  DW_EH_PE_indirect = 0x80,
  DW_EH_PE_omit = 0xFF
};

// Unwind Instructions

#define DW_CFA_OPCODE_MASK (0xC0)
#define DW_CFA_OPERAND_MASK (0x3F)

enum {
  DW_CFA_nop = 0x0,
  DW_CFA_set_loc = 0x1,
  DW_CFA_advance_loc1 = 0x2,
  DW_CFA_advance_loc2 = 0x3,
  DW_CFA_advance_loc4 = 0x4,
  DW_CFA_offset_extended = 0x5,
  DW_CFA_restore_extended = 0x6,
  DW_CFA_undefined = 0x7,
  DW_CFA_same_value = 0x8,
  DW_CFA_register = 0x9,
  DW_CFA_remember_state = 0xA,
  DW_CFA_restore_state = 0xB,
  DW_CFA_def_cfa = 0xC,
  DW_CFA_def_cfa_register = 0xD,
  DW_CFA_def_cfa_offset = 0xE,
  DW_CFA_def_cfa_expression = 0xF,
  DW_CFA_expression = 0x10,
  DW_CFA_offset_extended_sf = 0x11,
  DW_CFA_def_cfa_sf = 0x12,
  DW_CFA_def_cfa_offset_sf = 0x13,
  DW_CFA_val_offset = 0x14,
  DW_CFA_val_offset_sf = 0x15,
  DW_CFA_val_expression = 0x16,

  // opcode is in high 2 bits, operand in is lower 6 bits
  DW_CFA_advance_loc = 0x40,  // operand is delta
  DW_CFA_offset = 0x80,       // operand is register
  DW_CFA_restore = 0xC0,      // operand is register

  // GNU extensions
  DW_CFA_GNU_window_save = 0x2D,
  DW_CFA_GNU_args_size = 0x2E,
  DW_CFA_GNU_negative_offset_extended = 0x2F
};

// Expression Instructions
enum {
  DW_OP_addr = 0x03,
  DW_OP_deref = 0x06,
  DW_OP_const1u = 0x08,
  DW_OP_const1s = 0x09,
  DW_OP_const2u = 0x0A,
  DW_OP_const2s = 0x0B,
  DW_OP_const4u = 0x0C,
  DW_OP_const4s = 0x0D,
  DW_OP_const8u = 0x0E,
  DW_OP_const8s = 0x0F,
  DW_OP_constu = 0x10,
  DW_OP_consts = 0x11,
  DW_OP_dup = 0x12,
  DW_OP_drop = 0x13,
  DW_OP_over = 0x14,
  DW_OP_pick = 0x15,
  DW_OP_swap = 0x16,
  DW_OP_rot = 0x17,
  DW_OP_xderef = 0x18,
  DW_OP_abs = 0x19,
  DW_OP_and = 0x1A,
  DW_OP_div = 0x1B,
  DW_OP_minus = 0x1C,
  DW_OP_mod = 0x1D,
  DW_OP_mul = 0x1E,
  DW_OP_neg = 0x1F,
  DW_OP_not = 0x20,
  DW_OP_or = 0x21,
  DW_OP_plus = 0x22,
  DW_OP_plus_uconst = 0x23,
  DW_OP_shl = 0x24,
  DW_OP_shr = 0x25,
  DW_OP_shra = 0x26,
  DW_OP_xor = 0x27,
  DW_OP_skip = 0x2F,
  DW_OP_bra = 0x28,
  DW_OP_eq = 0x29,
  DW_OP_ge = 0x2A,
  DW_OP_gt = 0x2B,
  DW_OP_le = 0x2C,
  DW_OP_lt = 0x2D,
  DW_OP_ne = 0x2E,
  DW_OP_lit0 = 0x30,
  DW_OP_lit1 = 0x31,
  DW_OP_lit2 = 0x32,
  DW_OP_lit3 = 0x33,
  DW_OP_lit4 = 0x34,
  DW_OP_lit5 = 0x35,
  DW_OP_lit6 = 0x36,
  DW_OP_lit7 = 0x37,
  DW_OP_lit8 = 0x38,
  DW_OP_lit9 = 0x39,
  DW_OP_lit10 = 0x3A,
  DW_OP_lit11 = 0x3B,
  DW_OP_lit12 = 0x3C,
  DW_OP_lit13 = 0x3D,
  DW_OP_lit14 = 0x3E,
  DW_OP_lit15 = 0x3F,
  DW_OP_lit16 = 0x40,
  DW_OP_lit17 = 0x41,
  DW_OP_lit18 = 0x42,
  DW_OP_lit19 = 0x43,
  DW_OP_lit20 = 0x44,
  DW_OP_lit21 = 0x45,
  DW_OP_lit22 = 0x46,
  DW_OP_lit23 = 0x47,
  DW_OP_lit24 = 0x48,
  DW_OP_lit25 = 0x49,
  DW_OP_lit26 = 0x4A,
  DW_OP_lit27 = 0x4B,
  DW_OP_lit28 = 0x4C,
  DW_OP_lit29 = 0x4D,
  DW_OP_lit30 = 0x4E,
  DW_OP_lit31 = 0x4F,
  DW_OP_reg0 = 0x50,
  DW_OP_reg1 = 0x51,
  DW_OP_reg2 = 0x52,
  DW_OP_reg3 = 0x53,
  DW_OP_reg4 = 0x54,
  DW_OP_reg5 = 0x55,
  DW_OP_reg6 = 0x56,
  DW_OP_reg7 = 0x57,
  DW_OP_reg8 = 0x58,
  DW_OP_reg9 = 0x59,
  DW_OP_reg10 = 0x5A,
  DW_OP_reg11 = 0x5B,
  DW_OP_reg12 = 0x5C,
  DW_OP_reg13 = 0x5D,
  DW_OP_reg14 = 0x5E,
  DW_OP_reg15 = 0x5F,
  DW_OP_reg16 = 0x60,
  DW_OP_reg17 = 0x61,
  DW_OP_reg18 = 0x62,
  DW_OP_reg19 = 0x63,
  DW_OP_reg20 = 0x64,
  DW_OP_reg21 = 0x65,
  DW_OP_reg22 = 0x66,
  DW_OP_reg23 = 0x67,
  DW_OP_reg24 = 0x68,
  DW_OP_reg25 = 0x69,
  DW_OP_reg26 = 0x6A,
  DW_OP_reg27 = 0x6B,
  DW_OP_reg28 = 0x6C,
  DW_OP_reg29 = 0x6D,
  DW_OP_reg30 = 0x6E,
  DW_OP_reg31 = 0x6F,
  DW_OP_breg0 = 0x70,
  DW_OP_breg1 = 0x71,
  DW_OP_breg2 = 0x72,
  DW_OP_breg3 = 0x73,
  DW_OP_breg4 = 0x74,
  DW_OP_breg5 = 0x75,
  DW_OP_breg6 = 0x76,
  DW_OP_breg7 = 0x77,
  DW_OP_breg8 = 0x78,
  DW_OP_breg9 = 0x79,
  DW_OP_breg10 = 0x7A,
  DW_OP_breg11 = 0x7B,
  DW_OP_breg12 = 0x7C,
  DW_OP_breg13 = 0x7D,
  DW_OP_breg14 = 0x7E,
  DW_OP_breg15 = 0x7F,
  DW_OP_breg16 = 0x80,
  DW_OP_breg17 = 0x81,
  DW_OP_breg18 = 0x82,
  DW_OP_breg19 = 0x83,
  DW_OP_breg20 = 0x84,
  DW_OP_breg21 = 0x85,
  DW_OP_breg22 = 0x86,
  DW_OP_breg23 = 0x87,
  DW_OP_breg24 = 0x88,
  DW_OP_breg25 = 0x89,
  DW_OP_breg26 = 0x8A,
  DW_OP_breg27 = 0x8B,
  DW_OP_breg28 = 0x8C,
  DW_OP_breg29 = 0x8D,
  DW_OP_breg30 = 0x8E,
  DW_OP_breg31 = 0x8F,
  DW_OP_regx = 0x90,
  DW_OP_fbreg = 0x91,
  DW_OP_bregx = 0x92,
  DW_OP_piece = 0x93,
  DW_OP_deref_size = 0x94,
  DW_OP_xderef_size = 0x95,
  DW_OP_nop = 0x96,
  DW_OP_push_object_addres = 0x97,
  DW_OP_call2 = 0x98,
  DW_OP_call4 = 0x99,
  DW_OP_call_ref = 0x9A,
  DW_OP_lo_user = 0xE0,
  DW_OP_APPLE_uninit = 0xF0,
  DW_OP_hi_user = 0xFF
};
