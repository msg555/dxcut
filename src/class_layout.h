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
#ifndef DEX_CLASS_LAYOUT_H
#define DEX_CLASS_LAYOUT_H

#include <dxcut/dex.h>

#include "common.h"

typedef struct {
  dx_uint field_size;
  dx_uint* field_offs;
  raw_field* fields;

  dx_uint vtable_size;
  raw_method* vtable;
} dxc_alignment;

dxc_alignment dxc_compute_alignment(read_context* ctx, DexClass* cl);

raw_field dxc_get_obj_by_offset(dxc_alignment* algn, dx_uint offset);

raw_method dxc_lookup_vtable(dxc_alignment* algn, dx_uint ind);

void dxc_free_alignment(dxc_alignment* algn);

#endif // DEX_CLASS_LAYOUT_H
