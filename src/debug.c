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
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static dx_uint NO_INDEX = 0xFFFFFFFFU;

int dxc_read_debug_section(read_context* ctx, DexDebugInfo* debug_info,
                           dx_uint off) {
  debug_info->line_start = ctx->read_uleb(ctx, &off);
  dx_uint parameters = ctx->read_uleb(ctx, &off);
  if(!dxc_in_data(ctx, off)) {
    DXC_ERROR("debug item outside of data section");
    return 0;
  }

  if(!(debug_info->parameter_names = dxc_create_strstr(parameters))) {
    DXC_ERROR("debug parameter alloc failed");
    return 0;
  }
  dx_uint i;
  for(i = 0; i < parameters; i++) {
    dx_uint str_idx = ctx->read_ulebp1(ctx, &off);
    if(!dxc_in_data(ctx, off)) {
      DXC_ERROR("debug item outside of data section");
      return 0;
    }
    if(str_idx == NO_INDEX) {
      debug_info->parameter_names->s[i] = dxc_copy_str(&dxc_empty_str);
    } else if(str_idx >= ctx->strs_sz) {
      DXC_ERROR("debug parameter string index too large");
      return 0;
    } else {
      debug_info->parameter_names->s[i] = dxc_copy_str(ctx->strs[str_idx]);
    }
  }

  dx_uint sz = 10;
  if(!(debug_info->insns = (DexDebugInstruction*)
                            calloc(sz, sizeof(DexDebugInstruction)))) {
    DXC_ERROR("debug instruction alloc failed");
    return 0;
  }
  for(i = 0; ; i++) {
    if(!dxc_read_ok(ctx, off, 1)) {
      DXC_ERROR("debug info not within data section");
      return 0;
    }
    if(i == sz) {
      if(!(debug_info->insns = (DexDebugInstruction*)
           realloc(debug_info->insns, 2 * sz * sizeof(DexDebugInstruction)))) {
        DXC_ERROR("debug instruction alloc failed");
        return 0;
      }
      memset(debug_info->insns + sz, 0, sz * sizeof(DexDebugInstruction));
      sz *= 2;
    }
    DexDebugInstruction* insn = debug_info->insns + i;
    insn->opcode = (DexDebugOpCode)ctx->read_ubyte(ctx, &off);
    if(insn->opcode == DBG_END_SEQUENCE) {
      break;
    }
    switch(insn->opcode) {
      case DBG_ADVANCE_PC:
        insn->p.addr_diff = ctx->read_uleb(ctx, &off);
        break;
      case DBG_ADVANCE_LINE:
        insn->p.addr_diff = ctx->read_sleb(ctx, &off);
        break;
      case DBG_START_LOCAL:
      case DBG_START_LOCAL_EXTENDED: {
        dx_uint reg_num = ctx->read_uleb(ctx, &off);
        dx_uint name_idx = ctx->read_ulebp1(ctx, &off);
        dx_uint type_idx = ctx->read_ulebp1(ctx, &off);
        dx_uint sig_idx = insn->opcode == DBG_START_LOCAL_EXTENDED ?
                          ctx->read_ulebp1(ctx, &off) : NO_INDEX;
        if((name_idx != NO_INDEX && name_idx >= ctx->strs_sz) ||
           (type_idx != NO_INDEX && type_idx >= ctx->types_sz) ||
           (sig_idx != NO_INDEX && sig_idx >= ctx->strs_sz)) {
          DXC_ERROR("start local parameters invalid");
          return 0;
        }
        insn->p.start_local =
            (typeof(insn->p.start_local))malloc(sizeof(*insn->p.start_local));
        insn->p.start_local->register_num = reg_num;
        insn->p.start_local->name = insn->p.start_local->type =
            insn->p.start_local->sig = NULL;
        insn->p.start_local->name = name_idx == NO_INDEX ? NULL :
                                    dxc_copy_str(ctx->strs[name_idx]);
        insn->p.start_local->type = type_idx == NO_INDEX ? NULL :
                                    dxc_copy_str(ctx->types[type_idx]);
        insn->p.start_local->sig = sig_idx == NO_INDEX ? NULL :
                                    dxc_copy_str(ctx->strs[sig_idx]);
        break;
      } case DBG_END_LOCAL:
      case DBG_RESTART_LOCAL:
        insn->p.register_num = ctx->read_uleb(ctx, &off);
        break;
      case DBG_SET_FILE: {
        dx_uint name_idx = ctx->read_ulebp1(ctx, &off);
        if(name_idx != NO_INDEX && name_idx >= ctx->strs_sz) {
          DXC_ERROR("set file parameters invalid");
          return 0;
        }
        insn->p.name = name_idx == NO_INDEX ? NULL :
                       dxc_copy_str(ctx->strs[name_idx]);
        break;
      }
    }
    if(!dxc_in_data(ctx, off)) {
      DXC_ERROR("debug info not within data section");
      return 0;
    }
  }
  DexDebugInstruction* reloc;
  if((reloc = (DexDebugInstruction*)
      malloc((i + 1) * sizeof(DexDebugInstruction)))) {
    memcpy(reloc, debug_info->insns, (i + 1) * sizeof(DexDebugInstruction));
    free(debug_info->insns);
    debug_info->insns = reloc;
  }
  return 1;
}

void dxc_free_debug_info(DexDebugInfo* debug_info) {
  if(!debug_info) return;
  DexDebugInstruction* insn;
  for(insn = debug_info->insns; insn->opcode != DBG_END_SEQUENCE; insn++) {
    switch(insn->opcode) {
      case DBG_START_LOCAL_EXTENDED: // break intentionally ommitted
        dxc_free_str(insn->p.start_local->sig);
      case DBG_START_LOCAL:
        dxc_free_str(insn->p.start_local->name);
        dxc_free_str(insn->p.start_local->type);
        free(insn->p.start_local);
        break;
      case DBG_SET_FILE:
        dxc_free_str(insn->p.name);
        break;
    }
  }
  dxc_free_strstr(debug_info->parameter_names);
  free(debug_info->insns);
}
