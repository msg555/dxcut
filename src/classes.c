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
#include "classes.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "annotations.h"
#include "fields.h"
#include "methods.h"
#include "values.h"

static const dx_uint NO_INDEX = 0xFFFFFFFFU;

int dxc_read_class_section(read_context* ctx, dx_uint off, dx_uint size) {
  if(!(ctx->classes = (DexClass*)calloc(size + 1, sizeof(DexClass)))) {
    DXC_ERROR("class def alloc failed");
    return 0;
  }
  ctx->classes_sz = size;
  dxc_make_sentinel_class(ctx->classes + size);
  dx_uint i;
  for(i = 0; i < size; i++) {
    DexClass* cl = ctx->classes + i;
    dx_uint class_idx = ctx->read_uint(ctx, &off);
    dx_uint access_flags = ctx->read_uint(ctx, &off);
    dx_uint superclass_idx = ctx->read_uint(ctx, &off);
    dx_uint interfaces_off = ctx->read_uint(ctx, &off);
    dx_uint source_file_idx = ctx->read_uint(ctx, &off);
    dx_uint annotations_off = ctx->read_uint(ctx, &off);
    dx_uint class_data_off = ctx->read_uint(ctx, &off);
    dx_uint static_values_off = ctx->read_uint(ctx, &off);

    if(class_idx >= ctx->types_sz) {
      DXC_ERROR("class name type index too large");
      return 0;
    }
    if(superclass_idx != NO_INDEX && superclass_idx >= ctx->types_sz) {
      DXC_ERROR("class superclass type index too large");
      return 0;
    }
    if(source_file_idx != NO_INDEX && source_file_idx >= ctx->strs_sz)  {
      DXC_ERROR("class source file string index too large");
      return 0;
    }

    cl->name = dxc_copy_str(ctx->types[class_idx]);
    cl->access_flags = (DexAccessFlags)access_flags;
    cl->super_class = superclass_idx == NO_INDEX ? NULL :
                      dxc_copy_str(ctx->types[superclass_idx]);
    cl->source_file = source_file_idx == NO_INDEX ? NULL :
                      dxc_copy_str(ctx->strs[source_file_idx]);

    /* Load the annotations for this class. */
    if(annotations_off) {
      if(!dxc_read_annotation_directory(ctx, cl, annotations_off)) {
        return 0;
      }
    } else {
      dxc_make_sentinel_annotation(cl->annotations =
          (DexAnnotation*)calloc(1, sizeof(DexAnnotation)));
    }

    /* Load the interfaces for this class. */
    dx_uint interfaces_size = 0;
    dx_uint interfaces_pos = interfaces_off;
    if(interfaces_off != 0 && !dxc_read_ok(ctx, interfaces_off, 4)) {
      DXC_ERROR("class interface list not in data section");
      return 0;
    } else if(interfaces_off != 0) {
      interfaces_size = ctx->read_uint(ctx, &interfaces_pos);
    } else {
      interfaces_size = 0;
    }
    
    if(!(cl->interfaces = dxc_create_strstr(interfaces_size))) {
      DXC_ERROR("class interface alloc failed");
      return 0;
    }
    dx_uint j;
    for(j = 0; j < interfaces_size; j++) {
      if(!dxc_read_ok(ctx, interfaces_pos, 2)) {
        DXC_ERROR("class interface list not in data section");
        return 0;
      }
      dx_ushort interface_idx = ctx->read_ushort(ctx, &interfaces_pos);
      if(interface_idx >= ctx->types_sz) {
        DXC_ERROR("interface type index too large");
        return 0;
      }
      cl->interfaces->s[j] = dxc_copy_str(ctx->types[interface_idx]);
    }

    if(class_data_off != 0) {
      if(!dxc_read_ok(ctx, class_data_off, 4)) {
        DXC_ERROR("class data outside of data section");
        return 0;
      }
      dx_uint static_fields_sz = ctx->read_uleb(ctx, &class_data_off);
      dx_uint instance_fields_sz = ctx->read_uleb(ctx, &class_data_off);
      dx_uint direct_methods_sz = ctx->read_uleb(ctx, &class_data_off);
      dx_uint virtual_methods_sz = ctx->read_uleb(ctx, &class_data_off);
      
      // Read static fields.
      dx_uint idx = 0;
      if(!(cl->static_fields = (DexField*)calloc(static_fields_sz + 1,
                                                 sizeof(DexField)))) {
        DXC_ERROR("class static field alloc failed");
        return 0;
      }
      dxc_make_sentinel_field(cl->static_fields + static_fields_sz);
      for(j = 0; j < static_fields_sz; j++) {
        if(!dxc_read_encoded_field(ctx, cl->static_fields + j, cl->name, &idx,
                                   &class_data_off)) {
          return 0;
        }
      }

      // Read instance fields.
      idx = 0;
      if(!(cl->instance_fields = (DexField*)calloc(instance_fields_sz + 1,
                                                   sizeof(DexField)))) {
        DXC_ERROR("class instance field alloc failed");
        return 0;
      }
      dxc_make_sentinel_field(cl->instance_fields + instance_fields_sz);
      for(j = 0; j < instance_fields_sz; j++) {
        if(!dxc_read_encoded_field(ctx, cl->instance_fields + j, cl->name, &idx,
                                   &class_data_off)) {
          return 0;
        }
      }

      idx = 0;
      if(!(cl->direct_methods = (DexMethod*)calloc(direct_methods_sz + 1,
                                                   sizeof(DexMethod)))) {
        DXC_ERROR("class direct method alloc failed");
        return 0;
      }
      dxc_make_sentinel_method(cl->direct_methods + direct_methods_sz);
      for(j = 0; j < direct_methods_sz; j++) {
        if(!dxc_read_encoded_method(ctx, cl->direct_methods + j, cl->name,
                                    &idx, &class_data_off)) {
          return 0;
        }
      }

      idx = 0;
      if(!(cl->virtual_methods = (DexMethod*)calloc(virtual_methods_sz + 1,
                                                    sizeof(DexMethod)))) {
        DXC_ERROR("class virtual method alloc failed");
        return 0;
      }
      dxc_make_sentinel_method(cl->virtual_methods + virtual_methods_sz);
      for(j = 0; j < virtual_methods_sz; j++) {
        if(!dxc_read_encoded_method(ctx, cl->virtual_methods + j, cl->name,
                                    &idx, &class_data_off)) {
          return 0;
        }
      }
    } else {
      if(!(cl->static_fields = (DexField*)calloc(1, sizeof(DexField)))) {
        DXC_ERROR("class empty array alloc failed");
        return 0;
      }
      dxc_make_sentinel_field(cl->static_fields);
      if(!(cl->instance_fields =
                          (DexField*)calloc(1, sizeof(DexField)))) {
        DXC_ERROR("class empty array alloc failed");
        return 0;
      }
      dxc_make_sentinel_field(cl->instance_fields);
      if(!(cl->direct_methods =
                          (DexMethod*)calloc(1, sizeof(DexMethod)))) {
        DXC_ERROR("class empty array alloc failed");
        return 0;
      }
      dxc_make_sentinel_method(cl->direct_methods);
      if(!(cl->virtual_methods =
                          (DexMethod*)calloc(1, sizeof(DexMethod)))) {
        DXC_ERROR("class empty array alloc failed");
        return 0;
      }
      dxc_make_sentinel_method(cl->virtual_methods);
    }

    if(static_values_off != 0) {
      if(!dxc_read_value_array(ctx, &cl->static_values,
                               &static_values_off, 0)) {
        return 0;
      }
    } else {
      if(!(cl->static_values = (DexValue*)calloc(1, sizeof(DexField)))) {
        DXC_ERROR("class static value alloc failed");
        return 0;
      }
      dxc_make_sentinel_value(cl->static_values);
    }
  }
  return 1;
}

void dxc_free_class(DexClass* cl) {
  if(!cl) return;
  if(cl->annotations) {
    DexAnnotation* ptr;
    for(ptr = cl->annotations;
        !dxc_is_sentinel_annotation(ptr); ptr++) dxc_free_annotation(ptr);
  }
  if(cl->static_values) {
    DexValue* ptr;
    for(ptr = cl->static_values; !dxc_is_sentinel_value(ptr);
        ptr++) dxc_free_value(ptr);
  }
  if(cl->static_fields) {
    DexField* ptr;
    for(ptr = cl->static_fields; !dxc_is_sentinel_field(ptr);
        ptr++) dxc_free_field(ptr);
  }
  if(cl->instance_fields) {
    DexField* ptr;
    for(ptr = cl->instance_fields; !dxc_is_sentinel_field(ptr);
        ptr++) dxc_free_field(ptr);
  }
  if(cl->direct_methods) {
    DexMethod* ptr;
    for(ptr = cl->direct_methods; !dxc_is_sentinel_method(ptr);
        ptr++) dxc_free_method(ptr);
  }
  if(cl->virtual_methods) {
    DexMethod* ptr;
    for(ptr = cl->virtual_methods; !dxc_is_sentinel_method(ptr);
        ptr++) dxc_free_method(ptr);
  }
  dxc_free_str(cl->name);
  dxc_free_str(cl->super_class);
  dxc_free_str(cl->source_file);
  dxc_free_strstr(cl->interfaces);
  free(cl->annotations);
  free(cl->static_values);
  free(cl->static_fields);
  free(cl->instance_fields);
  free(cl->direct_methods);
  free(cl->virtual_methods);
}

int dxc_is_sentinel_class(DexClass* cl) {
  return cl->name == NULL;
}

void dxc_make_sentinel_class(DexClass* cl) {
  cl->name = NULL;
}
