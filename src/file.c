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
#include "file.h"

#include <stdlib.h>
#include <string.h>
#include <dxcut/file.h>
#include <dxcut/field.h>

#include "aux.h"
#include "classes.h"
#include "fields.h"
#include "methods.h"
#include "protos.h"
#include "read.h"
#include "strings.h"
#include "types.h"

static
const char DEX_MAGIC[8] = {'d', 'e', 'x', '\n'};

static
const char ODEX_MAGIC[8] = {'d', 'e', 'y', '\n'};

static
const dx_uint ADLER_CHECKSUM_MODULUS = 65521;

dx_uint dxc_checksum(const void* vbuf, int size) {
  dx_ubyte* buf = (dx_ubyte*)vbuf;
  dx_uint A = 1;
  dx_uint B = 0;
  int i;
  for(i = 0; i < size; i++) {
    A += buf[i];
    if(A >= ADLER_CHECKSUM_MODULUS) A -= ADLER_CHECKSUM_MODULUS;
    B += A;
    if(B >= ADLER_CHECKSUM_MODULUS) B -= ADLER_CHECKSUM_MODULUS;
  }
  return B << 16 | A;
}

dx_ubyte* dxc_calc_sha1(const void* vbuf, int size) {
  dx_ubyte* A = (dx_ubyte*)vbuf;
  dx_uint w[80];

  int size_start = (size + 9) / 64 * 64 + 56;
  dx_uint h[5] = {
    0x67452301,
    0xEFCDAB89,
    0x98BADCFE,
    0x10325476,
    0xC3D2E1F0
  };
  dx_ulong llsize = 8LL * size;

  int i;
  for(i = 0; i < size_start + 8; ) {
    int j;
    for(j = 0; j < 16; j++) {
      int k = 0;
      for(k = 0; k < 4; k++, i++) {
        dx_ubyte v;
        if(i < size) {
          v = A[i];
        } else if(i == size) {
          v = 0x80;
        } else if(i < size_start) {
          v = 0;
        } else {
          v = llsize >> 8LL * (7LL - i + size_start) & 0xFF;
        }
        w[j] = w[j] << 8;
        w[j] |= v;
      }
    }
    for(j = 16; j < 80; j++) {
      w[j] = w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16];
      w[j] = w[j] << 1 | w[j] >> 31;
    }
    dx_uint a = h[0];
    dx_uint b = h[1];
    dx_uint c = h[2];
    dx_uint d = h[3];
    dx_uint e = h[4];
    for(j = 0; j < 80; j++) {
      dx_uint f;
      dx_uint k;
      if(j < 20) {
        f = (b & c) | (~b & d);
        k = 0x5A827999;
      } else if(j < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if(j < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      dx_uint v = (a << 5 | a >> 27) + f + e + k + w[j];
      e = d;
      d = c;
      c = (b << 30 | b >> 2);
      b = a;
      a = v;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
  }
  dx_ubyte* res = (dx_ubyte*)malloc(20);
  int pos = 0;
  for(i = 0; i < 5; i++) {
    int j;
    for(j = 24; j >= 0; j -= 8) {
      res[pos++] = h[i] >> j & 0xFF;
    }
  }
  return res;
}

void dxc_free_odex_data(OdexData* data) {
  if(!data) return;
  if(data->dep_shas) {
    dx_ubyte** ptr = data->dep_shas;
    for(ptr = data->dep_shas; *ptr; ptr++) free(*ptr);
    free(data->dep_shas);
  }
  dxc_free_strstr(data->deps);
  free(data->id);
  free(data);
}

static
void free_context(read_context* ctx) {
  dx_uint i;
  if(ctx->strs) {
    for(i = 0; i < ctx->strs_sz; i++) dxc_free_str(ctx->strs[i]);
    free(ctx->strs);
  }
  if(ctx->types) {
    for(i = 0; i < ctx->types_sz; i++) dxc_free_str(ctx->types[i]);
    free(ctx->types);
  }
  if(ctx->protos) {
    for(i = 0; i < ctx->protos_sz; i++) dxc_free_strstr(ctx->protos[i]);
    free(ctx->protos);
  }
  if(ctx->fields) {
    for(i = 0; i < ctx->fields_sz; i++) {
      raw_field fld = ctx->fields[i];
      dxc_free_raw_field(fld);
      if(fld.annotations) {
        DexAnnotation* an;
        for(an = fld.annotations; !dxc_is_sentinel_annotation(an); an++) {
          dxc_free_annotation(an);
        }
        free(fld.annotations);
      }
    }
    free(ctx->fields);
  }
  if(ctx->methods) {
    for(i = 0; i < ctx->methods_sz; i++) {
      raw_method mtd = ctx->methods[i];
      dxc_free_raw_method(mtd);
      if(mtd.annotations) {
        DexAnnotation* an;
        for(an = mtd.annotations; !dxc_is_sentinel_annotation(an); an++) {
          dxc_free_annotation(an);
        }
        free(mtd.annotations);
      }
      if(mtd.parameter_annotations) {
        DexAnnotation** pan;
        for(pan = mtd.parameter_annotations; *pan; pan++) {
          DexAnnotation* an;
          for(an = *pan; !dxc_is_sentinel_annotation(an); an++) {
            dxc_free_annotation(an);
          }
          free(*pan);
        }
        free(mtd.parameter_annotations);
      }
    }
    free(ctx->methods);
  }
  if(ctx->classes) {
    for(i = 0; i < ctx->classes_sz; i++) {
      dxc_free_class(ctx->classes + i);
    }
    free(ctx->classes);
  }
}

DexFile* dxc_read_file(FILE* fin) {
  if(!fin) return NULL;
  fseek(fin, 0, SEEK_END);
  size_t size = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  void* buf = malloc(size);
  if(fread(buf, size, 1, fin) != 1) {
    DXC_ERROR("failed to read file into buffer");
    return NULL;
  }
  DexFile* res = dxc_read_buffer(buf, size);
  free(buf);
  return res;
}

static
dx_uint get_version(char* buf) {
  if(buf[3]) return -1;
  if(buf[0] < '0' || '9' < buf[0]) return -1;
  if(buf[1] < '0' || '9' < buf[1]) return -1;
  if(buf[2] < '0' || '9' < buf[2]) return -1;
  return (buf[0] - '0') * 100 + (buf[1] - '0') * 10 + buf[2] - '0';
}

DexFile* dxc_read_buffer(void* vbuf, dx_uint size) {
  if(size < 0x70) {
    DXC_ERROR("invalid file size");
    return NULL;
  }

  read_context ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.buf = (char*)vbuf;
  fill_read_functions(&ctx);
  dx_uint pos = 0;

  /* Check for file magic header. */
  dx_uint i;
  OdexData* metadata = NULL;
  if(!memcmp(ctx.buf, ODEX_MAGIC, 4)) {
    if(!(metadata = (OdexData*)calloc(1, sizeof(OdexData)))) {
      DXC_ERROR("failed to alloc odex data");
      return NULL;
    }
    ctx.odex_version = get_version(ctx.buf + 4);
    metadata->odex_version = ctx.odex_version;
    if(ctx.odex_version == (dx_uint)-1) {
      DXC_ERROR("invalid odex version");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    switch(ctx.odex_version) {
      case 35: break;
      case 36: break;
      default:
        DXC_ERROR("unknown odex version");
        dxc_free_odex_data(metadata);
        return NULL;
    }
    pos += 8;

    dx_uint dex_off = ctx.read_uint(&ctx, &pos);
    dx_uint dex_len = ctx.read_uint(&ctx, &pos);
    dx_uint deps_off = ctx.read_uint(&ctx, &pos);
    dx_uint deps_len = ctx.read_uint(&ctx, &pos);
    dx_uint aux_off = ctx.read_uint(&ctx, &pos);
    dx_uint aux_len = ctx.read_uint(&ctx, &pos);
    metadata->flags = ctx.read_uint(&ctx, &pos);
    dx_uint crc = ctx.read_uint(&ctx, &pos);

    if(dex_off + dex_len > size) {
      DXC_ERROR("dex file leaves file boundary");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    if(dex_len < 0x70) {
      DXC_ERROR("invalid embedded dex file size");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    if(deps_off != 0 && deps_off + deps_len > size) {
      DXC_ERROR("dependency list leaves file boundary");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    if(aux_off != 0 && aux_off + aux_len > size) {
      DXC_ERROR("auxilarry data leaves file boundary");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    if(aux_off < deps_off) {
      DXC_ERROR("deps section must come before aux section");
      dxc_free_odex_data(metadata);
      return NULL;
    }

    if(crc != 0xFFFFFFFF) {
      dx_uint crc_start = deps_off ? deps_off : aux_off;
      dx_uint crc_end = aux_off ? aux_off + aux_len :
                        (deps_off ? deps_off + deps_len : 0);
      dx_uint csum_actual = dxc_checksum(ctx.buf + crc_start,
                                         crc_end - crc_start);
      if(csum_actual != crc) {
        DXC_ERROR("invalid odex checksum");
        dxc_free_odex_data(metadata);
        return NULL;
      }
    }

    if(deps_off != 0 && !dxc_read_deps_table(&ctx, metadata,
                                             deps_off, deps_len)) {
      dxc_free_odex_data(metadata);
      return NULL;
    }
    if(aux_off != 0 && !dxc_read_aux_table(&ctx, metadata,
                                           aux_off, aux_len)) {
      dxc_free_odex_data(metadata);
      return NULL;
    }

    ctx.odex_buf = ctx.buf;
    ctx.buf += dex_off;
    size = dex_len;
    pos = 0;
  }
  if(!memcmp(ctx.buf, DEX_MAGIC, 4)) {
    ctx.dex_version = get_version(ctx.buf + 4);
    if(ctx.dex_version == (dx_uint)-1) {
      DXC_ERROR("invalid dex version");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    switch(ctx.dex_version) {
      case 35: break;
      default:
        DXC_ERROR("unknown dex version");
        dxc_free_odex_data(metadata);
        return NULL;
    }
    pos += 8;
  } else {
    DXC_ERROR("unrecognized file type");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  /* Check if the checksum is correct. */
  dx_uint csum_expected = ctx.read_uint(&ctx, &pos);
  dx_uint csum_actual = dxc_checksum(ctx.buf + pos, size - pos);
  if(csum_expected != csum_actual) {
    DXC_ERROR("invalid dex checksum");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  if(!metadata) {
    /* Check if the sha1 hash is correct. */
    int is_match = 1;
    dx_ubyte* sha1_actual = dxc_calc_sha1(ctx.buf + pos + 20, size - pos - 20);
    for(i = 0; i < 20 && is_match; i++) {
      is_match = sha1_actual[i] == ctx.read_ubyte(&ctx, &pos);
    }
    free(sha1_actual);
    if(0 && !is_match) {
      DXC_ERROR("invalid sha1 hash");
      dxc_free_odex_data(metadata);
      return NULL;
    }
  } else {
    /* For odex files this serves more as an identifier and contains the sha
     * of the dex file prior to optimizations.
     */
    if(!(metadata->id = (dx_ubyte*)malloc(20))) {
      DXC_ERROR("failed to alloc odex id");
      dxc_free_odex_data(metadata);
      return NULL;
    }
    memcpy(metadata->id, ctx.buf + pos, 20);
    pos += 20;
  }

  dx_uint file_size = ctx.read_uint(&ctx, &pos);
  if(file_size != size) {
    DXC_ERROR("invalid file size");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  dx_uint header_size = ctx.read_uint(&ctx, &pos);
  if(header_size != 0x70) {
    DXC_ERROR("invalid header size");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  dx_uint endian_tag = ctx.read_uint(&ctx, &pos);
  if(endian_tag != 0x12345678) {
    DXC_ERROR("invalid endian tag");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  dx_uint link_size = ctx.read_uint(&ctx, &pos);
  dx_uint link_off = ctx.read_uint(&ctx, &pos);
  if(link_size != 0 || link_off != 0) {
    DXC_ERROR("file has link table, exiting");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  dx_uint map_off = ctx.read_uint(&ctx, &pos);

  dx_uint string_ids_size = ctx.read_uint(&ctx, &pos);
  dx_uint string_ids_off = ctx.read_uint(&ctx, &pos);
  dx_uint type_ids_size = ctx.read_uint(&ctx, &pos);
  dx_uint type_ids_off = ctx.read_uint(&ctx, &pos);
  dx_uint proto_ids_size = ctx.read_uint(&ctx, &pos);
  dx_uint proto_ids_off = ctx.read_uint(&ctx, &pos);
  dx_uint field_ids_size = ctx.read_uint(&ctx, &pos);
  dx_uint field_ids_off = ctx.read_uint(&ctx, &pos);
  dx_uint method_ids_size = ctx.read_uint(&ctx, &pos);
  dx_uint method_ids_off = ctx.read_uint(&ctx, &pos);
  dx_uint class_defs_size = ctx.read_uint(&ctx, &pos);
  dx_uint class_defs_off = ctx.read_uint(&ctx, &pos);
  dx_uint data_size = ctx.read_uint(&ctx, &pos);
  dx_uint data_off = ctx.read_uint(&ctx, &pos);

  ctx.data_off = data_off;
  ctx.data_sz = data_size;
  if(string_ids_size != 0 &&
     string_ids_off + STRING_ID_ELEMENT_SIZE * string_ids_size > size) {
    DXC_ERROR("string table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }
  if(type_ids_size != 0 &&
     type_ids_off + TYPE_ID_ELEMENT_SIZE * type_ids_size > size) {
    DXC_ERROR("type table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }
  if(proto_ids_size != 0 &&
     proto_ids_off + PROTO_ID_ELEMENT_SIZE * proto_ids_size > size) {
    DXC_ERROR("proto table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }
  if(field_ids_size != 0 &&
     field_ids_off + FIELD_ID_ELEMENT_SIZE * field_ids_size > size) {
    DXC_ERROR("field table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }
  if(method_ids_size != 0 &&
     method_ids_off + METHOD_ID_ELEMENT_SIZE * method_ids_size > size) {
    DXC_ERROR("method table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }
  if(class_defs_size != 0 &&
     class_defs_off + CLASS_DEF_ELEMENT_SIZE * class_defs_size > size) {
    DXC_ERROR("class def table not within file bounds");
    dxc_free_odex_data(metadata);
    return NULL;
  }

  if(!dxc_read_string_section(&ctx, string_ids_off, string_ids_size) ||
     !dxc_read_type_section(&ctx, type_ids_off, type_ids_size) ||
     !dxc_read_proto_section(&ctx, proto_ids_off, proto_ids_size) ||
     !dxc_read_field_section(&ctx, field_ids_off, field_ids_size) ||
     !dxc_read_method_section(&ctx, method_ids_off, method_ids_size) ||
     !dxc_read_class_section(&ctx, class_defs_off, class_defs_size)) {
    free_context(&ctx);
    dxc_free_odex_data(metadata);
    return NULL;
  }
  DexFile* ret = (DexFile*)calloc(1, sizeof(DexFile));
  if(!ret) {
    DXC_ERROR("file pointer alloc failed");
    free_context(&ctx);
    dxc_free_odex_data(metadata);
    return NULL;
  }

  ret->classes = ctx.classes;
  ret->metadata = metadata;
  ctx.classes = NULL;
  ctx.classes_sz = 0;

  ret->method_table_size = method_ids_size;
  ret->field_table_size = field_ids_size;
  ret->type_table_size = type_ids_size;
  ret->method_table = (ref_method*)malloc(sizeof(ref_method) *
                                             ret->method_table_size);
  ret->field_table = (ref_field*)malloc(sizeof(ref_field) *
                                           ret->field_table_size);
  ret->type_table = (ref_str**)malloc(sizeof(ref_str*) *
                                             ret->type_table_size);
  for(i = 0; i < ret->method_table_size; ++i) {
    ret->method_table[i].defining_class =
        dxc_copy_str(ctx.methods[i].defining_class);
    ret->method_table[i].prototype = dxc_copy_strstr(ctx.methods[i].prototype);
    ret->method_table[i].name = dxc_copy_str(ctx.methods[i].name);
  }
  for(i = 0; i < ret->field_table_size; ++i) {
    ret->field_table[i].defining_class =
        dxc_copy_str(ctx.fields[i].defining_class);
    ret->field_table[i].type = dxc_copy_str(ctx.fields[i].type);
    ret->field_table[i].name = dxc_copy_str(ctx.fields[i].name);
  }
  for(i = 0; i < ret->type_table_size; ++i) {
    ret->type_table[i] = dxc_copy_str(ctx.types[i]);
  }

  free_context(&ctx);
  return ret;
}

void dxc_free_file(DexFile* dex) {
  DexClass* cl;
  if(dex->classes) {
    for(cl = dex->classes; !dxc_is_sentinel_class(cl); cl++) {
      dxc_free_class(cl);
    }
  }
  dxc_free_odex_data(dex->metadata);
  free(dex->classes);
  free(dex);
}
