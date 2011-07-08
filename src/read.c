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
#include "read.h"

#include <string.h>

#define READ_TYPE(type) \
  dx_ ## type read_ ## type(read_context* ctx, dx_uint* pos) { \
    dx_ ## type res; \
    memcpy(&res, ctx->buf + *pos, sizeof(res)); \
    *pos += sizeof(res); \
    return res; \
  }

#define READ_INV_TYPE(type) \
  dx_ ## type read_inv_ ## type(read_context* ctx, dx_uint* pos) { \
    dx_ ## type res; \
    dx_uint i; \
    for(i = 0; i < sizeof(res); i++) \
      ((char *)&res)[i] = ctx->buf[*pos + sizeof(res) - i - 1]; \
    *pos += sizeof(res); \
    return res; \
  }

READ_TYPE(ulong)
READ_TYPE(long)
READ_TYPE(uint)
READ_TYPE(int)
READ_TYPE(ushort)
READ_TYPE(short)
READ_TYPE(ubyte)
READ_TYPE(byte)
READ_INV_TYPE(ulong)
READ_INV_TYPE(long)
READ_INV_TYPE(uint)
READ_INV_TYPE(int)
READ_INV_TYPE(ushort)
READ_INV_TYPE(short)

#undef READ_TYPE
#undef READ_INV_TYPE

dx_uint read_uleb(read_context* ctx, dx_uint* pos) {
  dx_uint sz = ctx->data_off + ctx->data_sz;
  dx_uint res = 0;
  dx_uint i;
  for(i = 0; *pos < sz; i++) {
    dx_uint b = (dx_ubyte)ctx->buf[*pos];
    (*pos)++;
    res |= (b & 0x7F) << (i * 7);
    if(~b & 0x80) {
      return res;
    }
  }
  *pos = sz + 1;
  return 0;
}

dx_uint read_ulebp1(read_context* ctx, dx_uint* pos) {
  return read_uleb(ctx, pos) - 1;
}

dx_int read_sleb(read_context* ctx, dx_uint* pos) {
  dx_uint sz = ctx->data_off + ctx->data_sz;
  dx_uint res = 0;
  dx_uint msk = 0;
  dx_uint i;
  for(i = 0; *pos < sz; i++) {
    dx_uint b = (dx_ubyte)ctx->buf[*pos];
    (*pos)++;
    res |= (b & 0x7F) << (i * 7);
    msk |= 0x7F << (i * 7);
    if(~b & 0x80) {
      if(b & 0x40) {
        res |= ~msk;
      }
      return (dx_int)res;
    }
  }
  *pos = sz + 1;
  return 0;
}

void fill_read_functions(read_context* ctx) {
  ctx->read_uleb = read_uleb;
  ctx->read_ulebp1 = read_ulebp1;
  ctx->read_sleb = read_sleb;
  ctx->read_ulong = read_ulong;
  ctx->read_long = read_long;
  ctx->read_uint = read_uint;
  ctx->read_int = read_int;
  ctx->read_ushort = read_ushort;
  ctx->read_short = read_short;
  ctx->read_ubyte = read_ubyte;
  ctx->read_byte = read_byte;
}

void fill_inverse_read_functions(read_context* ctx) {
  ctx->read_uleb = read_uleb;
  ctx->read_ulebp1 = read_ulebp1;
  ctx->read_sleb = read_sleb;
  ctx->read_ulong = read_inv_ulong;
  ctx->read_long = read_inv_long;
  ctx->read_uint = read_inv_uint;
  ctx->read_int = read_inv_int;
  ctx->read_ushort = read_inv_ushort;
  ctx->read_short = read_inv_short;
  ctx->read_ubyte = read_ubyte;
  ctx->read_byte = read_byte;
}
