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
#include "protos.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int dxc_read_proto_section(read_context* ctx, dx_uint off, dx_uint size) {
  ctx->protos = (ref_strstr**)calloc(size, sizeof(ref_strstr*));
  ctx->protos_sz = size;
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_uint shorty_index = ctx->read_uint(ctx, &off);
    dx_uint return_type = ctx->read_uint(ctx, &off);
    dx_uint parameters_off = ctx->read_uint(ctx, &off);
    if(return_type >= ctx->types_sz)  {
      DXC_ERROR("proto type offset too large");
      return 0;
    }
    dx_uint arguments = 0;
    if(parameters_off != 0 && !dxc_read_ok(ctx, parameters_off, 4)) {
      DXC_ERROR("proto type list outside of data section");
      return 0;
    } else if(parameters_off != 0) {
      arguments = ctx->read_uint(ctx, &parameters_off);
      if(arguments + 1 == 0) {
        DXC_ERROR("prototype has too many arguments");
        return 0;
      }
    }

    ref_strstr* proto;
    if(!(ctx->protos[i] = proto = dxc_create_strstr(arguments + 1))) {
      DXC_ERROR("proto alloc failed");
      return 0;
    }
    proto->s[0] = dxc_copy_str(ctx->types[return_type]);

    dx_uint j;
    for(j = 1; j <= arguments; j++) {
      if(!dxc_read_ok(ctx, parameters_off, 2)) {
        DXC_ERROR("proto references outside data section");
        return 0;
      }
      dx_ushort type_idx = ctx->read_ushort(ctx, &parameters_off);
      if(type_idx >= ctx->types_sz) {
        DXC_ERROR("proto type offset too large");
        continue;
      }
      proto->s[j] = dxc_copy_str(ctx->types[type_idx]);
    }
  }
  return 1;
}
