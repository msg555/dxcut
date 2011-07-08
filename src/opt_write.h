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

TODO: This implementation should be moved into opt_write.c and write.h should be
created exposing anything this needs.  Currently this is just being included
right into write.c.
*/

#include "aux.h"

static
dx_uint class_name_hash(const char* s) {
  dx_uint ret = 1;
  while(*s != '\x0') {
    ret = ret * 31 + *s++;
  }
  return ret;
}  

//select count(distinct app_power.timestamp) as cnt, apps.name from apps,
//app_power where apps.id=app_power.app_id group by apps.name order by cnt desc
data_item write_deps(write_context* ctx, DexFile* dex) {
  dx_uint sz = 0;
  ref_str** dep;
  for(dep = dex->metadata->deps->s; *dep; dep++) sz++;

  data_item ret = init_data_item(0);
  write_uint(&ret, dex->metadata->dex_mod_time);
  write_uint(&ret, dex->metadata->dex_crc);
  write_uint(&ret, dex->metadata->vm_version);
  write_uint(&ret, sz);

  dx_uint i;
  dx_ubyte** dep_sha;
  for(dep = dex->metadata->deps->s, dep_sha = dex->metadata->dep_shas;
      *dep; dep++, dep_sha++) {
    dx_uint ln = strlen((*dep)->s);
    write_uint(&ret, ln + 1);
    for(i = 0; i <= ln; i++) write_ubyte(&ret, (*dep)->s[i]);
    for(i = 0; i < 20; i++) write_ubyte(&ret, (*dep_sha)[i]);
  }

  return ret;
}

static
dx_uint read_uint_from_data(data_item* data, dx_uint off) {
  dx_uint ret;
  memcpy(&ret, data->data + off, 4);
  return ret;
}

data_item write_aux(write_context* ctx, DexFile* dex, data_item* file,
                    dx_uint class_sz, dx_uint class_off,
                    dx_uint type_off, dx_uint str_off) {
  data_item ret = init_data_item(0);
  if(dex->metadata->aux_format == AUX_FORMAT_OLD) {
    write_uint(&ret, 0);
  }
  if(dex->metadata->has_class_lookup) {
    dx_uint entries = 1;
    while(2 * class_sz > entries) entries = entries << 1;
    if(dex->metadata->aux_format == AUX_FORMAT_NEW) {
      write_uint(&ret, AUX_CLASS_LOOKUP);
      write_uint(&ret, 8 + entries * 12);
    }
    write_uint(&ret, 8 + entries * 12);
    write_uint(&ret, entries);

    dx_uint* tab = (dx_uint*)calloc(entries * 3, sizeof(dx_uint));

    dx_uint i;
    for(i = 0; i < class_sz; i++) {
      dx_uint def_off = class_off + 32 * i;
      dx_uint desc_off = read_uint_from_data(file, -0x70 + // str to str data
          str_off + 4 * read_uint_from_data(file, -0x70 + // type to str
          type_off + 4 * read_uint_from_data(file, -0x70 + def_off)));
      while(file->data[desc_off - 0x70] & 0x80) desc_off++;
      desc_off++;
      char* desc = file->data + desc_off - 0x70;

      dx_uint hsh = class_name_hash(desc);
      int pos;
      for(pos = hsh & (entries - 1); ; pos = (pos + 1) & (entries - 1)) {
        if(!tab[3 * pos + 2]) {
          tab[3 * pos] = hsh;
          tab[3 * pos + 1] = desc_off;
          tab[3 * pos + 2] = def_off;
          break;
        }
      }
    }

    for(i = 0; i < 3 * entries; i++) {
      write_uint(&ret, tab[i]);
    }
    free(tab);

    if(dex->metadata->aux_format == AUX_FORMAT_NEW) {
      while(ret.data_sz & 7) write_ubyte(&ret, 0); // Align to 8 bytes.
    }
  }
  if(dex->metadata->has_register_maps) {
    if(dex->metadata->aux_format == AUX_FORMAT_OLD) {
      DXC_ERROR("aux format doesn't support register maps");
    } else {
      DXC_ERROR("register maps not implemented yet");
    }
  }
  if(dex->metadata->has_reducing_index_map) {
    if(dex->metadata->aux_format == AUX_FORMAT_OLD) {
      DXC_ERROR("aux format doesn't support index maps");
    } else {
      DXC_ERROR("reducing index map not implemented yet");
    }
  }
  if(dex->metadata->has_expanding_index_map) {
    if(dex->metadata->aux_format == AUX_FORMAT_OLD) {
      DXC_ERROR("aux format doesn't support index maps");
    } else {
      DXC_ERROR("expanding index map not implemented yet");
    }
  }
  if(dex->metadata->aux_format == AUX_FORMAT_NEW) {
    write_uint(&ret, AUX_END);
  }
  return ret;
}

