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
#ifndef DEX_COMMON_H
#define DEX_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <dxcut/annotation.h>
#include <dxcut/class.h>
#include <dxcut/dex.h>

#define DXC_ERROR(x) fprintf(stderr, "%s\n", x); fflush(stderr);

typedef struct {
  ref_str* defining_class;
  ref_str* type;
  ref_str* name;
  DexAnnotation* annotations;
} raw_field;

typedef struct {
  ref_str* defining_class;
  ref_strstr* prototype;
  ref_str* name;

  DexAnnotation* annotations;
  DexAnnotation** parameter_annotations;
} raw_method;

typedef struct read_context_t {
  char* buf;
  char* odex_buf;

  dx_uint dex_version;
  dx_uint odex_version;

  ref_str** strs;
  dx_uint strs_sz;
  ref_str** types;
  dx_uint types_sz;
  ref_strstr** protos;
  dx_uint protos_sz;
  raw_field* fields;
  dx_uint fields_sz;
  raw_method* methods;
  dx_uint methods_sz;
  dx_uint data_off;
  dx_uint data_sz;
  DexClass* classes;
  dx_uint classes_sz;

  dx_uint   (*read_uleb)  (struct read_context_t* ctx, dx_uint* pos);
  dx_uint   (*read_ulebp1)(struct read_context_t* ctx, dx_uint* pos);
  dx_int    (*read_sleb)  (struct read_context_t* ctx, dx_uint* pos);
  dx_ulong  (*read_ulong) (struct read_context_t* ctx, dx_uint* pos);
  dx_long   (*read_long)  (struct read_context_t* ctx, dx_uint* pos);
  dx_uint   (*read_uint)  (struct read_context_t* ctx, dx_uint* pos);
  dx_int    (*read_int)   (struct read_context_t* ctx, dx_uint* pos);
  dx_ushort (*read_ushort)(struct read_context_t* ctx, dx_uint* pos);
  dx_short  (*read_short) (struct read_context_t* ctx, dx_uint* pos);
  dx_ubyte  (*read_ubyte) (struct read_context_t* ctx, dx_uint* pos);
  dx_byte   (*read_byte)  (struct read_context_t* ctx, dx_uint* pos);
} read_context;

extern
ref_str dxc_empty_str;

extern
int dxc_in_data(read_context* ctx, dx_uint off);

extern
int dxc_read_ok(read_context* ctx, dx_uint off, dx_uint width);

extern
raw_field dxc_copy_raw_field(raw_field fld);

extern
raw_method dxc_copy_raw_method(raw_method mtd);

extern
void dxc_free_raw_field(raw_field fld);

extern
void dxc_free_raw_method(raw_method mtd);

#ifdef __cplusplus
}
#endif
#endif // DEX_COMMON_H
