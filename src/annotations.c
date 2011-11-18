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
#include "annotations.h"
#include "values.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int dxc_read_encoded_annotation(read_context* ctx, DexAnnotation* annotation,
                                dx_uint* off,
                                dx_uint depth, dx_uint has_visibility) {
  if(!dxc_read_ok(ctx, *off, 1)) {
    DXC_ERROR("encoded annotation not within data section");
    return 0;
  }
  if(has_visibility) {
    annotation->visibility = (DexAnnotationVisibility)
        ctx->read_ubyte(ctx, off);
  } else {
    annotation->visibility = VISIBILITY_NONE;
  }

  dx_uint type_idx = ctx->read_uleb(ctx, off);
  dx_uint size = ctx->read_uleb(ctx, off);
  if(!dxc_in_data(ctx, *off)) {
    DXC_ERROR("encoded annotation not within data section");
    return 0;
  }
  if(type_idx >= ctx->types_sz) {
    DXC_ERROR("annotation type index too large");
    return 0;
  }
  annotation->type = dxc_copy_str(ctx->types[type_idx]);
  annotation->parameters = (DexNameValuePair*)calloc(size + 1,
                                                     sizeof(DexNameValuePair));
  if(!annotation->parameters) {
    DXC_ERROR("annotation parameter alloc failed");
    return 0;
  }
  dxc_make_sentinel_parameter(annotation->parameters + size);
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_uint name_idx = ctx->read_uleb(ctx, off);
    if(!dxc_in_data(ctx, *off)) {
      DXC_ERROR("encoded annotation not within data section");
      return 0;
    }
    if(name_idx >= ctx->strs_sz) {
      DXC_ERROR("annotation parameter name index too large");
      return 0;
    }
    annotation->parameters[i].name = dxc_copy_str(ctx->strs[name_idx]);
    if(!dxc_read_value(ctx, &annotation->parameters[i].value, off, depth)) {
      return 0;
    }
  }
  return 1;
}

int dxc_read_annotation_list(read_context* ctx, DexAnnotation** anlist,
                             dx_uint off) {
  if(!dxc_read_ok(ctx, off, 4)) {
    DXC_ERROR("annotation list item not within data section");
    return 0;
  }
  dx_uint size = ctx->read_uint(ctx, &off);
  if(!dxc_read_ok(ctx, off, size * 4)) {
    DXC_ERROR("annotation list item not within data section");
    return 0;
  }
  DexAnnotation* res;
  *anlist = res = (DexAnnotation*)calloc(size + 1, sizeof(DexAnnotation));
  if(!res) {
    DXC_ERROR("annotation list alloc failed");
    return 0;
  }
  dxc_make_sentinel_annotation(res + size);
  dx_uint i;
  for(i = 0; i < size; i++) {
    dx_uint annon_off = ctx->read_uint(ctx, &off);
    if(!dxc_read_encoded_annotation(ctx, res + i, &annon_off, 0, 1)) {
      return 0;
    }
  }
  return 1;
}

int dxc_read_annotation_directory(read_context* ctx, DexClass* cl,
                                  dx_uint off) {
  if(!dxc_read_ok(ctx, off, 4 * 4)) {
    DXC_ERROR("annotation directory not within data section");
    return 0;
  }
  dx_uint class_annotation_off = ctx->read_uint(ctx, &off);
  dx_uint fsz = ctx->read_uint(ctx, &off);
  dx_uint msz = ctx->read_uint(ctx, &off);
  dx_uint psz = ctx->read_uint(ctx, &off);
  if(!dxc_read_ok(ctx, off, (fsz + msz + psz) * 8)) {
    DXC_ERROR("annotation directory not within data section");
    return 0;
  }
  if(class_annotation_off == 0) {
    if(!(cl->annotations = (DexAnnotation*)calloc(1, sizeof(DexAnnotation)))) {
      DXC_ERROR("annotation directory alloc failed");
      return 0;
    }
    dxc_make_sentinel_annotation(cl->annotations);
  } else {
    if(!dxc_read_annotation_list(ctx, &cl->annotations, class_annotation_off)) {
      return 0;
    }
  }
  dx_uint i;
  for(i = 0; i < fsz; i++) {
    dx_uint field_idx = ctx->read_uint(ctx, &off);
    dx_uint annotation_off = ctx->read_uint(ctx, &off);
    if(field_idx >= ctx->fields_sz) {
      DXC_ERROR("annotation field index too large");
      return 0;
    }
    if(!dxc_read_annotation_list(ctx, &ctx->fields[field_idx].annotations,
                                 annotation_off)) {
      return 0;
    }
  }
  for(i = 0; i < msz; i++) {
    dx_uint method_idx = ctx->read_uint(ctx, &off);
    dx_uint annotation_off = ctx->read_uint(ctx, &off);
    if(method_idx >= ctx->methods_sz) {
      DXC_ERROR("annotation method index too large");
      return 0;
    }
    if(!dxc_read_annotation_list(ctx, &ctx->methods[method_idx].annotations,
                                 annotation_off)) {
      return 0;
    }
  }
  for(i = 0; i < psz; i++) {
    dx_uint method_idx = ctx->read_uint(ctx, &off);
    dx_uint annotation_off = ctx->read_uint(ctx, &off);
    if(method_idx >= ctx->methods_sz) {
      DXC_ERROR("annotation method index too large");
      return 0;
    }
    if(!dxc_read_ok(ctx, annotation_off, 4)) {
      DXC_ERROR("annotation set ref list not within data section");
      return 0;
    }
    dx_uint params = ctx->read_uint(ctx, &annotation_off);
    if(!dxc_read_ok(ctx, annotation_off, 4 * params)) {
      DXC_ERROR("annotation set ref list not within data section");
      return 0;
    }
    if(!(ctx->methods[method_idx].parameter_annotations =
        (DexAnnotation**)calloc(params + 1, sizeof(DexAnnotation*)))) {
      DXC_ERROR("annotation set ref list alloc failed");
      return 0;
    }
    dx_uint j;
    for(j = 0; j < params; j++) {
      dx_uint annon_off = ctx->read_uint(ctx, &annotation_off);
      if(annon_off == 0) {
        if(!(ctx->methods[method_idx].parameter_annotations[j] =
            (DexAnnotation*)calloc(1, sizeof(DexAnnotation)))) {
          DXC_ERROR("annotation set ref list alloc failed");
          return 0;
        }
        dxc_make_sentinel_annotation(
            ctx->methods[method_idx].parameter_annotations[j]);
      } else if(!dxc_read_annotation_list(ctx,
            ctx->methods[method_idx].parameter_annotations + j, annon_off)) {
        return 0;
      }
    }
  }
  return 1;
}

void dxc_free_annotation(DexAnnotation* annotation) {
  if(!annotation) return;
  if(annotation->parameters) {
    DexNameValuePair* ptr;
    for(ptr = annotation->parameters;
        !dxc_is_sentinel_parameter(ptr); ptr++) dxc_free_parameter(ptr);
  }
  dxc_free_str(annotation->type);
  free(annotation->parameters);
}

int dxc_is_sentinel_annotation(const DexAnnotation* annotation) {
  return annotation->parameters == NULL;
}

void dxc_make_sentinel_annotation(DexAnnotation* annotation) {
  annotation->parameters = NULL;
}

void dxc_free_parameter(DexNameValuePair* parameter) {
  if(!parameter) return;
  dxc_free_str(parameter->name);
  dxc_free_value(&parameter->value);
}

int dxc_is_sentinel_parameter(const DexNameValuePair* parameter) {
  return parameter->name == NULL;
}

void dxc_make_sentinel_parameter(DexNameValuePair* parameter) {
  parameter->name = NULL;
}
