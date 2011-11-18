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
#include "values.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dxcut/annotation.h>
#include <dxcut/dex.h>
#include <dxcut/value.h>

#include "annotations.h"

#define MAX_ARRAY_DEPTH 256

int dxc_read_value_array(read_context* ctx, DexValue** values, dx_uint* offset,
                         dx_uint depth) {
  dx_uint sz = ctx->read_uleb(ctx, offset);
  if(!dxc_in_data(ctx, *offset)) {
    DXC_ERROR("encoded value array is outside of the data section");
    return 0;
  }
  DexValue* res;
  *values = res = (DexValue*)calloc(sz + 1, sizeof(DexValue));
  if(!res) {
    DXC_ERROR("encoded array malloc failed");
    return 0;
  }
  dxc_make_sentinel_value(res + sz);
  dx_uint i;
  for(i = 0; i < sz; i++) {
    if(!dxc_read_value(ctx, res + i, offset, depth)) {
      return 0;
    }
  }
  return 1;
}

int dxc_read_value(read_context* ctx, DexValue* value, dx_uint* offset,
                   dx_uint depth) {
  if(depth > MAX_ARRAY_DEPTH) {
    DXC_ERROR("encoded value nested too deeply");
    return 0;
  }
  if(!dxc_read_ok(ctx, *offset, 1)) {
    DXC_ERROR("encoded value not within data section");
    return 0;
  }
  dx_ubyte b = ctx->read_ubyte(ctx, offset);
  dx_ubyte value_arg = b >> 5;
  dx_ubyte value_type = b & 0x1F;
  dx_uint rsz = 0;
  dx_uint nsz = 0;
  int issigned = 0;
  int extright = value_type == VALUE_FLOAT || value_type == VALUE_DOUBLE;
  switch(value_type) {
    case VALUE_BYTE:
      issigned = nsz = rsz = 1;
    case VALUE_ARRAY:
    case VALUE_ANNOTATION:
    case VALUE_NULL:
      if(value_arg != 0) {
        DXC_ERROR("expected 0 value_arg for given value type");
        return 0;
      }
      break;
    case VALUE_SHORT:
    case VALUE_CHAR:
      rsz = value_arg + 1;
      nsz = 2;
    case VALUE_BOOLEAN:
      if(value_arg >= 2) {
        DXC_ERROR("expected value_arg in [0,1] for given value type");
        return 0;
      }
      break;
    case VALUE_INT:
      issigned = 1;
    case VALUE_FLOAT:
    case VALUE_STRING:
    case VALUE_TYPE:
    case VALUE_FIELD:
    case VALUE_METHOD:
    case VALUE_ENUM:
      nsz = 4;
      if(value_arg >= 4) {
        DXC_ERROR("expected value_arg in [0,3]");
        return 0;
      }
      rsz = value_arg + 1;
      break;
    case VALUE_LONG:
      issigned = 1;
    case VALUE_DOUBLE:
      nsz = 8;
      rsz = value_arg + 1;
      break;
    default:
      DXC_ERROR("unexpected value type");
      return 0;
  }
  if(!dxc_read_ok(ctx, *offset, rsz)) {
    DXC_ERROR("encoded value is outside of data section");
    return 0;
  }

  if(value_type == VALUE_ARRAY) {
    if(!dxc_read_value_array(ctx, &value->value.val_array, offset, depth + 1)) {
      return 0;
    }
  } else if(value_type == VALUE_ANNOTATION) {
    if(!(value->value.val_annotation =
        (DexAnnotation*)calloc(1, sizeof(DexAnnotation)))) {
      DXC_ERROR("encoded value annotation alloc failed");
      return 0;
    } else if(!dxc_read_encoded_annotation(ctx, value->value.val_annotation,
                                           offset, depth + 1, 0)) {
      return 0;
    }
  } else {
    dx_ubyte rbuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    dx_uint i;
    for(i = 0; i < rsz; i++) {
      rbuf[extright ? nsz - rsz + i : i] = ctx->read_ubyte(ctx, offset);
    }
    if(issigned && (rbuf[rsz - 1] & 0x80)) for(i = rsz; i < nsz; i++) {
      rbuf[i] = 0xFF;
    }

    dx_uint idx;
    switch(value_type) {
      case VALUE_NULL: break;
      case VALUE_BYTE: memcpy(&value->value.val_byte, rbuf, 1); break;
      case VALUE_SHORT: memcpy(&value->value.val_short, rbuf, 2); break;
      case VALUE_CHAR: memcpy(&value->value.val_char, rbuf, 2); break;
      case VALUE_INT: memcpy(&value->value.val_int, rbuf, 4); break;
      case VALUE_LONG: memcpy(&value->value.val_long, rbuf, 8); break;
      case VALUE_FLOAT: memcpy(&value->value.val_float, rbuf, 4); break;
      case VALUE_DOUBLE: memcpy(&value->value.val_double, rbuf, 8); break;
      case VALUE_BOOLEAN: value->value.val_boolean = value_arg; break;
      default:
        memcpy(&idx, rbuf, 4);
        switch(value_type) {
          case VALUE_STRING:
            if(idx >= ctx->strs_sz) {
              DXC_ERROR("value string index too large");
              return 0;
            }
            value->value.val_str = dxc_copy_str(ctx->strs[idx]);
            break;
          case VALUE_TYPE:
            if(idx >= ctx->types_sz) {
              DXC_ERROR("value type index too large");
              return 0;
            }
            value->value.val_type = dxc_copy_str(ctx->types[idx]);
            break;
          case VALUE_FIELD:
            if(idx >= ctx->fields_sz) {
              DXC_ERROR("value field index too large");
              return 0;
            }
            value->value.val_field.defining_class =
                dxc_copy_str(ctx->fields[idx].defining_class);
            value->value.val_field.type = dxc_copy_str(ctx->fields[idx].type);
            value->value.val_field.name = dxc_copy_str(ctx->fields[idx].name);
            break;
          case VALUE_METHOD: {
            if(idx >= ctx->methods_sz) {
              DXC_ERROR("value method index too large");
              return 0;
            }
            value->value.val_method.defining_class =
                dxc_copy_str(ctx->methods[idx].defining_class);
            value->value.val_method.name = dxc_copy_str(ctx->methods[idx].name);
            value->value.val_method.prototype =
                dxc_copy_strstr(ctx->methods[idx].prototype);
            break;
          } case VALUE_ENUM:
            if(idx >= ctx->fields_sz) {
              DXC_ERROR("value enum index too large");
              return 0;
            }
            value->value.val_enum.defining_class =
                dxc_copy_str(ctx->fields[idx].defining_class);
            value->value.val_enum.type = dxc_copy_str(ctx->fields[idx].type);
            value->value.val_enum.name = dxc_copy_str(ctx->fields[idx].name);
            break;
          default:
            DXC_ERROR("unknown value type encountered");
            return 0;
        }
    }
  }

  value->type = (DexValueType)value_type;
  return 1;
}

static int buf_size = -1;
static char* buffer = NULL;

const char* dxc_value_nice(const DexValue* value) {
  if(buf_size == -1) {
    buf_size = 30;
    buffer = (char*)malloc(31);
  }
  switch(value->type) {
    case VALUE_BYTE:
      sprintf(buffer, "%d", (int)value->value.val_byte);
      break;
    case VALUE_SHORT:
      sprintf(buffer, "%d", (int)value->value.val_short);
      break;
    case VALUE_CHAR:
      if(32 <= value->value.val_char && value->value.val_char < 128) {
        sprintf(buffer, "'%c'", (char)value->value.val_char);
      } else {
        sprintf(buffer, "%d", (int)value->value.val_char);
      }
      break;
    case VALUE_INT:
      sprintf(buffer, "%d", (int)value->value.val_int);
      break;
    case VALUE_LONG:
      sprintf(buffer, "%lldL", (long long)value->value.val_long);
      break;
    case VALUE_FLOAT:
      sprintf(buffer, "%.7gf", (double)value->value.val_float);
      break;
    case VALUE_DOUBLE:
      sprintf(buffer, "%.10g", (double)value->value.val_double);
      break;
    case VALUE_STRING:
    case VALUE_TYPE: {
      const char* s = value->type == VALUE_STRING ? value->value.val_str->s :
                                     dxc_type_nice(value->value.val_type->s);
      int sz = strlen(s);
      if(sz >= buf_size) {
        buf_size = sz + 8;
        buffer = realloc(buffer, buf_size + 1);
      }

      char* buf = buffer;
      buf += sprintf(buf, "\"");
      while(*s) {
        if(buf - buffer + 6 > buf_size) {
          buf_size = buf_size * 3 / 2 + 10;
          buffer = realloc(buffer, buf_size + 1);
        }

        unsigned char head = *s++;
        int code_point = 0;
        if(head >> 7 == 0x00) {
          code_point = head;
        } else if(head >> 5 == 0x06) {
          unsigned char n1 = *s++;
          if(n1 >> 6 != 0x02) {
            fprintf(stderr, "error decoding string\n"); fflush(stderr); break;
          }
          code_point = (head & 0x1F) << 6 | n1 & 0x3F;
        } else if(head >> 4 == 0x0E) {
          unsigned char n1 = *s++;
          if(n1 >> 6 != 0x02) {
            fprintf(stderr, "error decoding string\n"); fflush(stderr); break;
          }
          unsigned char n2 = *s++;
          if(n2 >> 6 != 0x02) {
            fprintf(stderr, "error decoding string\n"); fflush(stderr); break;
          }
          code_point = (head & 0x1F) << 12 | (n1 & 0x3F) << 6 | n2 & 0x3F;
        } else {
          fprintf(stderr, "error decoding string\n"); fflush(stderr); break;
        }
        switch(code_point) {
          case '\t': buf += sprintf(buf, "\\t"); break;
          case '\r': buf += sprintf(buf, "\\r"); break;
          case '\n': buf += sprintf(buf, "\\n"); break;
          case '\v': buf += sprintf(buf, "\\v"); break;
          case '\"': buf += sprintf(buf, "\\\""); break;
          case '\\': buf += sprintf(buf, "\\\\"); break;
          default:
            if(32 <= code_point && code_point < 128) {
              buf += sprintf(buf, "%c", (char)code_point);
            } else {
              buf += sprintf(buf, "\\u%04X", code_point);
            }
        }
      }
      buf += sprintf(buf, "\"");
      break;
    } case VALUE_FIELD:
      sprintf(buffer, "<field>");
      break;
    case VALUE_METHOD:
      sprintf(buffer, "<method>");
      break;
    case VALUE_ENUM:
      sprintf(buffer, "<enum>");
      break;
    case VALUE_ARRAY:
      sprintf(buffer, "{...}");
      break;
    case VALUE_ANNOTATION:
      sprintf(buffer, "<annotation>");
      break;
    case VALUE_NULL:
      sprintf(buffer, "null");
      break;
    case VALUE_BOOLEAN:
      sprintf(buffer, value->value.val_boolean ? "true" : "false");
      break;
    case VALUE_SENTINEL:
      break;
  }
  return buffer;
}

void dxc_free_value(DexValue* value) {
  if(!value) return;
  switch(value->type) {
    case VALUE_STRING:
      dxc_free_str(value->value.val_str);
      break;
    case VALUE_TYPE:
      dxc_free_str(value->value.val_type);
      break;
    case VALUE_FIELD:
      dxc_free_str(value->value.val_field.defining_class);
      dxc_free_str(value->value.val_field.type);
      dxc_free_str(value->value.val_field.name);
      break;
    case VALUE_METHOD:
      dxc_free_str(value->value.val_method.defining_class);
      dxc_free_strstr(value->value.val_method.prototype);
      dxc_free_str(value->value.val_method.name);
      break;
    case VALUE_ENUM:
      dxc_free_str(value->value.val_enum.defining_class);
      dxc_free_str(value->value.val_enum.type);
      dxc_free_str(value->value.val_enum.name);
      break;
    case VALUE_ARRAY:
      if(value->value.val_array) {
        DexValue* ptr;
        for(ptr = value->value.val_array; !dxc_is_sentinel_value(ptr);
            ptr++) dxc_free_value(ptr);
      }
      free(value->value.val_array);
      break;
    case VALUE_ANNOTATION:
      if(value->value.val_annotation) {
        dxc_free_annotation(value->value.val_annotation);
      }
      free(value->value.val_annotation);
      break;
    case VALUE_BYTE: case VALUE_SHORT: case VALUE_INT: case VALUE_LONG:
    case VALUE_FLOAT: case VALUE_DOUBLE: case VALUE_NULL: case VALUE_BOOLEAN:
    case VALUE_CHAR: case VALUE_SENTINEL:
      break;
  }
}

int dxc_is_sentinel_value(const DexValue* value) {
  return value->type == VALUE_SENTINEL;
}

void dxc_make_sentinel_value(DexValue* value) {
  value->type = VALUE_SENTINEL;
}
