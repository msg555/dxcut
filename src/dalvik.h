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
#ifndef DEX_DALVIK_H
#define DEX_DALVIK_H

#include <dxcut/dalvik.h>
#include <dxcut/dex.h>

#include "common.h"

extern
int dxc_read_dalvik(read_context* ctx, dx_uint size, dx_uint off,
                    DexInstruction** insns, dx_uint* count);

#endif // DEX_DALVIK_H
