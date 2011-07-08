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
/*! \file field.h
 */
#ifndef __DXCUT_FIELD_H
#define __DXCUT_FIELD_H
#include <dxcut/access_flags.h>
#include <dxcut/annotation.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// The access flags associated with this field.
  DexAccessFlags access_flags;
  
  /// The type of this field.
  ref_str* type;

  /// The name of this field.
  ref_str* name;
  
  /// A sentinel terminated list of Annotations that are applied directly to
  /// this field.
  DexAnnotation* annotations;
} DexField;

/** \fn void dxc_free_field(DexField* field)
 *  \brief Deletes all information for this field. Does not attempt to delete
 *  the passed pointer.
 */
extern
void dxc_free_field(DexField* field);

/** \fn int dxc_is_sentinel_field(DexField* field)
 *  \brief Returns true if this field marks the end of a field list.
 */
extern
int dxc_is_sentinel_field(DexField* field);

/** \fn void dxc_make_sentinel_field(DexField* field)
 *  \brief Marks this field the end of a list.
 */
extern
void dxc_make_sentinel_field(DexField* field);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_FIELD_H
