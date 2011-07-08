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
#include "methods.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dxcut/method.h>

#include "code.h"

int dxc_read_method_section(read_context* ctx, dx_uint off, dx_uint size) {
  ctx->methods = (raw_method*)calloc(size, sizeof(raw_method));
  ctx->methods_sz = size;
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_ushort class_idx = ctx->read_ushort(ctx, &off);
    dx_ushort proto_idx = ctx->read_ushort(ctx, &off);
    dx_uint name_idx = ctx->read_uint(ctx, &off);

    if(class_idx >= ctx->types_sz) {
      DXC_ERROR("method class type index too large");
      return 0;
    }
    if(proto_idx >= ctx->protos_sz) {
      DXC_ERROR("method prototype index too large");
      return 0;
    }
    if(name_idx >= ctx->strs_sz)  {
      DXC_ERROR("method name string index too large");
      return 0;
    }

    raw_method* rm = ctx->methods + i;
    rm->defining_class = dxc_copy_str(ctx->types[class_idx]);
    rm->prototype = dxc_copy_strstr(ctx->protos[proto_idx]);
    rm->name = dxc_copy_str(ctx->strs[name_idx]);
  }
  return 1;
}

int dxc_read_encoded_method(read_context* ctx, DexMethod* method,
                            ref_str* parent, dx_uint* method_idx,
                            dx_uint* off) {
  *method_idx += ctx->read_uleb(ctx, off);
  dx_uint access_flags = ctx->read_uleb(ctx, off);
  dx_uint code_off = ctx->read_uleb(ctx, off);
  if(!dxc_in_data(ctx, *off)) {
    DXC_ERROR("encoded method not in data section");
    return 0;
  }
  if(*method_idx >= ctx->methods_sz) {
    DXC_ERROR("encoded method index too large");
    return 0;
  }
  raw_method* rm = ctx->methods + *method_idx;
  if(strcmp(rm->defining_class->s, parent->s)) {
    DXC_ERROR("encoded method points to method with wrong parent type");
    return 0;
  }

  // Check for any annotations that were associated with this method_id.
  if(rm->annotations) {
    method->annotations = rm->annotations;
    rm->annotations = NULL;
  } else {
    dxc_make_sentinel_annotation(method->annotations =
        (DexAnnotation*)calloc(1, sizeof(DexAnnotation)));
  }

  // Check for any parameter annotations that were associated with this
  // method_id.
  if(rm->parameter_annotations) {
    method->parameter_annotations = rm->parameter_annotations;
    rm->parameter_annotations = NULL;
  } else {
    method->parameter_annotations =
        (DexAnnotation**)calloc(1, sizeof(DexAnnotation*));
  }

  method->access_flags = (DexAccessFlags)access_flags;
  method->name = dxc_copy_str(rm->name);
  method->prototype = dxc_copy_strstr(rm->prototype);
  
  if(code_off == 0) {
    method->code_body = NULL;
  } else {
    method->code_body = (DexCode*)calloc(1, sizeof(DexCode));
    if(!dxc_read_code(ctx, method->code_body, code_off)) {
      return 0;
    }
  }

  return 1;
}

void dxc_free_method(DexMethod* method) {
  if(!method) return;
  if(method->code_body) dxc_free_code(method->code_body);
  if(method->annotations) {
    DexAnnotation* ptr;
    for(ptr = method->annotations;
        !dxc_is_sentinel_annotation(ptr); ptr++) dxc_free_annotation(ptr);
  }
  if(method->parameter_annotations) {
    DexAnnotation** pan;
    for(pan = method->parameter_annotations; *pan; pan++) {
      DexAnnotation* an;
      for(an = *pan; !dxc_is_sentinel_annotation(an); an++) {
        dxc_free_annotation(an);
      }
      free(*pan);
    }
  }
  dxc_free_str(method->name);
  dxc_free_strstr(method->prototype);
  free(method->code_body);
  free(method->annotations);
  free(method->parameter_annotations);
}

int dxc_is_sentinel_method(DexMethod* method) {
  return method->prototype == NULL;
}

void dxc_make_sentinel_method(DexMethod* method) {
  method->prototype = NULL;
}
