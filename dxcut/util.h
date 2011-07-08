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
/*! \file util.h
 *  \brief Miscelaneous utility functions
 */
#ifndef __DXCUT_UTIL_H
#define __DXCUT_UTIL_H
#include <dxcut/file.h>
#ifdef __cplusplus
extern "C" {
#endif

/** \fn void dxc_rename_identifiers(DexFile* file,
          int num_fields, ref_field* source_fields, ref_field* dest_fields,
          int num_methods, ref_method* source_methods, ref_method* dest_methods,
          int num_classes, ref_str** source_classes, ref_str** dest_classes,
 *  Walks through the entire dex file changing the name of the identifiers
 *  passed.  Effectively first field renames will be done, then method renames,
 *  then class renames.
 *
 *  Field references to the field source_fields[i] will be replaced with
 *  references to the field dest_fields[i].  If source_fields[i] is a field
 *  defined in the dex file then source_fields[i] and dest_fields[i] should have
 *  the same defining class.
 *
 *  Methods are renamed in the same manner as fields.
 *
 *  Class references to the type source_classes[i] will be replaced with
 *  references to the type dest_classes[i].  This happens after the above
 *  replacements.
 *
 *  If num_classes, num_fields, num_methods is 0 then the corresponding
 *  source/dest arguments may be NULL.
 */
extern
void dxc_rename_identifiers(DexFile* file,
      int num_fields, ref_field* source_fields, ref_field* dest_fields,
      int num_methods, ref_method* source_methods, ref_method* dest_methods,
      int num_classes, ref_str** source_classes, ref_str** dest_classes);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_UTIL_H
