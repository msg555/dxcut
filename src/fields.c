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
#include "fields.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dxcut/field.h>
#include <dxcut/annotation.h>

int dxc_read_field_section(read_context* ctx, dx_uint off, dx_uint size) {
  ctx->fields = (raw_field*)calloc(size, sizeof(raw_field));
  ctx->fields_sz = size;
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_ushort class_idx = ctx->read_ushort(ctx, &off);
    dx_ushort type_idx = ctx->read_ushort(ctx, &off);
    dx_uint name_idx = ctx->read_uint(ctx, &off);

    if(class_idx >= ctx->types_sz) {
      DXC_ERROR("field class index too large");
      return 0;
    }
    if(type_idx >= ctx->types_sz) {
      DXC_ERROR("field type index too large");
      return 0;
    }
    if(name_idx >= ctx->strs_sz)  {
      DXC_ERROR("field name index too large");
      return 0;
    }

    raw_field* rf = ctx->fields + i;
    rf->defining_class = dxc_copy_str(ctx->types[class_idx]);
    rf->type = dxc_copy_str(ctx->types[type_idx]);
    rf->name = dxc_copy_str(ctx->strs[name_idx]);
  }
  return 1;
}

int dxc_read_encoded_field(read_context* ctx, DexField* fld, ref_str* parent,
                           dx_uint* idx, dx_uint* off) {
  *idx += ctx->read_uleb(ctx, off);
  fld->access_flags = (DexAccessFlags)ctx->read_uleb(ctx, off);
  if(*idx >= ctx->fields_sz) {
    DXC_ERROR("encoded field index too large");
    return 0;
  }
  raw_field* rf = ctx->fields + *idx;
  if(strcmp(rf->defining_class->s, parent->s)) {
    DXC_ERROR("encoded field appears with undexped defining class");
    return 0;
  }
  fld->type = dxc_copy_str(rf->type);
  fld->name = dxc_copy_str(rf->name);
  if(rf->annotations) {
    fld->annotations = rf->annotations;
    rf->annotations = NULL;
  } else {
    dxc_make_sentinel_annotation(fld->annotations =
        (DexAnnotation*)calloc(1, sizeof(DexAnnotation)));
  }
  return 1;
}

void dxc_free_field(DexField* field) {
  if(!field) return;
  if(field->annotations) {
    DexAnnotation* ptr;
    for(ptr = field->annotations;
        !dxc_is_sentinel_annotation(ptr); ptr++) dxc_free_annotation(ptr);
  }
  dxc_free_str(field->type);
  dxc_free_str(field->name);
  free(field->annotations);
}

int dxc_is_sentinel_field(const DexField* field) {
  return field->name == NULL;
}

void dxc_make_sentinel_field(DexField* field) {
  field->name = NULL;
}
