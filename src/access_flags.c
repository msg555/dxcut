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
#include <stdlib.h>
#include <stdio.h>

#include <dxcut/access_flags.h>

static
DexAccessFlags all_flags[] = {
  ACC_PUBLIC,
  ACC_PRIVATE,
  ACC_PROTECTED,
  ACC_STATIC,
  ACC_FINAL,
  ACC_SYNCHRONIZED,
  ACC_VOLATILE,
  //ACC_BRIDGE,
  ACC_TRANSIENT,
  //ACC_VARARGS,
  ACC_NATIVE,
  ACC_INTERFACE,
  ACC_ABSTRACT,
  ACC_STRICT,
  //ACC_SYNTHETIC,
  //ACC_ANNOTATION,
  //ACC_ENUM,
  //ACC_CONSTRUCTOR,
  ACC_DECLARED_SYNCHRONIZED,
};

static
const char* all_flags_str[] = {
  "public",
  "private",
  "protected",
  "static",
  "final",
  "synchronized",
  "volatile",
  //"bridge",
  "transient",
  //"varargs",
  "native",
  "interface",
  "abstract",
  "strict",
  //"synthetic",
  // "annotation",
  //"enum",
  //"constructor",
  "synchronized"
};

char buf[256];

char* dxc_access_flags_nice(DexAccessFlags flags) {
  int pos = 0;
  unsigned i;
  for(i = 0; i < sizeof(all_flags) / sizeof(DexAccessFlags); i++) {
    if(flags & all_flags[i]) {
      if(pos) buf[pos++] = ' ';
      if(all_flags[i] == ACC_INTERFACE && (flags & ACC_ANNOTATION)) {
        buf[pos++] = '@';
      }
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", all_flags_str[i]);
    }
  }
  buf[pos] = 0;
  return buf;
}
