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
#ifndef DEX_MUTF8
#define DEX_MUTF8

#include <dxcut/dex.h>

dx_uint mutf8_code_points(char* s);

int mutf8_compare(char* a, char* b);

int mutf8_ref_compare(ref_str* a, ref_str* b);

#endif // DEX_MUTF8
