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
#include "types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int dxc_read_type_section(read_context* ctx, dx_uint off, dx_uint size) {
  dx_uint i;
  ctx->types = (ref_str**)calloc(size, sizeof(ref_str*));
  ctx->types_sz = size;
  for(i = 0; i < size; i++) {
    dx_uint index = ctx->read_uint(ctx, &off);
    if(index >= ctx->strs_sz)  {
      DXC_ERROR("type id string offset too large");
      return 0;
    }
    ctx->types[i] = dxc_copy_str(ctx->strs[index]);
  }
  return 1;
}

static int buf_size = -1;
static char* buffer = NULL;

const char* dxc_type_nice(const char* type) {
  switch(*type) {
    case 'V': return "void";
    case 'Z': return "boolean";
    case 'B': return "byte";
    case 'S': return "short";
    case 'C': return "char";
    case 'I': return "int";
    case 'J': return "long";
    case 'F': return "float";
    case 'D': return "double";
    case 'L': break;
    case '[': {
      const char* res = dxc_type_nice(type + 1);
      int sz = strlen(res);
      if(sz + 2 > buf_size) {
        buffer = (char*)realloc(buffer, sz + 9);
        buf_size = sz + 8;
      }
      if(res != buffer) {
        strcpy(buffer, res);
      }
      buffer[sz] = '['; buffer[sz + 1] = ']'; buffer[sz + 2] = 0;
      return buffer;
    } default: return NULL;
  }
  type++;
  int sz = strlen(type);
  if(sz == 0 || type[sz - 1] != ';') return NULL;
  if(sz > buf_size || 64 > buf_size) {
    free(buffer);
    buffer = (char*)malloc(sz + 7);
    buf_size = sz + 6;
  }
  int i;
  for(i = 0; i + 1 < sz; i++) {
    if(type[i] == '/') {
      buffer[i] = '.';
    } else {
      buffer[i] = type[i];
    }
  }
  buffer[sz - 1] = 0;
  return buffer;
}

const char* dxc_type_name(const char* nice_type) {
  int sz = strlen(nice_type);
  if(sz + 8 > buf_size) {
    buffer = (char*)realloc(buffer, sz + 9);
    buf_size = sz + 8;
  }
  int pos = 0;
  while(sz >= 2 && nice_type[sz - 1] == ']' && nice_type[sz - 2] == '[') {
    sz -= 2;
    buffer[pos++] = '[';
  }
  int processed = 1;
  switch(sz) {
    case 3:
      if(!strncmp(nice_type, "int", sz)) buffer[pos++] = 'I';
      else processed = 0;
      break;
    case 4:
      if(!strncmp(nice_type, "void", sz)) buffer[pos++] = 'V';
      else if(!strncmp(nice_type, "char", sz)) buffer[pos++] = 'C';
      else if(!strncmp(nice_type, "long", sz)) buffer[pos++] = 'J';
      else if(!strncmp(nice_type, "byte", sz)) buffer[pos++] = 'B';
      else processed = 0;
      break;
    case 5:
      if(!strncmp(nice_type, "short", sz)) buffer[pos++] = 'S';
      else if(!strncmp(nice_type, "float", sz)) buffer[pos++] ='F';
      else processed = 0;
      break;
    case 6:
      if(!strncmp(nice_type, "double", sz)) buffer[pos++] = 'D';
      else processed = 0;
      break;
    case 7:
      if(!strncmp(nice_type, "double", sz)) buffer[pos++] = 'Z';
      else processed = 0;
      break;
    default:
      processed = 0;
  }
  if(!processed) {
    buffer[pos++] = 'L';
    int i;
    for(i = 0; i < sz; i++) {
      char ch = nice_type[i];
      buffer[pos++] = ch == '.' ? '/' : ch;
    }
    buffer[pos++] = ';';
  }
  buffer[pos] = 0;
  return buffer;
}

