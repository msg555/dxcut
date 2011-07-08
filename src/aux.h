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
#ifndef DEX_AUX_H
#define DEX_AUX_H

#include <dxcut/file.h>

#include "common.h"

typedef enum {
  AUX_CLASS_LOOKUP = 0x434c4b50,   /* CLKP */
  AUX_REGISTER_MAPS = 0x524d4150,   /* RMAP */
  AUX_REDUCING_INDEX_MAP = 0x5249584d,   /* RIXM */
  AUX_EXPANDING_INDEX_MAP = 0x4549584d,   /* EIXM */
  AUX_END = 0x41454e44,   /* AEND */
} DexAuxCodes;

typedef enum {
  AUX_FORMAT_NEW = 0,
  AUX_FORMAT_OLD = 1,
} DexAuxFormat;

extern
int dxc_read_deps_table(read_context* ctx, OdexData* metadata,
                        dx_uint deps_off, dx_uint deps_len);

extern
int dxc_read_aux_table(read_context* ctx, OdexData* metadata,
                       dx_uint aux_off, dx_uint aux_len);
#endif // DEX_AUX_H
