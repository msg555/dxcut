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
#include "mutf8.h"

dx_uint mutf8_code_points(char* s) {
  int points = 0;
  while(*s) {
    int sz;
    dx_ubyte head = *s++;
    if(head >> 7 == 0x00) {
      sz = 1;
    } else if(head >> 5 == 0x06) {
      sz = 2;
    } else if(head >> 4 == 0x0E) {
      sz = 3;
    } else {
      return -1;
    }
    while(--sz) {
      dx_ubyte v = *s++;
      if(v >> 6 !=  0x02) {
        return -1;
      }
    }
    points++;
  }
  return points;
}

// Basically the same as a byte-by-byte comparison except that we have to look
// out for the special null two-byte encoding.
int mutf8_compare(char* aa, char* bb) {
  dx_ubyte* a = (dx_ubyte*)aa;
  dx_ubyte* b = (dx_ubyte*)bb;
  for( ; *a && *a == *b; a++, b++);
  if(*a == *b) return 0;
  if(!*a) return -1;
  if(!*b) return 1;
  if(*a < *b) {
    return *b == 0xC0 && *(b + 1) == 0x80 ? 1 : -1;
  } else {
    return *a == 0xC0 && *(a + 1) == 0x80 ? -1 : 1;
  }
}

int mutf8_ref_compare(ref_str* a, ref_str* b) {
  return mutf8_compare(a->s, b->s);
}
