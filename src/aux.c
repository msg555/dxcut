#include "aux.h"

#include <stdlib.h>
#include <string.h>

#define AUX_TYPE_OLD 0
#define AUX_TYPE_NEW 1

static
int parse_index_map(read_context* ctx, dx_ubyte* data, int expanding) {
  DXC_ERROR("parse_index_map is unimplemented");
  return 0;
}

int dxc_read_deps_table(read_context* ctx, OdexData* metadata,
                        dx_uint deps_off, dx_uint deps_len) {
  dx_uint deps_end = deps_off + deps_len;
  if(deps_len < 16) {
    DXC_ERROR("deps section too small");
    return 0;
  }
  metadata->dex_mod_time = ctx->read_uint(ctx, &deps_off);
  metadata->dex_crc = ctx->read_uint(ctx, &deps_off);
  metadata->vm_version = ctx->read_uint(ctx, &deps_off);
  dx_uint sz = ctx->read_uint(ctx, &deps_off);
  
  dx_uint i;
  if(!(metadata->deps = dxc_create_strstr(sz)) ||
     !(metadata->dep_shas = (dx_ubyte**)calloc(sz + 1, sizeof(dx_ubyte*)))) {
    DXC_ERROR("deps alloc failed");
    return 0;
  }
  for(i = 0; i < sz; i++) {
    if(deps_off + 4 > deps_end) {
      DXC_ERROR("deps leaves deps section");
      return 0;
    }
    dx_uint ln = ctx->read_uint(ctx, &deps_off);
    if(deps_off + ln + 20 > deps_end) {
      DXC_ERROR("deps leaves deps section");
      return 0;
    }

    if(ctx->buf[deps_off + ln - 1] != 0) {
      DXC_ERROR("dependency does not end in null");
      return 0;
    }
    if(!(metadata->deps->s[i] = dxc_induct_str(ctx->buf + deps_off)) ||
       !(metadata->dep_shas[i] = (dx_ubyte*)malloc(20))) {
      DXC_ERROR("dependency alloc failed");
      return 0;
    }
    deps_off += ln;
    memcpy(metadata->dep_shas[i], ctx->buf + deps_off, 20);
    deps_off += 20;
  }
  return 1;
}

int dxc_read_aux_table(read_context* ctx, OdexData* metadata,
                       dx_uint aux_off, dx_uint aux_len) {
  dx_uint* aux = (dx_uint*)(ctx->buf + aux_off);
  
  if(!*aux) {
    // We don't actually need this information.  We'll just mark that it was
    // present so that we regenerate it on write.
    metadata->has_class_lookup = 1;
    metadata->aux_format = AUX_FORMAT_OLD;
  } else while(*aux != AUX_END) {
    metadata->aux_format = AUX_FORMAT_NEW;
    dx_uint size = aux[1];
    dx_ubyte* data = (dx_ubyte*)(aux + 2);
    switch(*aux) {
      case AUX_CLASS_LOOKUP:
        // We don't actually need this information.  We'll just mark that it was
        // present so that we regenerate it on write.
        metadata->has_class_lookup = 1;
        break;
      case AUX_REGISTER_MAPS:
        // We don't actually need this information.  We'll just mark that it was
        // present so that we regenerate it on write.
        metadata->has_register_maps = 1;
        break;
/*
      case AUX_REDUCING_INDEX_MAP:
        if(ctx->has_reductions) {
          DXC_ERROR("only one index map can be present");
          return 0;
        }
        metadata->has_reducing_index_map = 1;
        if(!parse_index_map(ctx, data, 1)) {
          return 0;
        }
        break;
      case AUX_EXPANDING_INDEX_MAP:
        if(ctx->has_reductions) {
          DXC_ERROR("only one index map can be present");
          return 0;
        }
        metadata->has_expanding_index_map = 1;
        if(!parse_index_map(ctx, data, 1)) {
          return 0;
        }
        break;
*/
    }
    size = (size + 8 + 7) & ~7;
    aux += size / 4;
  }
  return 1;
}
