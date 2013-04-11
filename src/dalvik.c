/*
Copyright (C) 2010 Mark Gordon

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "dalvik.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
dx_int get_int(dx_ushort* buf, int ind) {
  dx_uint res = buf[ind + 1];
  res = res << 16;
  res |= buf[ind];
  return (dx_int)res;
}

int dxc_read_dalvik(read_context* ctx, dx_uint size, dx_uint off,
                    DexInstruction** insns, dx_uint* count) {
  DexInstruction* res;
  if(!(res = *insns = (DexInstruction*)calloc(size, sizeof(DexInstruction)))) {
    DXC_ERROR("dalvik alloc failed");
    return 0;
  }
  *count = 0;

  dx_ushort* buf = (dx_ushort*)(ctx->buf + off);

  dx_uint i;
  for(i = 0; i < size; res++) {
    (*count)++;
    res->opcode = (DexOpCode)(buf[i] & 0x00FF);
    res->hi_byte = (DexPsuedoOpCode)(buf[i] >> 8);
    if(res->opcode == OP_PSUEDO &&
       res->hi_byte != PSUEDO_OP_NOP) {
      /* Handle the special psuedo opcode tables. */
      if(i + 2 > size) {
        DXC_ERROR("instruction table leaves code boundaries");
        return 0;
      }
      dx_ushort sz = buf[i + 1];
      switch(res->hi_byte) {
        case PSUEDO_OP_PACKED_SWITCH: {
          if(i + sz * 2 + 4 > size) {
            DXC_ERROR("instruction table leaves code boundaries");
            return 0;
          }
          res->special.packed_switch.size = sz;
          res->special.packed_switch.first_key = get_int(buf, i + 2);
          res->special.packed_switch.targets =
              (dx_int*)malloc(sizeof(dx_int) * sz);
          int j;
          for(j = 0; j < sz; j++) {
            res->special.packed_switch.targets[j] = get_int(buf, i + 2 * j + 4);
          }
          i += sz * 2 + 4;
          break;
        } case PSUEDO_OP_SPARSE_SWITCH: {
          if(i + sz * 4 + 2 > size) {
            DXC_ERROR("instruction table leaves code boundaries");
            return 0;
          }
          res->special.sparse_switch.size = sz;
          res->special.sparse_switch.keys =
              (dx_int*)malloc(sizeof(dx_int) * sz);
          res->special.sparse_switch.targets =
              (dx_int*)malloc(sizeof(dx_int) * sz);
          int j;
          for(j = 0; j < sz; j++) {
            res->special.sparse_switch.keys[j] = get_int(buf, i + 2 * j + 2);
          }
          for(j = 0; j < sz; j++) {
            res->special.sparse_switch.targets[j] = get_int(buf,
                                                      i + 2 * (sz + j) + 2);
          }
          i += sz * 4 + 2;
          break;
        } case PSUEDO_OP_FILL_DATA_ARRAY:
          if(i + 4 > size) {
            DXC_ERROR("instruction table leaves code boundaries");
            return 0;
          }
          res->special.fill_data_array.element_width = sz;
          res->special.fill_data_array.size = get_int(buf, i + 2);
          if(i + (sz * res->special.fill_data_array.size + 1) / 2 + 4 > size) {
            DXC_ERROR("instruction table leaves code boundaries");
            return 0;
          }
          res->special.fill_data_array.data =
              (dx_ubyte*)malloc(sz * res->special.fill_data_array.size);
          memcpy(res->special.fill_data_array.data, buf + i + 4,
                 sz * res->special.fill_data_array.size);
          i += (sz * res->special.fill_data_array.size + 1) / 2 + 4;
          break;
        default:
          DXC_ERROR("unrecognized psuedo opcode");
          return 0;
      }
    } else {
      /* Handle the normal opcodes. */
      const DexOpFormat* fmt = &dex_opcode_formats[res->opcode];
      if(fmt->name == NULL) {
        DXC_ERROR("unrecognized opcode");
        return 0;
      }
      if(i + fmt->size > size) {
        DXC_ERROR("instruction leaves code boundaries");
        return 0;
      }
      res->param[0] = buf[i + 1];
      res->param[1] = buf[i + 2];
      if(fmt->specialType != SPECIAL_NONE) {
        int pos = fmt->specialPos;
        int size = fmt->specialSize;
        dx_ulong v = 0;
        switch(pos) {
          case 0:
            switch(size) {
              case 1:
                v = buf[i] >> 12UL;
                break;
              case 2:
                v = buf[i] >> 8UL;
                break;
              default:
                DXC_ERROR("unhandled special alignment");
                return 0;
            }
            break;
          case 4:
            switch(size) {
              case 16:
                v |= ((dx_ulong)buf[i + 4]) << 48UL;
                v |= ((dx_ulong)buf[i + 3]) << 32UL;
              case 8:
                v |= ((dx_ulong)buf[i + 2]) << 16UL;
              case 4:
                v |= buf[i + 1];
                break;
              case 2:
                v = buf[i + 1] >> 8UL;
                break;
              default:
                DXC_ERROR("unhandled special alignment");
                return 0;
            }
            break;
          default:
            DXC_ERROR("unhandled special alignment");
            return 0;
        }
        if((fmt->specialType == SPECIAL_CONSTANT ||
            fmt->specialType == SPECIAL_TARGET) && size < 16) {
          // These two types both need to be sign extended.
          if(v & (1ULL << (size * 4 - 1))) {
            v |= ~((1ULL << size * 4) - 1);
          }
        }
        switch(fmt->specialType) {
          case SPECIAL_CONSTANT:
            res->special.constant = v;
            break;
          case SPECIAL_TARGET:
            res->special.target = v;
            break;
          case SPECIAL_STRING:
            if(v >= ctx->strs_sz) {
              DXC_ERROR("invalid string index in bytecode");
              return 0;
            }
            res->special.str = dxc_copy_str(ctx->strs[v]);
            break;
          case SPECIAL_TYPE:
            if(v >= ctx->types_sz) {
              DXC_ERROR("invalid type index in bytecode");
              return 0;
            }
            res->special.type = dxc_copy_str(ctx->types[v]);
            break;
          case SPECIAL_FIELD:
            if(v >= ctx->fields_sz) {
              DXC_ERROR("invalid field index in bytecode");
              return 0;
            }
            res->special.field.defining_class =
                dxc_copy_str(ctx->fields[v].defining_class);
            res->special.field.name =
                dxc_copy_str(ctx->fields[v].name);
            res->special.field.type =
                dxc_copy_str(ctx->fields[v].type);
            break;
          case SPECIAL_METHOD:
            if(v >= ctx->methods_sz) {
              DXC_ERROR("invalid method index in bytecode");
              return 0;
            }
            res->special.method.defining_class =
                dxc_copy_str(ctx->methods[v].defining_class);
            res->special.method.name =
                dxc_copy_str(ctx->methods[v].name);
            res->special.method.prototype =
                dxc_copy_strstr(ctx->methods[v].prototype);
            break;
          case SPECIAL_INLINE:
            res->special.inline_ind = v;
            break;
          case SPECIAL_OBJECT:
            res->special.object_off = v;
            break;
          case SPECIAL_VTABLE:
            res->special.vtable_ind = v;
            break;
          case SPECIAL_NONE:
            break;
        }
      }
      i += fmt->size;
    }
  }
  if((res = (DexInstruction*)malloc((*count) * sizeof(DexInstruction)))) {
    memcpy(res, *insns, (*count) * sizeof(DexInstruction));
    free(*insns);
    *insns = res;
  }
  return 1;
}

void dxc_free_instruction(DexInstruction* insn) {
  if(!insn) return;
  if(insn->opcode == OP_PSUEDO &&
     insn->hi_byte != PSUEDO_OP_NOP) {
    switch(insn->hi_byte) {
      case PSUEDO_OP_PACKED_SWITCH:
        free(insn->special.packed_switch.targets);
        break;
      case PSUEDO_OP_SPARSE_SWITCH:
        free(insn->special.sparse_switch.keys);
        free(insn->special.sparse_switch.targets);
        break;
      case PSUEDO_OP_FILL_DATA_ARRAY:
        free(insn->special.fill_data_array.data);
        break;
    }
  } else switch(dex_opcode_formats[insn->opcode].specialType) {
    case SPECIAL_STRING:
      dxc_free_str(insn->special.str);
      break;
    case SPECIAL_TYPE:
      dxc_free_str(insn->special.type);
      break;
    case SPECIAL_FIELD:
      dxc_free_str(insn->special.field.defining_class);
      dxc_free_str(insn->special.field.name);
      dxc_free_str(insn->special.field.type);
      break;
    case SPECIAL_METHOD:
      dxc_free_str(insn->special.method.defining_class);
      dxc_free_str(insn->special.method.name);
      dxc_free_strstr(insn->special.method.prototype);
      break;
    case SPECIAL_NONE:
    case SPECIAL_CONSTANT:
    case SPECIAL_TARGET:
    case SPECIAL_INLINE:
    case SPECIAL_OBJECT:
    case SPECIAL_VTABLE:
      break;
  }
}

dx_uint dxc_insn_width(const DexInstruction* insn) {
  if(insn->opcode == OP_PSUEDO && insn->hi_byte != PSUEDO_OP_NOP) {
    switch(insn->hi_byte) {
      case PSUEDO_OP_PACKED_SWITCH:
        return 4 + 2 * insn->special.packed_switch.size;
      case PSUEDO_OP_SPARSE_SWITCH:
        return 2 + 4 * insn->special.sparse_switch.size;
      case PSUEDO_OP_FILL_DATA_ARRAY:
        return 4 + (insn->special.fill_data_array.element_width *
                    insn->special.fill_data_array.size + 1) / 2;
    }
  }
  return dex_opcode_formats[insn->opcode].size;
}

dx_ubyte dxc_num_registers(const DexInstruction* insn) {
  DexOpFormat fmt = dex_opcode_formats[insn->opcode];
  if(fmt.format_id[0] == 'r') {
    return insn->hi_byte;
  } else if(fmt.format_id[0] == '5') {
    return insn->hi_byte >> 4;
  } else {
    return fmt.format_id[0] - '0';
  }
}

dx_int dxc_set_num_registers(DexInstruction* insn, dx_ubyte regs) {
  DexOpFormat fmt = dex_opcode_formats[insn->opcode];
  dx_int ret = dxc_num_registers(insn);
  if(fmt.format_id[0] == 'r') {
    insn->hi_byte = regs;
  } else if(fmt.format_id[0] == '5') {
    if(regs > 5) return -1;
    insn->hi_byte = (insn->hi_byte & 0x0F) | (regs << 4);
  } else if(regs != fmt.format_id[0] - '0') {
    return -1;
  }
  return ret;
}

dx_int dxc_get_register(const DexInstruction* insn, dx_uint index) {
  if(index >= dxc_num_registers(insn)) {
    return -1;
  }
  DexOpFormat fmt = dex_opcode_formats[insn->opcode];
  if(fmt.format_id[0] == 'r') {
    return insn->param[1] + index;
  } else switch(fmt.format_id[0]) {
    case '1':
      switch(fmt.format_id[1]) {
        case 'n':
          return insn->hi_byte & 0xF;
        case 'x':
        case 't':
        case 's':
        case 'h':
        case 'c':
        case 'i':
        case 'l':
          return insn->hi_byte;
      }
      break;
    case '2':
      switch(fmt.format_id[1]) {
        case 'x':
          if(fmt.size == 3) {
            return insn->param[index];
          } else if(fmt.size == 2) {
            return index ? insn->param[0] : insn->hi_byte;
          } // Fall through intentional when fmt.size is 1.
        case 't':
        case 's':
        case 'c':
          return insn->hi_byte >> (index << 2) & 0xF;
        case 'b':
          return index ? insn->param[0] & 0x00FF : insn->hi_byte;
      }
    case '3':
      if(fmt.format_id[1] == 'x') {
        if(index == 0) return insn->hi_byte;
        if(index == 1) return insn->param[0] & 0xFF;
        if(index == 2) return insn->param[0] >> 8;
      }
      break;
    case '5':
      if(index == 4) return insn->hi_byte & 0xF;
      return insn->param[1] >> (index << 2) & 0xF;
  }
  DXC_ERROR("Unexpected format identifier");
  return -1;
}

dx_int dxc_register_width(const DexInstruction* insn, dx_uint index) {
  if(index >= dxc_num_registers(insn)) {
    return -1;
  }
  DexOpFormat fmt = dex_opcode_formats[insn->opcode];
  if(fmt.format_id[0] == 'r') {
    return 4;
  } else switch(fmt.format_id[0]) {
    case '1':
      switch(fmt.format_id[1]) {
        case 'n':
          return 1;
        case 'x':
        case 't':
        case 's':
        case 'h':
        case 'c':
        case 'i':
        case 'l':
          return 2;
      }
      break;
    case '2':
      switch(fmt.format_id[1]) {
        case 'x':
          if(fmt.size == 3) {
            return 4;
          } else if(fmt.size == 2) {
            return index ? 4 : 2;
          } // Fall through intentional when fmt.size is 1.
        case 't':
        case 's':
        case 'c':
          return 1;
        case 'b':
          return 2;
      }
    case '3':
      if(fmt.format_id[1] == 'x') {
        return 2;
      }
      break;
    case '5':
      return 1;
  }
  DXC_ERROR("Unexpected format identifier");
  return -1;
}

dx_int dxc_set_register(DexInstruction* insn, dx_uint index, dx_ushort reg) {
  if(index >= dxc_num_registers(insn)) {
    return -1;
  }
  dx_int width = dxc_register_width(insn, index);
  if(width == -1) return -1;
  if(width < 4 && reg >= 1U << width * 4) return -1;
  dx_int ret = dxc_get_register(insn, index);
  if(ret == -1) return -1;
  DexOpFormat fmt = dex_opcode_formats[insn->opcode];
  if(fmt.format_id[0] == 'r') {
    if(index != 0) {
      return -1;
    }
    insn->param[1] = reg;
  } else switch(fmt.format_id[0]) {
    case '1':
      switch(fmt.format_id[1]) {
        case 'n':
          insn->hi_byte = (insn->hi_byte & 0xF0) | reg;
          break;
        case 'x':
        case 't':
        case 's':
        case 'h':
        case 'c':
        case 'i':
        case 'l':
          insn->hi_byte = reg;
          break;
      }
      break;
    case '2':
      switch(fmt.format_id[1]) {
        case 'x':
          if(fmt.size == 3) {
            insn->param[index] = reg;
            break;
          } else if(fmt.size == 2) {
            if(index) {
              insn->param[0] = reg;
            } else {
              insn->hi_byte = reg;
            }
            break;
          } // Fall through intentional when fmt.size is 1.
        case 't':
        case 's':
        case 'c':
          insn->hi_byte = ((0xFF ^ (0xF << (index << 2))) & insn->hi_byte) |
                          (reg << (index << 2));
          break;
        case 'b':
          if(index) {
            insn->param[0] = (insn->param[0] & 0x00FF) | reg;
          } else {
            insn->hi_byte = reg;
          }
      }
    case '3':
      if(fmt.format_id[1] == 'x') {
        if(index == 0) insn->hi_byte = reg;
        if(index == 1) insn->param[0] = (insn->param[0] & 0xFF00) | reg;
        if(index == 2) insn->param[0] = (insn->param[0] & 0x00FF) | (reg << 8);
      }
      break;
    case '5':
      if(index == 4) {
        insn->hi_byte = (insn->hi_byte & 0xF0) | reg;
      } else {
        insn->param[1] = ((0xFFFF ^ (0xF << (index << 2))) & insn->param[1]) |
                         (reg << (index << 2));
      }
      break;
  }
  return ret;
}

#define FA DEX_INSTR_FLAG_CONTINUE
#define FB (DEX_INSTR_FLAG_CONTINUE | DEX_INSTR_FLAG_THROW)
#define FC (DEX_INSTR_FLAG_CONTINUE | DEX_INSTR_FLAG_THROW | \
            DEX_INSTR_FLAG_INVOKE)
#define FD DEX_INSTR_FLAG_RETURN
#define FE DEX_INSTR_FLAG_THROW
#define FF DEX_INSTR_FLAG_BRANCH
#define FG (DEX_INSTR_FLAG_BRANCH | DEX_INSTR_FLAG_CONTINUE)
#define FH (DEX_INSTR_FLAG_SWITCH | DEX_INSTR_FLAG_CONTINUE)
#define WR DEX_INSTR_FLAG_WRITE_REG
#define WD1 DEX_INSTR_FLAG_WIDE_R1
#define WD2 DEX_INSTR_FLAG_WIDE_R2
#define WD12 (DEX_INSTR_FLAG_WIDE_R1 | DEX_INSTR_FLAG_WIDE_R2)
#define WD23 (DEX_INSTR_FLAG_WIDE_R2 | DEX_INSTR_FLAG_WIDE_R3)
#define WD123 (DEX_INSTR_FLAG_WIDE_R1 | DEX_INSTR_FLAG_WIDE_R2 | \
               DEX_INSTR_FLAG_WIDE_R3)

const DexOpFormat dex_opcode_formats[256] = {
/* 00 */ { "nop", "0x", 1, SPECIAL_NONE, 0, 0, FA},
/* 01 */ { "move", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 02 */ { "move/from16", "2x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 03 */ { "move/16", "2x", 3, SPECIAL_NONE, 0, 0, FA | WR},
/* 04 */ { "move-wide", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 05 */ { "move-wide/from16", "2x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 06 */ { "move-wide/16", "2x", 3, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 07 */ { "move-object", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 08 */ { "move-object/from16", "2x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 09 */ { "move-object/16", "2x", 3, SPECIAL_NONE, 0, 0, FA | WR},
/* 0A */ { "move-result", "1x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 0B */ { "move-result-wide", "1x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* 0C */ { "move-result-object", "1x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 0D */ { "move-exception", "1x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 0E */ { "return-void", "0x", 1, SPECIAL_NONE, 0, 0, FD},
/* 0F */ { "return", "1x", 1, SPECIAL_NONE, 0, 0, FD},
/* 10 */ { "return-wide", "1x", 1, SPECIAL_NONE, 0, 0, FD | WD1},
/* 11 */ { "return-object", "1x", 1, SPECIAL_NONE, 0, 0, FD},
/* 12 */ { "const/4", "1n", 1, SPECIAL_CONSTANT, 0, 1, FA | WR},
/* 13 */ { "const/16", "1s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* 14 */ { "const", "1i", 3, SPECIAL_CONSTANT, 4, 8, FA | WR},
/* 15 */ { "const/high16", "1h", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* 16 */ { "const-wide/16", "1s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR | WD1},
/* 17 */ { "const-wide/32", "1i", 3, SPECIAL_CONSTANT, 4, 8, FA | WR | WD1},
/* 18 */ { "const-wide", "1l", 5, SPECIAL_CONSTANT, 4, 16, FA | WR | WD1},
/* 19 */ { "const-wide/high16", "1h", 2, SPECIAL_CONSTANT, 4, 4, FA | WR | WD1},
/* 1A */ { "const-string", "1c", 2, SPECIAL_STRING, 4, 4, FB | WR},
/* 1B */ { "const-string/jumbo", "1c", 3, SPECIAL_STRING, 4, 8, FB | WR},
/* 1C */ { "const-class", "1c", 2, SPECIAL_TYPE, 4, 4, FB | WR},
/* 1D */ { "monitor-enter", "1x", 1, SPECIAL_NONE, 0, 0, FB},
/* 1E */ { "monitor-exit", "1x", 1, SPECIAL_NONE, 0, 0, FB},
/* 1F */ { "check-cast", "1c", 2, SPECIAL_TYPE, 4, 4, FB},
/* 20 */ { "instance-of", "2c", 2, SPECIAL_TYPE, 4, 4, FB | WR},
/* 21 */ { "array-length", "2x", 1, SPECIAL_NONE, 0, 0, FB | WR},
/* 22 */ { "new-instance", "1c", 2, SPECIAL_TYPE, 4, 4, FB | WR},
/* 23 */ { "new-array", "2c", 2, SPECIAL_TYPE, 4, 4, FB | WR},
/* 24 */ { "filled-new-array", "5c", 3, SPECIAL_TYPE, 4, 4, FB},
/* 25 */ { "filled-new-array/range", "rc", 3, SPECIAL_TYPE, 4, 4, FB},
/* 26 */ { "fill-array-data", "1t", 3, SPECIAL_TARGET, 4, 8, FA},
/* 27 */ { "throw", "1x", 1, SPECIAL_NONE, 0, 0, FE},
/* 28 */ { "goto", "0t", 1, SPECIAL_TARGET, 0, 2, FF},
/* 29 */ { "goto/16", "0t", 2, SPECIAL_TARGET, 4, 4, FF},
/* 2A */ { "goto/32", "0t", 3, SPECIAL_TARGET, 4, 8, FF},
/* 2B */ { "packed-switch", "1t", 3, SPECIAL_TARGET, 4, 8, FH},
/* 2C */ { "sparse-switch", "1t", 3, SPECIAL_TARGET, 4, 8, FH},
/* 2D */ { "cmpl-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 2E */ { "cmpg-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 2F */ { "cmpl-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD23},
/* 30 */ { "cmpg-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD23},
/* 31 */ { "cmp-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD23},
/* 32 */ { "if-eq", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 33 */ { "if-ne", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 34 */ { "if-lt", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 35 */ { "if-ge", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 36 */ { "if-gt", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 37 */ { "if-le", "2t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 38 */ { "if-eqz", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 39 */ { "if-nez", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 3A */ { "if-ltz", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 3B */ { "if-gez", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 3C */ { "if-gtz", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 3D */ { "if-lez", "1t", 2, SPECIAL_TARGET, 4, 4, FG},
/* 3E */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 3F */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 40 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 41 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 42 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 43 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 44 */ { "aget", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 45 */ { "aget-wide", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR | WD1},
/* 46 */ { "aget-object", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 47 */ { "aget-boolean", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 48 */ { "aget-byte", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 49 */ { "aget-char", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 4A */ { "aget-short", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 4B */ { "aput", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 4C */ { "aput-wide", "3x", 2, SPECIAL_NONE, 0, 0, FB | WD1},
/* 4D */ { "aput-object", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 4E */ { "aput-boolean", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 4F */ { "aput-byte", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 50 */ { "aput-char", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 51 */ { "aput-short", "3x", 2, SPECIAL_NONE, 0, 0, FB},
/* 52 */ { "iget", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 53 */ { "iget-wide", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR | WD1},
/* 54 */ { "iget-object", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 55 */ { "iget-boolean", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 56 */ { "iget-byte", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 57 */ { "iget-char", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 58 */ { "iget-short", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 59 */ { "iput", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 5A */ { "iput-wide", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WD1},
/* 5B */ { "iput-object", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 5C */ { "iput-boolean", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 5D */ { "iput-byte", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 5E */ { "iput-char", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 5F */ { "iput-short", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 60 */ { "sget", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 61 */ { "sget-wide", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR | WD1},
/* 62 */ { "sget-object", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 63 */ { "sget-boolean", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 64 */ { "sget-byte", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 65 */ { "sget-char", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 66 */ { "sget-short", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* 67 */ { "sput", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 68 */ { "sput-wide", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WD1},
/* 69 */ { "sput-object", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 6A */ { "sput-boolean", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 6B */ { "sput-byte", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 6C */ { "sput-char", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 6D */ { "sput-short", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* 6E */ { "invoke-virtual", "5c", 3, SPECIAL_METHOD, 4, 4, FC},
/* 6F */ { "invoke-super", "5c", 3, SPECIAL_METHOD, 4, 4, FC},
/* 70 */ { "invoke-direct", "5c", 3, SPECIAL_METHOD, 4, 4, FC},
/* 71 */ { "invoke-static", "5c", 3, SPECIAL_METHOD, 4, 4, FC},
/* 72 */ { "invoke-interface", "5c", 3, SPECIAL_METHOD, 4, 4, FC},
/* 73 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 74 */ { "invoke-virtual/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* 75 */ { "invoke-super/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* 76 */ { "invoke-direct/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* 77 */ { "invoke-static/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* 78 */ { "invoke-interface/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* 79 */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 7A */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Unallocated
/* 7B */ { "neg-int", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 7C */ { "not-int", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 7D */ { "neg-long", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 7E */ { "not-long", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 7F */ { "neg-float", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 80 */ { "neg-double", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 81 */ { "int-to-long", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* 82 */ { "int-to-float", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 83 */ { "int-to-double", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* 84 */ { "long-to-int", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD2},
/* 85 */ { "long-to-float", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD2},
/* 86 */ { "long-to-double", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 87 */ { "float-to-int", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 88 */ { "float-to-long", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* 89 */ { "float-to-double", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* 8A */ { "double-to-int", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD2},
/* 8B */ { "double-to-long", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* 8C */ { "double-to-float", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD2},
/* 8D */ { "int-to-byte", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 8E */ { "int-to-char", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 8F */ { "int-to-short", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* 90 */ { "add-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 91 */ { "sub-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 92 */ { "mul-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 93 */ { "div-int", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 94 */ { "rem-int", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR},
/* 95 */ { "and-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 96 */ { "or-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 97 */ { "xor-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 98 */ { "shl-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 99 */ { "shr-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 9A */ { "ushr-int", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* 9B */ { "add-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* 9C */ { "sub-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* 9D */ { "mul-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* 9E */ { "div-long", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR | WD123},
/* 9F */ { "rem-long", "3x", 2, SPECIAL_NONE, 0, 0, FB | WR | WD123},
/* A0 */ { "and-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* A1 */ { "or-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* A2 */ { "xor-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* A3 */ { "shl-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* A4 */ { "shr-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* A5 */ { "ushr-long", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* A6 */ { "add-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* A7 */ { "sub-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* A8 */ { "mul-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* A9 */ { "div-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* AA */ { "rem-float", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR},
/* AB */ { "add-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* AC */ { "sub-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* AD */ { "mul-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* AE */ { "div-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* AF */ { "rem-double", "3x", 2, SPECIAL_NONE, 0, 0, FA | WR | WD123},
/* B0 */ { "add-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B1 */ { "sub-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B2 */ { "mul-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B3 */ { "div-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FB | WR},
/* B4 */ { "rem-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FB | WR},
/* B5 */ { "and-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B6 */ { "or-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B7 */ { "xor-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B8 */ { "shl-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* B9 */ { "shr-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* BA */ { "ushr-int/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* BB */ { "add-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* BC */ { "sub-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* BD */ { "mul-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* BE */ { "div-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FB | WR | WD12},
/* BF */ { "rem-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FB | WR | WD12},
/* C0 */ { "and-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* C1 */ { "or-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* C2 */ { "xor-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* C3 */ { "shl-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* C4 */ { "shr-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* C5 */ { "ushr-long/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD1},
/* C6 */ { "add-float/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* C7 */ { "sub-float/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* C8 */ { "mul-float/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* C9 */ { "div-float/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* CA */ { "rem-float/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR},
/* CB */ { "add-double/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* CC */ { "sub-double/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* CD */ { "mul-double/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* CE */ { "div-double/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* CF */ { "rem-double/2addr", "2x", 1, SPECIAL_NONE, 0, 0, FA | WR | WD12},
/* D0 */ { "add-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D1 */ { "rsub-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D2 */ { "mul-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D3 */ { "div-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FB | WR},
/* D4 */ { "rem-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FB | WR},
/* D5 */ { "and-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D6 */ { "or-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D7 */ { "xor-int/lit16", "2s", 2, SPECIAL_CONSTANT, 4, 4, FA | WR},
/* D8 */ { "add-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* D9 */ { "rsub-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* DA */ { "mul-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* DB */ { "div-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FB | WR},
/* DC */ { "rem-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FB | WR},
/* DD */ { "and-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* DE */ { "or-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* DF */ { "xor-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* E0 */ { "shl-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* E1 */ { "shr-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* E2 */ { "ushr-int/lit8", "2b", 2, SPECIAL_CONSTANT, 4, 2, FA | WR},
/* E3 */ { "iget-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* E4 */ { "iput-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* E5 */ { "sget-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* E6 */ { "sput-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* E7 */ { "iget-object-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* E8 */ { "iget-wide-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WR | WD1},
/* E9 */ { "iput-wide-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB | WD1},
/* EA */ { "sget-wide-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR | WD1},
/* EB */ { "sput-wide-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WD1},
/* EC */ { "breakpoint", "0x", 1, SPECIAL_NONE, 0, 0, 0}, // Shouldn't see this
/* ED */ { "throw-verification-error", "0x", 1, SPECIAL_NONE, 0, 0, FE},
/* EE */ { "execute-inline", "5c", 3, SPECIAL_INLINE, 4, 4, FB},
/* EF */ { "execute-inline/range", "rc", 3, SPECIAL_INLINE, 4, 4, FB},
/* F0 */ { "object-init/range", "rc", 3, SPECIAL_METHOD, 4, 4, FC},
/* F1 */ { "return-void-barrier", "0x", 1, SPECIAL_NONE, 0, 0, FD},
/* F2 */ { "iget-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB},
/* F3 */ { "iget-wide-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB | WD1},
/* F4 */ { "iget-object-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB},
/* F5 */ { "iput-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB},
/* F6 */ { "iput-wide-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB | WD1},
/* F7 */ { "iput-object-quick", "2cs", 2, SPECIAL_OBJECT, 4, 4, FB},
/* F8 */ { "invoke-virtual-quick", "5ms", 3, SPECIAL_VTABLE, 4, 4, FC},
/* F9 */ { "invoke-virtual-quick/range", "rms", 3, SPECIAL_VTABLE, 4, 4, FC},
/* FA */ { "invoke-super-quick", "5ms", 3, SPECIAL_VTABLE, 4, 4, FC},
/* FB */ { "invoke-super-quick/range", "rms", 3, SPECIAL_VTABLE, 4, 4, FC},
/* FC */ { "iput-object-volatile", "2c", 2, SPECIAL_FIELD, 4, 4, FB},
/* FD */ { "sget-object-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB | WR},
/* FE */ { "sput-object-volatile", "1c", 2, SPECIAL_FIELD, 4, 4, FB},
/* FF */ { 0, "0x", 1, SPECIAL_NONE, 0, 0, 0},
};
