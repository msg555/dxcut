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
#include "code.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dalvik.h"
#include "debug.h"

int dxc_read_code(read_context* ctx, DexCode* code, dx_uint off) {
  if(!dxc_read_ok(ctx, off, 16)) {
    DXC_ERROR("code item not within data section");
    return 0;
  }
  code->registers_size = ctx->read_ushort(ctx, &off);
  code->ins_size = ctx->read_ushort(ctx, &off);
  code->outs_size = ctx->read_ushort(ctx, &off);
  dx_ushort tries_size = ctx->read_ushort(ctx, &off);
  
  dx_uint debug_info_off = ctx->read_uint(ctx, &off);
  if(debug_info_off) {
    if(!(code->debug_information =
        (DexDebugInfo*)calloc(1, sizeof(DexDebugInfo)))) {
      DXC_ERROR("code debug alloc failed");
      return 0;
    }
    if(!dxc_read_debug_section(ctx, code->debug_information, debug_info_off)) {
      return 0;
    }
  }

  dx_uint insns_size = ctx->read_uint(ctx, &off);
  if(!dxc_read_ok(ctx, off, insns_size * 2)) {
    DXC_ERROR("code item not within data section");
    return 0;
  }
  
  if(!dxc_read_dalvik(ctx, insns_size, off, &code->insns, &code->insns_count)) {
    return 0;
  }
  off += sizeof(dx_ushort) * insns_size;
  
  if(tries_size == 0) {
    if(!(code->tries = (DexTryBlock*)calloc(1, sizeof(DexTryBlock)))) {
      DXC_ERROR("code try block alloc failed");
      return 0;
    }
    dxc_make_sentinel_try_block(code->tries);
  } else {
    if(insns_size & 1) {
      /* Need to burn a short to make things 4-byte aligned. */
      off += sizeof(dx_ushort);
    }
    dx_uint handler_off = off + tries_size * 8;
    
    if(!(code->tries =
       (DexTryBlock*)calloc(tries_size + 1, sizeof(DexTryBlock)))) {
      DXC_ERROR("code try block alloc failed");
      return 0;
    }
    dxc_make_sentinel_try_block(code->tries + tries_size);
    int i;
    for(i = 0; i < tries_size; i++) {
      DexTryBlock* t = code->tries + i;
      t->start_addr = ctx->read_uint(ctx, &off);
      t->insn_count = ctx->read_ushort(ctx, &off);
      dx_uint hoff = handler_off + ctx->read_ushort(ctx, &off);
      
      dx_int hsz = ctx->read_sleb(ctx, &hoff);
      if(!dxc_in_data(ctx, hoff)) {
        DXC_ERROR("encoded handler not within data section");
        return 0;
      }

      int has_catch_all = hsz <= 0;
      if(has_catch_all) hsz = -hsz;

      if(!(t->handlers = (DexHandler*)calloc(hsz + 1, sizeof(DexHandler)))) {
        DXC_ERROR("code handler alloc failed");
        return 0;
      }
      dxc_make_sentinel_handler(t->handlers + hsz);
      int j;
      for(j = 0; j < hsz; j++) {
        dx_uint type_idx = ctx->read_uleb(ctx, &hoff);
        dx_uint addr = ctx->read_uleb(ctx, &hoff);
        if(!dxc_in_data(ctx, hoff)) {
          DXC_ERROR("encoded handler not within data section");
          return 0;
        }
        if(type_idx >= ctx->types_sz) {
          DXC_ERROR("handler catch type index too large");
          return 0;
        }
        t->handlers[j].type = dxc_copy_str(ctx->types[type_idx]);
        t->handlers[j].addr = addr;
      }

      if(has_catch_all) {
        if(!(t->catch_all_handler =
            (DexHandler*)calloc(1, sizeof(DexHandler)))) {
          DXC_ERROR("code catch all handler alloc failed");
          return 0;
        }
        t->catch_all_handler->addr = ctx->read_uleb(ctx, &hoff);
        if(!dxc_in_data(ctx, hoff)) {
          DXC_ERROR("encoded handler not within data section");
          return 0;
        }
      }
    }
  }
  return 1;
}

void dxc_free_code(DexCode* code) {
  if(!code) return;
  if(code->debug_information) {
    dxc_free_debug_info(code->debug_information);
  }
  if(code->tries) {
    DexTryBlock* ptr;
    for(ptr = code->tries;
        !dxc_is_sentinel_try_block(ptr); ptr++) dxc_free_try_block(ptr);
  }
  if(code->insns) {
    dx_uint i;
    for(i = 0; i < code->insns_count; i++) {
      dxc_free_instruction(code->insns + i);
    }
  }
  free(code->debug_information);
  free(code->tries);
  free(code->insns);
}
