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
#include "class_layout.h"

dxc_alignment* dxc_compute_alignments(read_context* ctx, DexClass** cl) {
  int 
}

raw_field* dxc_get_obj_by_offset(dxc_alignment* algn, dx_uint offset) {
  dx_uint lo = 0;
  dx_uint hi = algn->field_size;
  while(lo < hi) {
    dx_uint md = lo + (hi - lo) / 2;
    if(algn->field_offs[md] < offset) {
      lo = md + 1;
    } else {
      hi = md;
    }
  }
  return lo < algn->field_size && offset == algn->field_offs[lo] ?
         algn->fields + lo : NULL;
}

raw_method* dxc_lookup_vtable(dxc_alignment* algn, dx_uint ind) {
  if(ind >= algn->vtable_size) {
    return NULL;
  }
  return algn->vtable + ind;
}

void dxc_free_alignment(dxc_alignment* algn) {
  int i;
  free(algn->field_offs);
  for(i = 0; i < algn->field_size; i++) {
    dxc_free_field(algn->fields + i);
  }
  free(algn->fields);
  for(i = 0; i < algn->vtable_size; i++) {
    dxc_free_method(algn->vtable + i);
  }
  free(algn->vtable);
}

