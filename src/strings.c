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
#include "strings.h"

#include <stdlib.h>
#include <stdio.h>

static
ref_str* read_string_data(read_context* ctx, dx_uint off) {
  dx_uint len = ctx->read_uleb(ctx, &off);
  dx_uint orig_off = off;
  dx_uint sz = 0;
  while(dxc_in_data(ctx, off) && ctx->read_ubyte(ctx, &off)) sz++;
  if(!dxc_in_data(ctx, off)) {
    DXC_ERROR("string data item not in data section");
    return NULL;
  }
  dx_uint i;
  return dxc_induct_str(ctx->buf + orig_off);
}

int dxc_read_string_section(read_context* ctx, dx_uint pos, dx_uint size) {
  ctx->strs = (ref_str**)calloc(size, sizeof(ref_str*));
  ctx->strs_sz = size;
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_uint string_data_off = ctx->read_uint(ctx, &pos);
    if(!(ctx->strs[i] = read_string_data(ctx, string_data_off))) {
      return 0;
    }
  }
  return 1;
}
