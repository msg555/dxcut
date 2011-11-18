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

#include <dxcut/dxcut.h>

#include "common.h"
#include "file.h"
#include "fields.h"
#include "methods.h"
#include "mutf8.h"

static const dx_uint NO_INDEX = 0xFFFFFFFFU;

typedef struct {
  ref_str** strs;
  ref_str** types;
  ref_strstr** protos;
  raw_field* fields;
  raw_method* methods;
  dx_uint strs_size;
  dx_uint types_size;
  dx_uint protos_size;
  dx_uint fields_size;
  dx_uint methods_size;
  dx_uint strs_cap;
  dx_uint types_cap;
  dx_uint protos_cap;
  dx_uint fields_cap;
  dx_uint methods_cap;
} constant_pool;

static
int compare_proto(ref_strstr* aa, ref_strstr* bb) {
  ref_str** a = aa->s;
  ref_str** b = bb->s;
  for(; *a && *b; a++, b++) {
    int res = mutf8_compare((*a)->s, (*b)->s);
    if(res) return res;
  }
  if(*a == NULL) return *b ? -1 : 0;
  return 1;
}

static
int compare_field(raw_field a, raw_field b) {
  int res = mutf8_compare(a.defining_class->s, b.defining_class->s);
  if(res) return res;
  res = mutf8_compare(a.name->s, b.name->s);
  if(res) return res;
  return mutf8_compare(a.type->s, b.type->s);
}

static
int compare_method(raw_method a, raw_method b) {
  int res = mutf8_compare(a.defining_class->s, b.defining_class->s);
  if(res) return res;
  res = mutf8_compare(a.name->s, b.name->s);
  if(res) return res;
  return compare_proto(a.prototype, b.prototype);
}

static int compare_mutf8_ptr(const void* a, const void* b) {
  return mutf8_compare((*(ref_str**)a)->s, (*(ref_str**)b)->s);
}
static int compare_proto_ptr(const void* a, const void* b) {
  return compare_proto(*(ref_strstr**)a, *(ref_strstr**)b);
}
static int compare_field_ptr(const void* a, const void* b) {
  return compare_field(*(raw_field*)a, *(raw_field*)b);
}
static int compare_method_ptr(const void* a, const void* b) {
  return compare_method(*(raw_method*)a, *(raw_method*)b);
}

#define add_pool(A, sz, cap, val, cpy_func) \
  if(sz == cap) { \
    cap = cap * 3 / 2 + 1; \
    A = (typeof(A))realloc(A, sizeof(*A) * cap); \
  } \
  A[sz++] = cpy_func(val);

static
void add_str(constant_pool* pool, ref_str* str) {
  add_pool(pool->strs, pool->strs_size, pool->strs_cap, str, dxc_copy_str);
}

static
void add_type(constant_pool* pool, ref_str* type) {
  add_pool(pool->types, pool->types_size, pool->types_cap, type, dxc_copy_str);
}

static
void add_proto(constant_pool* pool, ref_strstr* proto) {
  add_pool(pool->protos, pool->protos_size, pool->protos_cap, proto,
           dxc_copy_strstr);
}

static
void add_field(constant_pool* pool, raw_field field) {
  add_pool(pool->fields, pool->fields_size, pool->fields_cap, field,
           dxc_copy_raw_field);
}

static
void add_method(constant_pool* pool, raw_method method) {
  add_pool(pool->methods, pool->methods_size, pool->methods_cap, method,
           dxc_copy_raw_method);
}

#undef find_pool

#define find_pool(A, sz, val, cmp) \
  dx_uint lo = 0; \
  dx_uint hi = sz; \
  while(lo < hi) { \
    dx_uint mid = lo + (hi - lo) / 2; \
    int r = cmp(val, A[mid]); \
    if(!r) return mid; \
    if(r < 0) hi = mid; \
    else lo = mid + 1; \
  } \
  return NO_INDEX;

static
dx_uint find_str(constant_pool* pool, ref_str* str) {
  find_pool(pool->strs, pool->strs_size, str, mutf8_ref_compare);
}

static
dx_uint find_type(constant_pool* pool, ref_str* type) {
  find_pool(pool->types, pool->types_size, type, mutf8_ref_compare);
}

static
dx_uint find_proto(constant_pool* pool, ref_strstr* proto) {
  find_pool(pool->protos, pool->protos_size, proto, compare_proto);
}

static
dx_uint find_field(constant_pool* pool, raw_field field) {
  find_pool(pool->fields, pool->fields_size, field, compare_field);
}

static
dx_uint find_method(constant_pool* pool, raw_method method) {
  find_pool(pool->methods, pool->methods_size, method, compare_method);
}

#undef find_pool

typedef enum DexItemTypes {
  // deps: all
  TYPE_HEADER_ITEM = 0x0000,
  // deps: string_data
  TYPE_STRING_ID_ITEM = 0x0001,
  // deps: strings
  TYPE_TYPE_ID_ITEM = 0x0002,
  // deps: types
  TYPE_PROTO_ID_ITEM = 0x0003,
  // deps: strings, types
  TYPE_FIELD_ID_ITEM = 0x0004,
  // deps: strings, types, protos
  TYPE_METHOD_ID_ITEM = 0x0005,
  // deps: strings, types, type_list, annotation_directory, class_data,
  //       encoded_array
  TYPE_CLASS_DEF_ITEM = 0x0006,
  // deps: all
  TYPE_MAP_LIST = 0x1000,
  // deps: types
  TYPE_TYPE_LIST = 0x1001,
  // deps: annotation_set
  TYPE_ANNOTATION_SET_REF_LIST = 0x1002,
  // deps: annotation
  TYPE_ANNOTATION_SET_ITEM = 0x1003,
  // deps: fields, methods, code
  TYPE_CLASS_DATA_ITEM = 0x2000,
  // deps: types, debug_info
  TYPE_CODE_ITEM = 0x2001,
  // deps: none
  TYPE_STRING_DATA_ITEM = 0x2002,
  // deps: strings, types
  TYPE_DEBUG_INFO_ITEM = 0x2003,
  // deps: strings, types, protos, fields, methods
  TYPE_ANNOTATION_ITEM = 0x2004,
  // deps: strings, types, protos, fields, methods
  TYPE_ENCODED_ARRAY_ITEM = 0x2005,
  // deps: fields, methods, annotation_set, annotation_set_ref_list
  TYPE_ANNOTATIONS_DIRECTORY_ITEM = 0x2006,
  TYPE_LAST
} DexItemTypes;

// The order to resolve dependencies.  The type at index i only depends on types
// with strictly lower index.  TYPE_HEADER and TYPE_MAP are the constant pool
// items are handled specially.
DexItemTypes resolve_order[] = {
  TYPE_STRING_DATA_ITEM,
  TYPE_TYPE_LIST,
  TYPE_DEBUG_INFO_ITEM,
  TYPE_CODE_ITEM,
  TYPE_CLASS_DATA_ITEM,
  TYPE_ANNOTATION_ITEM,
  TYPE_ANNOTATION_SET_ITEM,
  TYPE_ANNOTATION_SET_REF_LIST,
  TYPE_ENCODED_ARRAY_ITEM,
  TYPE_ANNOTATIONS_DIRECTORY_ITEM,
};

typedef struct {
  dx_uint index;
  dx_uint offset;
} resolve_item;

typedef struct {
  int type;
  char* data;
  dx_uint data_sz;
  dx_uint data_cap;
  struct {
    dx_uint sz;
    dx_uint cap;
    resolve_item dat[1];
  }* resolve;
} data_item;

typedef struct {
  constant_pool pool;
  data_item* dat;

  int dat_sz;
  int dat_cap;
} write_context;

void init_ctx(write_context* ctx) {
  constant_pool* pool = &ctx->pool;
  pool->strs_size = pool->types_size = pool->protos_size =
      pool->fields_size = pool->methods_size = 0;
  pool->strs_cap = pool->types_cap = pool->protos_cap =
      pool->fields_cap = pool->methods_cap = 10;
  pool->strs = (ref_str**)malloc(sizeof(ref_str*) * pool->strs_cap);
  pool->types = (ref_str**)malloc(sizeof(ref_str*) * pool->types_cap);
  pool->protos = (ref_strstr**)malloc(sizeof(ref_strstr*) * pool->protos_cap);
  pool->fields = (raw_field*)malloc(sizeof(raw_field) * pool->fields_cap);
  pool->methods = (raw_method*)malloc(sizeof(raw_method) * pool->methods_cap);
  ctx->dat_sz = 0;
  ctx->dat_cap = 128;
  ctx->dat = (data_item*)malloc(sizeof(data_item) * ctx->dat_cap);
}

// Isn't responsible for freeing the data_items in dat.
static
void free_ctx(write_context ctx) {
  free(ctx.dat);
  dx_uint i;
  for(i = 0; i < ctx.pool.strs_size; i++) dxc_free_str(ctx.pool.strs[i]);
  for(i = 0; i < ctx.pool.types_size; i++) dxc_free_str(ctx.pool.types[i]);
  for(i = 0; i < ctx.pool.protos_size; i++) dxc_free_strstr(ctx.pool.protos[i]);
  for(i = 0; i < ctx.pool.fields_size; i++) {
    dxc_free_raw_field(ctx.pool.fields[i]);
  }
  for(i = 0; i < ctx.pool.methods_size; i++) {
    dxc_free_raw_method(ctx.pool.methods[i]);
  }
  free(ctx.pool.strs);
  free(ctx.pool.types);
  free(ctx.pool.protos);
  free(ctx.pool.fields);
  free(ctx.pool.methods);
}

static
data_item init_data_item(int type) {
  data_item ret;
  ret.type = type;
  ret.data_sz = 0;
  ret.data_cap = 4;
  ret.data = (char*)malloc(ret.data_cap);
  ret.resolve = NULL;
  return ret;
}

void free_data_item(data_item d) {
  free(d.resolve);
  free(d.data);
}

dx_uint add_data(write_context* ctx, data_item d) {
  if(ctx->dat_sz == ctx->dat_cap) {
    ctx->dat_cap = ctx->dat_cap * 3 / 2 + 1;
    ctx->dat = (data_item*)realloc(ctx->dat, sizeof(data_item) * ctx->dat_cap);
  }
  ctx->dat[ctx->dat_sz] = d;
  return ctx->dat_sz++;
}

void concat_data_and_free(data_item* d, data_item* d2) {
  if(d->data_sz + d2->data_sz > d->data_cap) {
    while(d->data_sz + d2->data_sz > d->data_cap) {
      d->data_cap = d->data_cap * 3 / 2 + 1;
    }
    d->data = (char*)realloc(d->data, d->data_cap);
  }
  memcpy(d->data + d->data_sz, d2->data, d2->data_sz);
  d->data_sz += d2->data_sz;
  free(d2->data);

  if(d2->resolve) {
    if(d->resolve) {
      if(d->resolve->sz + d2->resolve->sz > d->resolve->cap) {
        d->resolve->cap = d->resolve->sz + d2->resolve->sz;
        d->resolve = (typeof(d->resolve))realloc(d->resolve,
                                    8 + sizeof(resolve_item) * d->resolve->cap);
      }
      memcpy(d->resolve->dat + d->resolve->sz, d2->resolve->dat,
             d2->resolve->sz);
      d->resolve->sz += d2->resolve->sz;
    } else {
      d->resolve = d2->resolve;
    }
  }
  free(d2->resolve);
}

#define pop_array(A, pool, sentinel_func, pop_func) { \
  typeof(A) ptr; \
  for(ptr = A; !sentinel_func(ptr); ptr++) { \
    pop_func(ptr, pool); \
  } \
}

static void pop_value(DexValue*, constant_pool*);

static
void pop_annotation(DexAnnotation* an, constant_pool* pool) {
  add_type(pool, an->type);
  DexNameValuePair* para;
  for(para = an->parameters; !dxc_is_sentinel_parameter(para); para++) {
    add_str(pool, para->name);
    pop_value(&para->value, pool);
  }
}

static
void pop_value(DexValue* val, constant_pool* pool) {
  switch(val->type) {
    case VALUE_STRING:
      add_str(pool, val->value.val_str);
      break;
    case VALUE_TYPE:
      add_type(pool, val->value.val_type);
      break;
    case VALUE_FIELD:
    case VALUE_ENUM: {
      raw_field fld;
      fld.defining_class = val->value.val_field.defining_class;
      fld.name = val->value.val_field.name;
      fld.type = val->value.val_field.type;
      add_field(pool, fld);
      break;
    } case VALUE_METHOD: {
      raw_method mtd;
      mtd.defining_class = val->value.val_method.defining_class;
      mtd.name = val->value.val_method.name;
      mtd.prototype = val->value.val_method.prototype;
      add_method(pool, mtd);
      break;
    } case VALUE_ARRAY:
      pop_array(val->value.val_array, pool, dxc_is_sentinel_value, pop_value);
      break;
    case VALUE_ANNOTATION:
      pop_annotation(val->value.val_annotation, pool);
      break;
    case VALUE_BYTE: case VALUE_SHORT: case VALUE_CHAR: case VALUE_INT:
    case VALUE_LONG: case VALUE_FLOAT: case VALUE_DOUBLE: case VALUE_NULL:
    case VALUE_BOOLEAN: case VALUE_SENTINEL:
      break;
  }
}

static
void pop_field(DexField* fld, constant_pool* pool) {
  add_type(pool, fld->type);
  add_str(pool, fld->name);
  pop_array(fld->annotations, pool, dxc_is_sentinel_annotation, pop_annotation);
}

static
void pop_dbg_insn(DexDebugInstruction* dbg_insn, constant_pool* pool) {
  switch(dbg_insn->opcode) {
    case DBG_START_LOCAL_EXTENDED:
      if(dbg_insn->p.start_local->sig) {
        add_str(pool, dbg_insn->p.start_local->sig);
      }
    case DBG_START_LOCAL:
      if(dbg_insn->p.start_local->name) {
        add_str(pool, dbg_insn->p.start_local->name);
      }
      if(dbg_insn->p.start_local->type) {
        add_type(pool, dbg_insn->p.start_local->type);
      }
      break;
    case DBG_SET_FILE:
      if(dbg_insn->p.name) add_str(pool, dbg_insn->p.name);
      break;
  }
}

static
void pop_debug_information(DexDebugInfo* dbg, constant_pool* pool) {
  DexDebugInstruction* in;
  for(in = dbg->insns; in->opcode != DBG_END_SEQUENCE; in++) {
    pop_dbg_insn(in, pool);
  }
  ref_str** s;
  for(s = dbg->parameter_names->s; *s; s++) {
    if((*s)->s[0]) add_str(pool, *s);
  }
}

static
void pop_handler(DexHandler* handler, constant_pool* pool) {
  if(handler->type) add_type(pool, handler->type);
}

static
void pop_try_block(DexTryBlock* try_block, constant_pool* pool) {
  pop_array(try_block->handlers, pool, dxc_is_sentinel_handler, pop_handler);
  if(try_block->catch_all_handler) {
    pop_handler(try_block->catch_all_handler, pool);
  }
}

static
void pop_insn(DexInstruction* insn, constant_pool* pool) {
  switch(dex_opcode_formats[insn->opcode].specialType) {
    case SPECIAL_STRING:
      add_str(pool, insn->special.str);
      break;
    case SPECIAL_TYPE:
      add_type(pool, insn->special.type);
      break;
    case SPECIAL_FIELD: {
      raw_field fld;
      fld.defining_class = insn->special.field.defining_class;
      fld.name = insn->special.field.name;
      fld.type = insn->special.field.type;
      add_field(pool, fld);
      break;
    } case SPECIAL_METHOD: {
      raw_method mtd;
      mtd.defining_class = insn->special.method.defining_class;
      mtd.name = insn->special.method.name;
      mtd.prototype = insn->special.method.prototype;
      add_method(pool, mtd);
      break;
    }
    case SPECIAL_NONE: case SPECIAL_CONSTANT: case SPECIAL_TARGET:
    case SPECIAL_INLINE: case SPECIAL_OBJECT: case SPECIAL_VTABLE:
      break;
  }
}

static
void pop_code(DexCode* code, constant_pool* pool) {
  if(code->debug_information) {
    pop_debug_information(code->debug_information, pool);
  }
  pop_array(code->tries, pool, dxc_is_sentinel_try_block, pop_try_block);
  dx_uint i;
  for(i = 0; i < code->insns_count; i++) {
    pop_insn(code->insns + i, pool);
  }
}

static
void pop_method(DexMethod* mtd, constant_pool* pool) {
  add_str(pool, mtd->name);
  add_proto(pool, mtd->prototype);
  if(mtd->code_body) pop_code(mtd->code_body, pool);
  pop_array(mtd->annotations, pool, dxc_is_sentinel_annotation, pop_annotation);
  DexAnnotation** anns;
  for(anns = mtd->parameter_annotations; *anns; anns++) {
    pop_array(*anns, pool, dxc_is_sentinel_annotation, pop_annotation);
  }
}

static
void pop_class(DexClass* cl, constant_pool* pool) {
  add_type(pool, cl->name);
  if(cl->super_class) add_type(pool, cl->super_class);
  ref_str** s;
  for(s = cl->interfaces->s; *s; s++) {
    add_type(pool, *s);
  }
  if(cl->source_file) add_str(pool, cl->source_file);
  pop_array(cl->annotations, pool, dxc_is_sentinel_annotation, pop_annotation);
  pop_array(cl->static_values, pool, dxc_is_sentinel_value, pop_value);
  pop_array(cl->static_fields, pool, dxc_is_sentinel_field, pop_field);
  pop_array(cl->instance_fields, pool, dxc_is_sentinel_field, pop_field);
  pop_array(cl->direct_methods, pool, dxc_is_sentinel_method, pop_method);
  pop_array(cl->virtual_methods, pool, dxc_is_sentinel_method, pop_method);
  int iter;
  DexField* fld;
  for(iter = 0; iter < 2; iter++)
  for(fld = iter ? cl->instance_fields : cl->static_fields;
      !dxc_is_sentinel_field(fld); fld++) {
    raw_field rf;
    rf.defining_class = cl->name;
    rf.name = fld->name;
    rf.type = fld->type;
    add_field(pool, rf);
  }
  DexMethod* mtd;
  for(iter = 0; iter < 2; iter++)
  for(mtd = iter ? cl->virtual_methods : cl->direct_methods;
      !dxc_is_sentinel_method(mtd); mtd++) {
    raw_method rm;
    rm.defining_class = cl->name;
    rm.name = mtd->name;
    rm.prototype = mtd->prototype;
    add_method(pool, rm);
  }
}

static
ref_str* getShorty(ref_strstr* s) {
  int sz = 0;
  ref_str** t;
  for(t = s->s; *t; t++) sz++;
  ref_str* ret = (ref_str*)malloc(4 + sz + 1);
  ret->cnt = 1;
  char* rets;
  for(rets = ret->s, t = s->s; *t; t++, rets++) {
    if((*t)->s[0] == '[') *rets = 'L';
    else *rets = (*t)->s[0];
  }
  *rets = 0;
  return ret;
}

static
void write_ubyte(data_item* d, dx_ubyte x) {
  if(d->data_sz == d->data_cap) {
    d->data_cap = d->data_cap * 3 / 2 + 1;
    d->data = (char*)realloc(d->data, d->data_cap);
  }
  d->data[d->data_sz++] = x;
}

static
void write_byte(data_item* d, dx_byte x) { write_ubyte(d, x); }

static
void write_ushort(data_item* d, dx_ushort x) {
  write_ubyte(d, x & 0xFF);
  write_ubyte(d, x >> 8 & 0xFF);
}

static
void write_short(data_item* d, dx_short x) { write_ushort(d, x); }

static
void write_uint(data_item* d, dx_uint x) {
  write_ubyte(d, x & 0xFF);
  write_ubyte(d, x >> 8 & 0xFF);
  write_ubyte(d, x >> 16 & 0xFF);
  write_ubyte(d, x >> 24 & 0xFF);
}

static
void write_int(data_item* d, dx_int x) { write_uint(d, x); }

static
void write_ulong(data_item* d, dx_ulong x) {
  write_ubyte(d, x & 0xFF);
  write_ubyte(d, x >> 8 & 0xFF);
  write_ubyte(d, x >> 16 & 0xFF);
  write_ubyte(d, x >> 24 & 0xFF);
  write_ubyte(d, x >> 32 & 0xFF);
  write_ubyte(d, x >> 40 & 0xFF);
  write_ubyte(d, x >> 48 & 0xFF);
  write_ubyte(d, x >> 56 & 0xFF);
}

static
void write_long(data_item* d, dx_long x) { write_ulong(d, x); }

static
void write_uleb(data_item* d, dx_uint x) {
  do {
    write_ubyte(d, 0x80 | (x & 0x7F));
    x = x >> 7;
  } while(x);
  d->data[d->data_sz - 1] ^= 0x80;
}

static
void write_sleb(data_item* d, dx_int x) {
  while(1) {
    write_ubyte(d, 0x80 | (x & 0x7F));
    x = x >> 6;
    if(x == 0 || x == -1) break;
    x = x >> 1;
  }
  d->data[d->data_sz - 1] ^= 0x80;
}

static
void write_ulebp1(data_item* d, dx_uint x) { write_uleb(d, x + 1); }

static
void write_resolve(data_item* d, dx_uint rid) {
  if(!d->resolve) {
    d->resolve = (typeof(d->resolve))malloc(8 + sizeof(resolve_item));
    d->resolve->sz = 0;
    d->resolve->cap = 1;
  } else if(d->resolve->sz == d->resolve->cap) {
    d->resolve->cap = d->resolve->cap * 3 / 2 + 1;
    d->resolve = (typeof(d->resolve))realloc(d->resolve,
                                    8 + sizeof(resolve_item) * d->resolve->cap);
  }
  d->resolve->dat[d->resolve->sz].index = rid;
  d->resolve->dat[d->resolve->sz].offset = d->data_sz;
  d->resolve->sz++;
  write_uint(d, 0);
}

static
void write_uleb_resolve(data_item* d, dx_uint rid) {
  write_resolve(d, rid);
  d->data[d->data_sz - 4] = 0xFF;
}

static
dx_ushort uint2ushort(dx_uint x) {
  if(x > 0xFFFF) {
    fprintf(stderr, "uint to large in conversion to ushort\n");
    fflush(stderr);
    return 0;
  }
  return x;
}

static
void align(data_item* d, int alignment) {
  while(d->data_sz % alignment) {
    write_ubyte(d, 0);
  }
}

typedef struct {
  dx_uint id;
  dx_uint index;
} IdIndexPair;

static
int compare_idindexpair(const void* a, const void* b) {
  IdIndexPair* aa = (IdIndexPair*)a;
  IdIndexPair* bb = (IdIndexPair*)b;
  if(aa->id < bb->id) return -1;
  if(aa->id > bb->id) return 1;
  if(aa->index < bb->index) return -1;
  if(aa->index > bb->index) return 1;
  return 0;
}

static
void write_encoded_annotation(write_context* ctx, data_item* d,
                              DexAnnotation* an);

static
void write_encoded_array(write_context* ctx, data_item* d,
                         DexValue* vals);

static
void write_encoded_value(write_context* ctx, data_item* d,
                         DexValue* val) {
  constant_pool* pool = &ctx->pool;
  dx_ulong v = 0;
  int sgn = 0;
  int frc = -1;
  switch(val->type) {
    case VALUE_BYTE:
      v = val->value.val_byte;
      sgn = 1;
      break;
    case VALUE_SHORT:
      v = val->value.val_short;
      sgn = 1;
      break;
    case VALUE_CHAR:
      v = val->value.val_char;
      sgn = 0;
      break;
    case VALUE_INT:
      v = val->value.val_int;
      sgn = 1;
      break;
    case VALUE_LONG:
      v = val->value.val_long;
      sgn = 1;
      break;
    case VALUE_FLOAT:
      memcpy(&v, &val->value.val_float, 4);
      frc = 4;
      break;
    case VALUE_DOUBLE:
      memcpy(&v, &val->value.val_double, 8);
      frc = 8;
      break;
    case VALUE_STRING:
      v = find_str(pool, val->value.val_str);
      break;
    case VALUE_TYPE:
      v = find_type(pool, val->value.val_type);
      break;
    case VALUE_FIELD: {
      raw_field fld;
      fld.defining_class = val->value.val_field.defining_class;
      fld.name = val->value.val_field.name;
      fld.type = val->value.val_field.type;
      v = find_field(pool, fld);
      break;
    } case VALUE_METHOD: {
      raw_method mtd;
      mtd.defining_class = val->value.val_method.defining_class;
      mtd.name = val->value.val_method.name;
      mtd.prototype = val->value.val_method.prototype;
      v = find_method(pool, mtd);
      break;
    } case VALUE_ENUM: {
      raw_field fld;
      fld.defining_class = val->value.val_enum.defining_class;
      fld.name = val->value.val_enum.name;
      fld.type = val->value.val_enum.type;
      v = find_field(pool, fld);
      break;
    } case VALUE_ARRAY:
      write_ubyte(d, VALUE_ARRAY);
      write_encoded_array(ctx, d, val->value.val_array);
      return;
    case VALUE_ANNOTATION:
      write_ubyte(d, VALUE_ANNOTATION);
      write_encoded_annotation(ctx, d, val->value.val_annotation);
      return;
    case VALUE_NULL:
      write_ubyte(d, VALUE_NULL);
      return;
    case VALUE_BOOLEAN:
      write_ubyte(d, VALUE_BOOLEAN | (val->value.val_boolean ? 1 : 0) << 5);
      return;
    default:
      fprintf(stderr, "unrecognized value type\n");
      fflush(stderr);
  }
  if(frc != -1) {
    write_ubyte(d, val->type | ((frc - 1) << 5));
    int i;
    for(i = 0; i < frc; i++) {
      write_ubyte(d, (v >> 8 * i) & 0xFF);
    }
  } else {
    data_item dtmp = init_data_item(0);
    if(sgn) {
      int sbt;
      dx_long vv = v;
      do {
        sbt = vv & 0x80;
        write_ubyte(&dtmp, vv & 0xFF);
        vv = vv >> 8;
      } while(!((sbt && vv == -1) || (!sbt && vv == 0)));
    } else {
      do {
        write_ubyte(&dtmp, v & 0xFF);
        v = v >> 8;
      } while(v);
    }
    write_ubyte(d, val->type | ((dtmp.data_sz - 1) << 5));
    concat_data_and_free(d, &dtmp);
  }
}

static
void write_encoded_array(write_context* ctx, data_item* d,
                         DexValue* vals) {
  dx_uint sz = 0;
  DexValue* ptr;
  for(ptr = vals; !dxc_is_sentinel_value(ptr); ptr++) sz++;
  write_uleb(d, sz);
  for(ptr = vals; !dxc_is_sentinel_value(ptr); ptr++) {
    write_encoded_value(ctx, d, ptr);
  }
}

static
dx_uint write_encoded_array_item(write_context* ctx, DexValue* vals) {
  data_item d = init_data_item(TYPE_ENCODED_ARRAY_ITEM);
  write_encoded_array(ctx, &d, vals);
  return add_data(ctx, d);
}

static
void write_encoded_annotation(write_context* ctx, data_item* d,
                              DexAnnotation* an) {
  dx_uint sz = 0;
  DexNameValuePair* ptr;
  for(ptr = an->parameters; !dxc_is_sentinel_parameter(ptr); ptr++) sz++;
  write_uleb(d, find_type(&ctx->pool, an->type));
  write_uleb(d, sz);

  IdIndexPair* param_data = (IdIndexPair*)malloc(sizeof(IdIndexPair) * sz);
  IdIndexPair* pos;
  for(ptr = an->parameters, pos = param_data;
      !dxc_is_sentinel_parameter(ptr); ptr++, pos++) {
    pos->id = find_str(&ctx->pool, ptr->name);
    pos->index = pos - param_data;
  }
  qsort(param_data, sz, sizeof(IdIndexPair), compare_idindexpair);
  for(pos = param_data; pos != param_data + sz; pos++) {
    write_uleb(d, pos->id);
    write_encoded_value(ctx, d, &an->parameters[pos->index].value);
  }
  free(param_data);
}

static
dx_uint write_annotation_item(write_context* ctx, DexAnnotation* an) {
  data_item d = init_data_item(TYPE_ANNOTATION_ITEM);
  write_ubyte(&d, an->visibility);
  write_encoded_annotation(ctx, &d, an);
  return add_data(ctx, d);
}

static
dx_uint write_annotation_set_item(write_context* ctx,
                                  DexAnnotation* anns) {
  data_item d = init_data_item(TYPE_ANNOTATION_SET_ITEM);
  dx_uint sz = 0;
  DexAnnotation* an;
  for(an = anns; !dxc_is_sentinel_annotation(an); an++) sz++;

  write_uint(&d, sz);
  for(an = anns; !dxc_is_sentinel_annotation(an); an++) {
    write_resolve(&d, write_annotation_item(ctx, an));
  }
  return add_data(ctx, d);

  IdIndexPair* param_data = (IdIndexPair*)malloc(sizeof(IdIndexPair) * sz);
  IdIndexPair* pos;
  for(an = anns, pos = param_data; !dxc_is_sentinel_annotation(an); an++, pos++) {
    pos->id = find_type(&ctx->pool, an->type);
    pos->index = write_annotation_item(ctx, an);
  }
  qsort(param_data, sz, sizeof(IdIndexPair), compare_idindexpair);
  for(pos = param_data; pos != param_data + sz; pos++) {
    write_resolve(&d, pos->index);
  }
  free(param_data);
}

static
dx_uint write_annotation_set_ref_list(write_context* ctx,
                                      DexAnnotation** anns) {
  data_item d = init_data_item(TYPE_ANNOTATION_SET_REF_LIST);
  dx_uint sz = 0;
  DexAnnotation** an;
  for(an = anns; *an; an++) sz++;

  write_uint(&d, sz);
  for(an = anns; *an; an++) {
    write_resolve(&d, write_annotation_set_item(ctx, *an));
  }
  return add_data(ctx, d);
}

static
dx_uint write_annotation_directory(write_context* ctx, DexClass* cl) {
  constant_pool* pool = &ctx->pool;

  dx_uint fields_sz = 0;
  dx_uint methods_sz = 0;
  DexField* fld;
  DexMethod* mtd;
  for(fld = cl->static_fields; !dxc_is_sentinel_field(fld); fld++) fields_sz++;
  for(fld = cl->instance_fields; !dxc_is_sentinel_field(fld); fld++) {
    fields_sz++;
  }
  for(mtd = cl->direct_methods; !dxc_is_sentinel_method(mtd); mtd++) {
    methods_sz++;
  }
  for(mtd = cl->virtual_methods; !dxc_is_sentinel_method(mtd); mtd++) {
    methods_sz++;
  }
  IdIndexPair* field_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                 fields_sz);
  IdIndexPair* method_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                  methods_sz);
  IdIndexPair* param_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                 methods_sz);
  dx_uint fields_cnt = 0;
  dx_uint methods_cnt = 0;
  dx_uint param_cnt = 0;
  dx_uint fields_pos = 0;
  dx_uint methods_pos = 0;
  dx_uint param_pos = 0;

  int iter;
  for(iter = 0; iter < 2; iter++)
  for(fld = (iter ? cl->instance_fields : cl->static_fields);
      !dxc_is_sentinel_field(fld); fld++) {
    raw_field rf;
    rf.defining_class = cl->name;
    rf.name = fld->name;
    rf.type = fld->type;
    field_offs[fields_pos].id = find_field(pool, rf);
    if(!dxc_is_sentinel_annotation(fld->annotations)) {
      fields_cnt++;
      field_offs[fields_pos++].index =
          write_annotation_set_item(ctx, fld->annotations);
    } else {
      field_offs[fields_pos++].index = NO_INDEX;
    }
  }
  for(iter = 0; iter < 2; iter++)
  for(mtd = (iter ? cl->virtual_methods : cl->direct_methods);
      !dxc_is_sentinel_method(mtd); mtd++) {
    raw_method rm;
    rm.defining_class = cl->name;
    rm.name = mtd->name;
    rm.prototype = mtd->prototype;
    method_offs[methods_pos].id = find_method(pool, rm);
    if(!dxc_is_sentinel_annotation(mtd->annotations)) {
      methods_cnt++;
      method_offs[methods_pos++].index =
          write_annotation_set_item(ctx, mtd->annotations);
    } else {
      method_offs[methods_pos++].index = NO_INDEX;
    }
    param_offs[param_pos].id = find_method(pool, rm);
    if(*mtd->parameter_annotations) {
      param_cnt++;
      param_offs[param_pos++].index =
          write_annotation_set_ref_list(ctx, mtd->parameter_annotations);
    } else {
      param_offs[param_pos++].index = NO_INDEX;
    }
  }
  qsort(field_offs, fields_pos, sizeof(IdIndexPair), compare_idindexpair);
  qsort(method_offs, methods_pos, sizeof(IdIndexPair), compare_idindexpair);
  qsort(param_offs, param_pos, sizeof(IdIndexPair), compare_idindexpair);

  data_item d = init_data_item(TYPE_ANNOTATIONS_DIRECTORY_ITEM);
  if(dxc_is_sentinel_annotation(cl->annotations)) {
    write_uint(&d, 0);
  } else {
    write_resolve(&d, write_annotation_set_item(ctx, cl->annotations));
  }
  write_uint(&d, fields_cnt);
  write_uint(&d, methods_cnt);
  write_uint(&d, param_cnt);
  dx_uint i;
  for(i = 0; i < fields_pos; i++) {
    if(field_offs[i].index != NO_INDEX) {
      write_uint(&d, field_offs[i].id);
      write_resolve(&d, field_offs[i].index);
    }
  }
  for(i = 0; i < methods_pos; i++) {
    if(method_offs[i].index != NO_INDEX) {
      write_uint(&d, method_offs[i].id);
      write_resolve(&d, method_offs[i].index);
    }
  }
  for(i = 0; i < param_pos; i++) {
    if(param_offs[i].index != NO_INDEX) {
      write_uint(&d, param_offs[i].id);
      write_resolve(&d, param_offs[i].index);
    }
  }
  
  free(field_offs);
  free(method_offs);
  free(param_offs);
  return add_data(ctx, d);
}

static
void write_dalvik(write_context* ctx, data_item* d,
                  DexInstruction* insns, dx_uint insns_count) {
  constant_pool* pool = &ctx->pool;
  dx_uint insns_size = 0;
  data_item data = init_data_item(0);
  dx_uint i;
  for(i = 0; i < insns_count; i++) {
    DexInstruction* insn = insns + i;
    if(insn->opcode == OP_PSUEDO &&
       insn->hi_byte != PSUEDO_OP_NOP) {
      dx_ushort head = insn->hi_byte;
      head = head << 8 | insn->opcode;
      write_ushort(&data, head);
      switch(insn->hi_byte) {
        case PSUEDO_OP_PACKED_SWITCH: {
          dx_ushort sz = insn->special.packed_switch.size;
          dx_int first_key = insn->special.packed_switch.first_key;
          dx_int* targets = insn->special.packed_switch.targets;
          write_ushort(&data, sz);
          write_ushort(&data, first_key & 0xFFFF);
          write_ushort(&data, first_key >> 16 & 0xFFFF);
          int j;
          for(j = 0; j < sz; j++) {
            write_ushort(&data, targets[j] & 0xFFFF);
            write_ushort(&data, targets[j] >> 16 & 0xFFFF);
          }
          break;
        } case PSUEDO_OP_SPARSE_SWITCH: {
          dx_ushort sz = insn->special.sparse_switch.size;
          dx_int* keys = insn->special.sparse_switch.keys;
          dx_int* targets = insn->special.sparse_switch.targets;
          write_ushort(&data, sz);
          int j;
          for(j = 0; j < sz; j++) {
            write_ushort(&data, keys[j] & 0xFFFF);
            write_ushort(&data, keys[j] >> 16 & 0xFFFF);
          }
          for(j = 0; j < sz; j++) {
            write_ushort(&data, targets[j] & 0xFFFF);
            write_ushort(&data, targets[j] >> 16 & 0xFFFF);
          }
          break;
        } case PSUEDO_OP_FILL_DATA_ARRAY: {
          dx_ushort element_width = insn->special.fill_data_array.element_width;
          dx_uint sz = insn->special.fill_data_array.size;
          dx_ubyte* dat = insn->special.fill_data_array.data;
          write_ushort(&data, element_width);
          write_ushort(&data, sz & 0xFFFF);
          write_ushort(&data, sz >> 16 & 0xFFFF);
          dx_uint j;
          for(j = 0; j < element_width * sz; j++) {
            write_ubyte(&data, dat[j]);
          }
          align(&data, 2);
          break;
        } default:
          fprintf(stderr, "unrecognized psuedo opcode\n");
          fflush(stderr);
      }
    } else {
      DexOpFormat fmt = dex_opcode_formats[insn->opcode];
      dx_short param[5];
      if((fmt.size == 1 && !strcmp("0x", fmt.format_id)) ||
         (fmt.size == 2 && !strcmp("0t", fmt.format_id)) ||
         (fmt.size == 3 && !strcmp("0t", fmt.format_id)) ||
         (fmt.size == 3 && !strcmp("2x", fmt.format_id))) {
        // high byte of first short must be 0.  We fill in the low byte with
        // the opcode below.
        param[0] = insn->opcode;
      } else {
        param[0] = ((dx_ushort)insn->hi_byte) << 8 | insn->opcode;
      }
      param[1] = insn->param[0];
      param[2] = insn->param[1];
      if(fmt.specialType != SPECIAL_NONE) {
        dx_ulong v = 0;
        switch(fmt.specialType) {
          case SPECIAL_CONSTANT:
            v = insn->special.constant;
            break;
          case SPECIAL_TARGET:
            v = (dx_long)insn->special.target;
            break;
          case SPECIAL_STRING:
            v = find_str(pool, insn->special.str);
            break;
          case SPECIAL_TYPE:
            v = find_type(pool, insn->special.type);
            break;
          case SPECIAL_FIELD: {
            raw_field rf;
            rf.defining_class = insn->special.field.defining_class;
            rf.name = insn->special.field.name;
            rf.type = insn->special.field.type;
            v = find_field(pool, rf);
            break;
          } case SPECIAL_METHOD: {
            raw_method rm;
            rm.defining_class = insn->special.method.defining_class;
            rm.name = insn->special.method.name;
            rm.prototype = insn->special.method.prototype;
            v = find_method(pool, rm);
            break;
          } case SPECIAL_INLINE:
            v = insn->special.inline_ind;
            break;
          case SPECIAL_OBJECT:
            v = insn->special.object_off;
            break;
          case SPECIAL_VTABLE:
            v = insn->special.vtable_ind;
            break;
          default:
            fprintf(stderr, "unexpected special type in dalvik\n");
            fflush(stderr);
        }
        int pos = fmt.specialPos;
        int size = fmt.specialSize;
        if(fmt.specialType == SPECIAL_CONSTANT ||
           fmt.specialType == SPECIAL_TARGET) {
          dx_long vv = v;
          if(size < 16 && 
             (vv < -(1LL << (size * 4 - 1)) || (1LL << (size * 4 - 1)) <= vv)) {
fprintf(stderr, "wut2 %s\n", fmt.name);
            DXC_ERROR("special value too large for encoding");
          }
        } else if((size < 16) && v >= (1ULL << size * 4)) { 
fprintf(stderr, "wut %s\n", fmt.name);
          DXC_ERROR("special value too large for encoding");
        }
        switch(pos) {
          case 0:
            switch(size) {
              case 1:
                param[0] = (param[0] & 0x0FFFUL) | v << 12;
                break;
              case 2:
                param[0] = (param[0] & 0x00FFUL) | v << 8;
                break;
              default:
                DXC_ERROR("unhandled special alignment");
            }
            break;
          case 4:
            switch(size) {
              case 16:
                param[4] = v >> 48 & 0xFFFF;
                param[3] = v >> 32 & 0xFFFF;
              case 8:
                param[2] = v >> 16 & 0xFFFF;
              case 4:
                param[1] = v & 0xFFFF;
                break;
              case 2:
                param[1] = (param[1] & 0x00FF) | v << 8;
                break;
              default:
                DXC_ERROR("unhandled special alignment");
            }
            break;
          default:
            DXC_ERROR("unhandled special alignment");
        }
      }
      int j;
      for(j = 0; j < fmt.size; j++) {
        write_ushort(&data, param[j]);
      }
    }
  }
  write_uint(d, data.data_sz / 2);
  concat_data_and_free(d, &data);
}

static
dx_uint write_debug_info_item(write_context* ctx, DexDebugInfo* dbg) {
  constant_pool* pool = &ctx->pool;
  data_item d = init_data_item(TYPE_DEBUG_INFO_ITEM);
  
  dx_uint para_sz = 0;
  ref_str** ptr;
  for(ptr = dbg->parameter_names->s; *ptr; ptr++) para_sz++;

  write_uleb(&d, dbg->line_start);
  write_uleb(&d, para_sz);
  for(ptr = dbg->parameter_names->s; *ptr; ptr++) {
    write_ulebp1(&d, (*ptr)->s[0] ? find_str(pool, *ptr) : NO_INDEX);
  }
  DexDebugInstruction* insn;
  for(insn = dbg->insns; ; insn++) {
    write_ubyte(&d, insn->opcode);
    switch(insn->opcode) {
      case DBG_ADVANCE_PC:
        write_uleb(&d, insn->p.addr_diff);
        break;
      case DBG_ADVANCE_LINE: 
        write_sleb(&d, insn->p.line_diff);
        break;
      case DBG_START_LOCAL:
      case DBG_START_LOCAL_EXTENDED:
        write_uleb(&d, insn->p.start_local->register_num);
        write_ulebp1(&d, insn->p.start_local->name ?
                         find_str(pool, insn->p.start_local->name) : NO_INDEX);
        write_ulebp1(&d, insn->p.start_local->type ?
                         find_type(pool, insn->p.start_local->type) : NO_INDEX);
        if(insn->opcode == DBG_START_LOCAL_EXTENDED) {
          write_ulebp1(&d, insn->p.start_local->sig ?
                           find_str(pool, insn->p.start_local->sig) : NO_INDEX);
        }
        break;
      case DBG_END_LOCAL:
      case DBG_RESTART_LOCAL:
        write_uleb(&d, insn->p.register_num);
        break;
      case DBG_SET_FILE:
        write_ulebp1(&d, insn->p.name ?
                     find_str(pool, insn->p.name) : NO_INDEX);
        break;
    }
    if(insn->opcode == DBG_END_SEQUENCE) break;
  }
  return add_data(ctx, d);
}

static
dx_uint write_code_item(write_context* ctx, DexCode* code) {
  constant_pool* pool = &ctx->pool;
  data_item d = init_data_item(TYPE_CODE_ITEM);

  dx_uint try_sz = 0;
  DexTryBlock* ptr;
  for(ptr = code->tries; !dxc_is_sentinel_try_block(ptr); ptr++) try_sz++;

  write_ushort(&d, code->registers_size);
  write_ushort(&d, code->ins_size);
  write_ushort(&d, code->outs_size);
  write_ushort(&d, uint2ushort(try_sz));
  if(code->debug_information) {
    write_resolve(&d, write_debug_info_item(ctx, code->debug_information));
  } else {
    write_uint(&d, 0);
  }
  write_dalvik(ctx, &d, code->insns, code->insns_count);
  
  if(!try_sz) {
    return add_data(ctx, d);
  }

  IdIndexPair* try_data = (IdIndexPair*)malloc(sizeof(IdIndexPair) * try_sz);
  IdIndexPair* pos;
  for(ptr = code->tries, pos = try_data;
      !dxc_is_sentinel_try_block(ptr); ptr++, pos++) {
    pos->id = ptr->start_addr;
    pos->index = pos - try_data;
  }
  qsort(try_data, try_sz, sizeof(IdIndexPair), compare_idindexpair);

  align(&d, 4);
  data_item catch_handler = init_data_item(0);
  write_uleb(&catch_handler, try_sz);
  for(pos = try_data; pos != try_data + try_sz; pos++) {
    ptr = code->tries + pos->index;
    dx_uint hsz = 0;
    DexHandler* hnd;
    for(hnd = ptr->handlers; !dxc_is_sentinel_handler(hnd); hnd++) hsz++;

    if(!hsz && !ptr->catch_all_handler) {
      fprintf(stderr, "try block with no handlers detected\n");
      fflush(stderr);
    }

    write_uint(&d, ptr->start_addr);
    write_ushort(&d, ptr->insn_count);
    write_ushort(&d, catch_handler.data_sz);

    write_sleb(&catch_handler, ptr->catch_all_handler ?
               -(dx_int)hsz : (dx_int)hsz);
    for(hnd = ptr->handlers; !dxc_is_sentinel_handler(hnd); hnd++) {
      write_uleb(&catch_handler, find_type(pool, hnd->type));
      write_uleb(&catch_handler, hnd->addr);
    }
    if(ptr->catch_all_handler) {
      write_uleb(&catch_handler, ptr->catch_all_handler->addr);
    }
  }
  concat_data_and_free(&d, &catch_handler);
  free(try_data);

  return add_data(ctx, d);
}

static
dx_uint write_class_data(write_context* ctx, DexClass* cl) {
  constant_pool* pool = &ctx->pool;

  dx_uint static_fields_sz = 0;
  dx_uint instance_fields_sz = 0;
  dx_uint direct_methods_sz = 0;
  dx_uint virtual_methods_sz = 0;
  DexField* fld;
  DexMethod* mtd;
  for(fld = cl->static_fields; !dxc_is_sentinel_field(fld); fld++) {
    static_fields_sz++;
  }
  for(fld = cl->instance_fields; !dxc_is_sentinel_field(fld); fld++) {
    instance_fields_sz++;
  }
  for(mtd = cl->direct_methods; !dxc_is_sentinel_method(mtd); mtd++) {
    direct_methods_sz++;
  }
  for(mtd = cl->virtual_methods; !dxc_is_sentinel_method(mtd); mtd++) {
    virtual_methods_sz++;
  }
  IdIndexPair* static_field_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                        static_fields_sz);
  IdIndexPair* instance_field_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                          instance_fields_sz);
  IdIndexPair* direct_method_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                         direct_methods_sz);
  IdIndexPair* virtual_method_offs = (IdIndexPair*)malloc(sizeof(IdIndexPair) *
                                                          virtual_methods_sz);
  dx_uint static_fields_pos = 0;
  dx_uint instance_fields_pos = 0;
  dx_uint direct_methods_pos = 0;
  dx_uint virtual_methods_pos = 0;

  dx_uint i;
  for(i = 0; i < static_fields_sz; i++) {
    fld = cl->static_fields + i;
    raw_field rf;
    rf.defining_class = cl->name;
    rf.name = fld->name;
    rf.type = fld->type;
    static_field_offs[static_fields_pos].id = find_field(pool, rf);
    static_field_offs[static_fields_pos++].index = i;
  }
  for(i = 0; i < instance_fields_sz; i++) {
    fld = cl->instance_fields + i;
    raw_field rf;
    rf.defining_class = cl->name;
    rf.name = fld->name;
    rf.type = fld->type;
    instance_field_offs[instance_fields_pos].id = find_field(pool, rf);
    instance_field_offs[instance_fields_pos++].index = i;
  }
  for(i = 0; i < direct_methods_sz; i++) {
    mtd = cl->direct_methods + i;
    raw_method rm;
    rm.defining_class = cl->name;
    rm.name = mtd->name;
    rm.prototype = mtd->prototype;
    direct_method_offs[direct_methods_pos].id = find_method(pool, rm);
    direct_method_offs[direct_methods_pos++].index = i;
  }
  for(i = 0; i < virtual_methods_sz; i++) {
    mtd = cl->virtual_methods + i;
    raw_method rm;
    rm.defining_class = cl->name;
    rm.name = mtd->name;
    rm.prototype = mtd->prototype;
    virtual_method_offs[virtual_methods_pos].id = find_method(pool, rm);
    virtual_method_offs[virtual_methods_pos++].index = i;
  }
  qsort(static_field_offs, static_fields_sz, sizeof(IdIndexPair),
        compare_idindexpair);
  qsort(instance_field_offs, instance_fields_sz, sizeof(IdIndexPair),
        compare_idindexpair);
  qsort(direct_method_offs, direct_methods_sz, sizeof(IdIndexPair),
        compare_idindexpair);
  qsort(virtual_method_offs, virtual_methods_sz, sizeof(IdIndexPair),
        compare_idindexpair);

  data_item d = init_data_item(TYPE_CLASS_DATA_ITEM);

  write_uleb(&d, static_fields_sz);
  write_uleb(&d, instance_fields_sz);
  write_uleb(&d, direct_methods_sz);
  write_uleb(&d, virtual_methods_sz);
  dx_uint last_id = 0;

  for(i = 0; i < static_fields_sz; i++) {
    fld = cl->static_fields + static_field_offs[i].index;
    dx_uint next_id = static_field_offs[i].id;
    write_uleb(&d, next_id - last_id);
    write_uleb(&d, fld->access_flags);
    last_id = next_id;
  }
  last_id = 0;
  for(i = 0; i < instance_fields_sz; i++) {
    fld = cl->instance_fields + instance_field_offs[i].index;
    dx_uint next_id = instance_field_offs[i].id;
    write_uleb(&d, next_id - last_id);
    write_uleb(&d, fld->access_flags);
    last_id = next_id;
  }
  last_id = 0;
  for(i = 0; i < direct_methods_sz; i++) {
    mtd = cl->direct_methods + direct_method_offs[i].index;
    dx_uint next_id = direct_method_offs[i].id;
    write_uleb(&d, next_id - last_id);
    write_uleb(&d, mtd->access_flags);
    if(mtd->code_body) {
      write_uleb_resolve(&d, write_code_item(ctx, mtd->code_body));
    } else {
      write_uleb(&d, 0);
    }
    last_id = next_id;
  }
  last_id = 0;
  for(i = 0; i < virtual_methods_sz; i++) {
    mtd = cl->virtual_methods + virtual_method_offs[i].index;
    dx_uint next_id = virtual_method_offs[i].id;
    write_uleb(&d, next_id - last_id);
    write_uleb(&d, mtd->access_flags);
    if(mtd->code_body) {
      write_uleb_resolve(&d, write_code_item(ctx, mtd->code_body));
    } else {
      write_uleb(&d, 0);
    }
    last_id = next_id;
  }

  free(static_field_offs);
  free(instance_field_offs);
  free(direct_method_offs);
  free(virtual_method_offs);
  return add_data(ctx, d);
}

static
dx_uint write_type_list(write_context* ctx, ref_str** type_list) {
  data_item d = init_data_item(TYPE_TYPE_LIST);
  dx_uint sz = 0;
  ref_str** s;
  for(s = type_list; *s; s++) sz++;
  
  write_uint(&d, sz);
  for(s = type_list; *s; s++) {
    write_ushort(&d, uint2ushort(find_type(&ctx->pool, *s)));
  }
  return add_data(ctx, d);
}

static
dx_uint write_string_data(write_context* ctx, ref_str* s) {
  data_item d = init_data_item(TYPE_STRING_DATA_ITEM);
  dx_uint code_points = mutf8_code_points(s->s);
  if(code_points == (dx_uint)-1) {
    fprintf(stderr, "Invalid MUTF-8 encoding\n");
    fflush(stderr);
  }
  write_uleb(&d, code_points);
  char* ptr;
  for(ptr = s->s; *ptr; ptr++) write_ubyte(&d, *ptr);
  write_ubyte(&d, 0);
  return add_data(ctx, d);
}

static
void write_constant_pool(write_context* ctx) {
  constant_pool* pool = &ctx->pool;
  dx_uint i;
  for(i = 0; i < pool->strs_size; i++) {
    data_item d = init_data_item(TYPE_STRING_ID_ITEM);
    write_resolve(&d, write_string_data(ctx, pool->strs[i]));
    add_data(ctx, d);
  }
  for(i = 0; i < pool->types_size; i++) {
    data_item d = init_data_item(TYPE_TYPE_ID_ITEM);
    write_uint(&d, find_str(pool, pool->types[i]));
    add_data(ctx, d);
  }
  for(i = 0; i < pool->protos_size; i++) {
    data_item d = init_data_item(TYPE_PROTO_ID_ITEM);
    ref_str* shrty = getShorty(pool->protos[i]);
    write_uint(&d, find_str(pool, shrty));
    dxc_free_str(shrty);
    write_uint(&d, find_type(pool, pool->protos[i]->s[0]));
    if(!pool->protos[i]->s[1]) {
      write_uint(&d, 0);
    } else {
      write_resolve(&d, write_type_list(ctx, pool->protos[i]->s + 1));
    }
    add_data(ctx, d);
  }
  for(i = 0; i < pool->fields_size; i++) {
    raw_field fld = pool->fields[i];
    data_item d = init_data_item(TYPE_FIELD_ID_ITEM);
    write_ushort(&d, uint2ushort(find_type(pool, fld.defining_class)));
    write_ushort(&d, uint2ushort(find_type(pool, fld.type)));
    write_uint(&d, find_str(pool, fld.name));
    add_data(ctx, d);
  }
  for(i = 0; i < pool->methods_size; i++) {
    raw_method mtd = pool->methods[i];
    data_item d = init_data_item(TYPE_METHOD_ID_ITEM);
    write_ushort(&d, uint2ushort(find_type(pool, mtd.defining_class)));
    write_ushort(&d, uint2ushort(find_proto(pool, mtd.prototype)));
    write_uint(&d, find_str(pool, mtd.name));
    add_data(ctx, d);
  }
}

static
void write_class(write_context* ctx, DexClass* cl) {
  constant_pool* pool = &ctx->pool;
  data_item d = init_data_item(TYPE_CLASS_DEF_ITEM);

  write_uint(&d, find_type(pool, cl->name));
  write_uint(&d, cl->access_flags);
  write_uint(&d, cl->super_class ? find_type(pool, cl->super_class) : NO_INDEX);
  if(!cl->interfaces->s[0]) {
    write_uint(&d, 0);
  } else {
    write_resolve(&d, write_type_list(ctx, cl->interfaces->s));
  }
  write_uint(&d, cl->source_file ? find_str(pool, cl->source_file) :
                                   NO_INDEX);
  write_resolve(&d, write_annotation_directory(ctx, cl));
  if(dxc_is_sentinel_field(cl->static_fields) &&
     dxc_is_sentinel_field(cl->instance_fields) &&
     dxc_is_sentinel_method(cl->direct_methods) &&
     dxc_is_sentinel_method(cl->virtual_methods)) {
    write_uint(&d, 0);
  } else {
    write_resolve(&d, write_class_data(ctx, cl));
  }
  write_resolve(&d, write_encoded_array_item(ctx, cl->static_values));

  add_data(ctx, d);
}

static
void write_classes(write_context* ctx, DexClass* classes) {
  constant_pool* pool = &ctx->pool;
  int sz = 0;
  DexClass* cl;
  DexClass** cl_type = (DexClass**)calloc(sizeof(DexClass*), pool->types_size);
  for(cl = classes; !dxc_is_sentinel_class(cl); cl++) {
    sz++;
    cl_type[find_type(pool, cl->name)] = cl;
  }

  IdIndexPair* stck = (IdIndexPair*)calloc(sizeof(IdIndexPair), sz + 1);
  for(cl = classes; !dxc_is_sentinel_class(cl); cl++) {
    int spos = 0;
    stck[spos].id = find_type(pool, cl->name);
    stck[spos].index = -1;
    while(spos >= 0) {
      int id = stck[spos].id;
      int index = stck[spos].index++;
      DexClass* cl = cl_type[id];
      if(!cl) {
        spos--;
        continue;
      }
      if(index == -1 && !cl->super_class) {
        stck[spos].index++;
        index++;
      }
      if(index == -1) {
        stck[++spos].id = find_type(pool, cl->super_class);
        stck[spos].index = -1;
      } else if(cl->interfaces->s[index]) {
        stck[++spos].id = find_type(pool, cl->interfaces->s[index]);
        stck[spos].index = -1;
      } else {
        write_class(ctx, cl);
        cl_type[id] = NULL;
        spos--;
      }
    }
  }

  free(stck);
  free(cl_type);
}

static
void write_to_file(FILE* fout, data_item d) {
  if(d.data_sz != fwrite(d.data, 1, d.data_sz, fout)) {
    fprintf(stderr, "failed to write all of file contents\n");
    fflush(stderr);
  }
}

#define make_unique(A, sz, compare_func_raw, compare_func, free_func) { \
  qsort(A, sz, sizeof(*A), compare_func_raw); \
  for(i = pos = 0; i < sz; i++) { \
    if(!pos || compare_func(A[pos - 1], A[i])) { \
      A[pos++] = A[i]; \
    } else { \
      free_func(A[i]); \
    } \
  } \
  sz = pos; \
}

typedef struct {
  int type;
  int offset;
  int size;
} layout_list_item;

typedef struct  {
  int index;
  int crc;
} ord_data_item;

static
int compare_crcs(const void* a, const void* b) {
  ord_data_item* da = (ord_data_item*)a;
  ord_data_item* db = (ord_data_item*)b;
  if(da->crc < db->crc) return -1;
  if(da->crc > db->crc) return 1;
  return 0;
}

// TODO: Do this right.
#include "opt_write.h"

static
void perform_resolve(write_context* ctx, data_item* d, int* offsets) {
  dx_uint i;
  for(i = 0; d->resolve && i < d->resolve->sz; i++) {
    int pos = d->resolve->dat[i].offset;
    int ind = d->resolve->dat[i].index;
    dx_uint val = offsets[ind];

    if(d->data[pos]) {
      // This is an offset encoded as a uleb.  To make things simpler we have
      // already allocated 4 bytes for this field.  If the offset is larger
      // than 2^28 (256 MiB) we may have a problem.
      int k;
      if(pos >= 1 << 28) {
        fprintf(stderr, "offset too large encoded as uleb\n");
        fflush(stderr);
      }
      for(k = 0; k < 4; k++) {
        d->data[pos + k] = (k != 3 ? 0x80 : 0x00) | (val >> 7 * k & 0x7F);
      }
    } else {
      int k;
      for(k = 0; k < 4; k++) {
        d->data[pos + k] = val >> 8 * k & 0xFF;
      }
    }
  }
  free(d->resolve);
  d->resolve = NULL;
}

void dxc_write_file(DexFile* dex, FILE* fout) {
  write_context ctx;
  init_ctx(&ctx);
  constant_pool* pool = &ctx.pool;
  pop_array(dex->classes, pool, dxc_is_sentinel_class, pop_class);

  int pos;
  dx_uint i;
  make_unique(pool->methods, pool->methods_size, compare_method_ptr,
              compare_method, dxc_free_raw_method);
  for(i = 0; i < pool->methods_size; i++) {
    add_type(pool, pool->methods[i].defining_class);
    add_str(pool, pool->methods[i].name);
    add_proto(pool, pool->methods[i].prototype);
  }
  make_unique(pool->fields, pool->fields_size, compare_field_ptr,
              compare_field, dxc_free_raw_field);
  for(i = 0; i < pool->fields_size; i++) {
    add_type(pool, pool->fields[i].defining_class);
    add_str(pool, pool->fields[i].name);
    add_type(pool, pool->fields[i].type);
  }
  make_unique(pool->protos, pool->protos_size, compare_proto_ptr,
              compare_proto, dxc_free_strstr);
  for(i = 0; i < pool->protos_size; i++) {
    ref_str** s;
    for(s = pool->protos[i]->s; *s; s++) {
      add_type(pool, *s);
    }
    ref_str* shrty = getShorty(pool->protos[i]);
    add_str(pool, shrty);
    dxc_free_str(shrty);
  }
  make_unique(pool->types, pool->types_size, compare_mutf8_ptr,
              mutf8_ref_compare, dxc_free_str);
  for(i = 0; i < pool->types_size; i++) {
    add_str(pool, pool->types[i]);
  }
  make_unique(pool->strs, pool->strs_size, compare_mutf8_ptr,
              mutf8_ref_compare, dxc_free_str);

  write_constant_pool(&ctx);
  write_classes(&ctx, dex->classes);

  int* alignment_mp = (int*)malloc(sizeof(int) * TYPE_LAST);
  alignment_mp[TYPE_HEADER_ITEM] = 4;
  alignment_mp[TYPE_STRING_ID_ITEM] = 4;
  alignment_mp[TYPE_TYPE_ID_ITEM] = 4;
  alignment_mp[TYPE_PROTO_ID_ITEM] = 4;
  alignment_mp[TYPE_FIELD_ID_ITEM] = 4;
  alignment_mp[TYPE_METHOD_ID_ITEM] = 4;
  alignment_mp[TYPE_CLASS_DEF_ITEM] = 4;
  alignment_mp[TYPE_MAP_LIST] = 4;
  alignment_mp[TYPE_TYPE_LIST] = 4;
  alignment_mp[TYPE_ANNOTATION_SET_REF_LIST] = 4;
  alignment_mp[TYPE_ANNOTATION_SET_ITEM] = 4;
  alignment_mp[TYPE_CLASS_DATA_ITEM] = 1;
  alignment_mp[TYPE_CODE_ITEM] = 4;
  alignment_mp[TYPE_STRING_DATA_ITEM] = 1;
  alignment_mp[TYPE_DEBUG_INFO_ITEM] = 1;
  alignment_mp[TYPE_ANNOTATION_ITEM] = 1;
  alignment_mp[TYPE_ENCODED_ARRAY_ITEM] = 1;
  alignment_mp[TYPE_ANNOTATIONS_DIRECTORY_ITEM] = 4;

  dx_uint* type_list_sz = (dx_uint*)calloc(sizeof(dx_uint), TYPE_LAST);
  dx_uint** type_map = (dx_uint**)malloc(sizeof(dx_uint*) * TYPE_LAST);

  // Time to actually layout the file and resolve offsets.
  dx_uint n = ctx.dat_sz;
  dx_uint max_reassign_sz = 0;
  for(i = 0; i < n; i++) {
    type_list_sz[ctx.dat[i].type]++;
  }
  for(i = 0; i < TYPE_LAST; i++) {
    if(TYPE_CLASS_DEF_ITEM < i) {
      max_reassign_sz = max_reassign_sz > type_list_sz[i] ? max_reassign_sz :
                                                            type_list_sz[i];
    }
    if(type_list_sz[i]) {
      type_map[i] = (dx_uint*)malloc(sizeof(dx_uint) * type_list_sz[i]);
    }
    type_list_sz[i] = 0;
  }
  for(i = 0; i < n; i++) {
    dx_uint typ = ctx.dat[i].type;
    type_map[typ][type_list_sz[typ]++] = i;
  }
  
  dx_uint off = 0x70;
  int* offsets = (int*)calloc(sizeof(int), n);
  for(i = TYPE_STRING_ID_ITEM; i <= TYPE_CLASS_DEF_ITEM; i++) {
    int algn = alignment_mp[i];
    dx_uint j;
    for(j = 0; j < type_list_sz[i]; j++) {
      dx_uint ind = type_map[i][j];
      while(off % algn != 0) off++;
      offsets[ind] = off;
      off += ctx.dat[ind].data_sz;
    }
  }

  ord_data_item* data_ord =
      (ord_data_item*)malloc(sizeof(ord_data_item) * max_reassign_sz);
  dx_uint iter;
  for(iter = 0; iter < sizeof(resolve_order) / sizeof(DexItemTypes); iter++) {
    i = resolve_order[iter];
    if(!type_list_sz[i]) continue;
    dx_uint j;
    for(j = 0; j < type_list_sz[i]; j++) {
      int jj = type_map[i][j];
      perform_resolve(&ctx, ctx.dat + jj, offsets);
      offsets[jj] = -1;
      data_ord[j].index = jj;
      data_ord[j].crc = dxc_checksum(ctx.dat[jj].data, ctx.dat[jj].data_sz);
    }
    qsort(data_ord, type_list_sz[i], sizeof(ord_data_item), compare_crcs);
    if(i == TYPE_CODE_ITEM) {
    } else for(j = 0; j < type_list_sz[i]; j++) {
      int jj = data_ord[j].index;
      if(offsets[jj] != -1) continue;
      dx_uint k;
      for(k = j + 1;
          k < type_list_sz[i] && data_ord[j].crc == data_ord[k].crc; k++) {
        int kk = data_ord[k].index;
        if(ctx.dat[jj].data_sz == ctx.dat[kk].data_sz && offsets[kk] == -1 &&
           !memcmp(ctx.dat[jj].data, ctx.dat[kk].data, ctx.dat[jj].data_sz)) {
          offsets[kk] = jj;
        }
      }
    }
    int pos = 0;
    int algn = alignment_mp[i];
    for(j = 0; j < type_list_sz[i]; j++) {
      int jj = data_ord[j].index;
      if(offsets[jj] == -1) {
        type_map[i][pos++] = jj;
        while(off % algn != 0) off++;
        offsets[jj] = off;
        off += ctx.dat[jj].data_sz;
      } else {
        offsets[jj] = offsets[offsets[jj]];
        free_data_item(ctx.dat[jj]);
      }
    }
    type_list_sz[i] = pos;
  }
  free(data_ord);
  while(off % 4 != 0) off++;

  for(i = TYPE_STRING_ID_ITEM; i <= TYPE_CLASS_DEF_ITEM; i++) {
    dx_uint j;
    for(j = 0; j < type_list_sz[i]; j++) {
      perform_resolve(&ctx, ctx.dat + type_map[i][j], offsets);
    }
  }

  dx_uint map_off = off;
  dx_uint str_off = 0;
  dx_uint type_off = 0;
  dx_uint proto_off = 0;
  dx_uint field_off = 0;
  dx_uint method_off = 0;
  dx_uint class_off = 0;
  dx_uint data_off = 0;

  // Build map structure.
  dx_uint layout_size = 2;
  dx_uint layout_pos = 0;
  for(i = 0; i < TYPE_LAST; i++) if(type_list_sz[i]) layout_size++;
  layout_list_item* layout_list =
      (layout_list_item*)malloc(sizeof(layout_list_item) * layout_size);

  layout_list[layout_pos].type = TYPE_HEADER_ITEM;
  layout_list[layout_pos].offset = 0;
  layout_list[layout_pos++].size = 1;
  for(i = TYPE_STRING_ID_ITEM; i <= TYPE_CLASS_DEF_ITEM; i++) {
    if(!type_list_sz[i]) continue;
    layout_list[layout_pos].type = i;
    layout_list[layout_pos].offset = offsets[type_map[i][0]];
    layout_list[layout_pos++].size = type_list_sz[i];
  }
  for(iter = 0; iter < sizeof(resolve_order) / sizeof(DexItemTypes); iter++) {
    i = resolve_order[iter];
    if(!type_list_sz[i]) continue;
    layout_list[layout_pos].type = i;
    layout_list[layout_pos].offset = offsets[type_map[i][0]];
    layout_list[layout_pos++].size = type_list_sz[i];
  }
  layout_list[layout_pos].type = TYPE_MAP_LIST;
  layout_list[layout_pos].offset = map_off;
  layout_list[layout_pos++].size = 1;

  data_item map_data = init_data_item(TYPE_MAP_LIST);
  write_uint(&map_data, layout_size);
  for(i = 0; i < layout_size; i++) {
    write_ushort(&map_data, layout_list[i].type);
    write_ushort(&map_data, 0); // unused
    write_uint(&map_data, layout_list[i].size);
    write_uint(&map_data, layout_list[i].offset);

    switch(layout_list[i].type) {
      case TYPE_HEADER_ITEM: break;
      case TYPE_STRING_ID_ITEM: str_off = layout_list[i].offset; break;
      case TYPE_TYPE_ID_ITEM: type_off = layout_list[i].offset; break;
      case TYPE_PROTO_ID_ITEM: proto_off = layout_list[i].offset; break;
      case TYPE_FIELD_ID_ITEM: field_off = layout_list[i].offset; break;
      case TYPE_METHOD_ID_ITEM: method_off = layout_list[i].offset; break;
      case TYPE_CLASS_DEF_ITEM: class_off = layout_list[i].offset; break;
      default:
        if(!data_off) {
          data_off = layout_list[i].offset;
        }
    }
  }

  // Write out the file (except for the header) to one contiguous memory buffer.
  data_item file = init_data_item(0);
  for(i = TYPE_STRING_ID_ITEM; i <= TYPE_CLASS_DEF_ITEM; i++) {
    int algn = alignment_mp[i];
    dx_uint j;
    for(j = 0; j < type_list_sz[i]; j++) {
      while(file.data_sz % algn != 0) write_ubyte(&file, 0);
      concat_data_and_free(&file, ctx.dat + type_map[i][j]);
    }
  }
  for(iter = 0; iter < sizeof(resolve_order) / sizeof(DexItemTypes); iter++) {
    int i = resolve_order[iter];
    int algn = alignment_mp[i];
    dx_uint j;
    for(j = 0; j < type_list_sz[i]; j++) {
      while(file.data_sz % algn != 0) write_ubyte(&file, 0);
      concat_data_and_free(&file, ctx.dat + type_map[i][j]);
    }
  }
  while(file.data_sz % 4 != 0) write_ubyte(&file, 0);
  concat_data_and_free(&file, &map_data);

  dx_uint dex_file_size = 0x70 + file.data_sz;
  if(dex->metadata) {
    data_item deps_section = write_deps(&ctx, dex);
    data_item aux_section = write_aux(&ctx, dex, &file,
        type_list_sz[TYPE_CLASS_DEF_ITEM], class_off, type_off, str_off);
    data_item opt_header = init_data_item(0);
  
    char opt_magic[8];
    snprintf(opt_magic, 8, "dey\n%03d", dex->metadata->odex_version);
    for(i = 0; i < 8; i++) {
      write_ubyte(&opt_header, opt_magic[i]);
    }

    write_uint(&opt_header, 0x28);
    write_uint(&opt_header, dex_file_size);
    write_uint(&opt_header, 0x28 + dex_file_size);
    write_uint(&opt_header, deps_section.data_sz);

    /* The aux seciton must be aligned on 8 byte boundaries. */
    while((dex_file_size + deps_section.data_sz) & 7) {
      write_ubyte(&deps_section, 0);
    }

    write_uint(&opt_header, 0x28 + dex_file_size + deps_section.data_sz);
    write_uint(&opt_header, aux_section.data_sz);
    write_uint(&opt_header, dex->metadata->flags);
    
    concat_data_and_free(&deps_section, &aux_section);
    write_uint(&opt_header,
               dxc_checksum(deps_section.data, deps_section.data_sz));
    concat_data_and_free(&file, &deps_section);

    write_to_file(fout, opt_header);
    free_data_item(opt_header);
  }

  // Compute the header, this is sort of complicated as the contents depend
  // on the contents of the rest of the file including some of the header
  // itself.
  data_item header = init_data_item(TYPE_HEADER_ITEM);
  write_uint(&header, dex_file_size);
  write_uint(&header, 0x70); // header_size
  write_uint(&header, 0x12345678); // ENDIAN_CONSTANT
  // TODO: there is really no reason we can't support the link table.
  write_uint(&header, 0); // link_size (no link table supported)
  write_uint(&header, 0); // link_off (no link table supported)
  write_uint(&header, map_off); // map_off (map is not supported)
  write_uint(&header, pool->strs_size);
  write_uint(&header, str_off);
  write_uint(&header, pool->types_size);
  write_uint(&header, type_off);
  write_uint(&header, pool->protos_size);
  write_uint(&header, proto_off);
  write_uint(&header, pool->fields_size);
  write_uint(&header, field_off);
  write_uint(&header, pool->methods_size);
  write_uint(&header, method_off);
  write_uint(&header, type_list_sz[TYPE_CLASS_DEF_ITEM]);
  write_uint(&header, class_off);
  write_uint(&header, 0x70 + file.data_sz - data_off);
  write_uint(&header, data_off);

  dx_uint hdr_sz = header.data_sz;
  concat_data_and_free(&header, &file);
  file = header;
  header = init_data_item(TYPE_HEADER_ITEM);
  
  dx_ubyte* sha1 = dex->metadata ? dex->metadata->id :
                   dxc_calc_sha1(file.data, hdr_sz + dex_file_size - 0x70);
  for(i = 0; i < 20; i++) {
    write_ubyte(&header, sha1[i]);
  }
  if(!dex->metadata) free(sha1);

  hdr_sz += header.data_sz;
  concat_data_and_free(&header, &file);
  file = header;
  header = init_data_item(TYPE_HEADER_ITEM);

  dx_uint crc = dxc_checksum(file.data, hdr_sz + dex_file_size - 0x70);
  for(i = 0; i < 8; i++) {
    write_ubyte(&header, "dex\n035\x0"[i]);
  }
  write_uint(&header, crc);

  write_to_file(fout, header);
  free_data_item(header);
  write_to_file(fout, file);
  free_data_item(file);

  free(alignment_mp);
  for(i = 0; i < TYPE_LAST; i++) {
    if(type_list_sz[i]) free(type_map[i]);
  }
  free(type_map);
  free(type_list_sz);
  free(offsets);
  free(layout_list);
  free_ctx(ctx);
}

#undef make_unique
