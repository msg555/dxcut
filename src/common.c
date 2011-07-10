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
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "common.h"

ref_str dxc_empty_str = {1, {0}};

int dxc_in_data(read_context* ctx, dx_uint off) {
  return ctx->data_off <= off && off <= ctx->data_off + ctx->data_sz;
}

int dxc_read_ok(read_context* ctx, dx_uint off, dx_uint width) {
  return ctx->data_off <= off && off + width <= ctx->data_off + ctx->data_sz &&
         off <= off + width;
}

ref_str* dxc_induct_str(const char* s) {
  dx_uint sz = strlen(s);
  ref_str* ret = (ref_str*)malloc(5 + sz);
  if(!ret) {
    DXC_ERROR("induct str alloc failed");
    return NULL;
  }
  ret->cnt = 1;
  strcpy(ret->s, s);
  return ret;
}

ref_str* dxc_copy_str(ref_str* s) {
  s->cnt++;
  return s;
}

ref_strstr* dxc_copy_strstr(ref_strstr* s) {
  s->cnt++;
  return s;
}

ref_strstr* dxc_create_strstr(dx_uint sz) {
  ref_strstr* ret = (ref_strstr*)calloc(offsetof(ref_strstr, s) +
                                        (sz + 1) * sizeof(ref_str*), 1);
  if(ret) ret->cnt = 1;
  return ret;
}

raw_field dxc_copy_raw_field(raw_field fld) {
  raw_field ret;
  memset(&ret, 0, sizeof(ret));
  ret.defining_class = dxc_copy_str(fld.defining_class);
  ret.name = dxc_copy_str(fld.name);
  ret.type = dxc_copy_str(fld.type);
  return ret;
}

raw_method dxc_copy_raw_method(raw_method mtd) {
  raw_method ret;
  memset(&ret, 0, sizeof(ret));
  ret.defining_class = dxc_copy_str(mtd.defining_class);
  ret.name = dxc_copy_str(mtd.name);
  ret.prototype = dxc_copy_strstr(mtd.prototype);
  return ret;
}

void dxc_free_str(ref_str* s) {
  if(!s) return;
  if(!--(s->cnt)) {
    free(s);
  }
}

void dxc_free_strstr(ref_strstr* s) {
  if(!s) return;
  if(!--(s->cnt)) {
    ref_str** ptr;
    for(ptr = s->s; *ptr; ptr++) dxc_free_str(*ptr);
    free(s);
  }
}

void dxc_free_raw_field(raw_field fld) {
  dxc_free_str(fld.defining_class);
  dxc_free_str(fld.name);
  dxc_free_str(fld.type);
}

void dxc_free_raw_method(raw_method mtd) {
  dxc_free_str(mtd.defining_class);
  dxc_free_str(mtd.name);
  dxc_free_strstr(mtd.prototype);
}
