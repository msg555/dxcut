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
/*! \file method.h
 */
#ifndef __DXCUT_METHOD_H
#define __DXCUT_METHOD_H
#include <dxcut/access_flags.h>
#include <dxcut/code.h>
#include <dxcut/annotation.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// Indicates the access flags for this method.
  DexAccessFlags access_flags;

  /// The name of this method.
  ref_str* name;

  /// A NULL terminated list of the object names for this method.  The first
  /// element should always exist and represents the return type of this method.
  ref_strstr* prototype;
  
  /// A pointer to the code body for this method or NULL if this method is
  /// either abstract or native.
  DexCode* code_body;
  
  /// A sentinel terminated list of Annotations that are applied directly to
  /// this method.
  DexAnnotation* annotations;

  /// A NULL terminated list of sentinel terminated annotations that are applied
  /// to each of the parameters of this method.  The length of this list
  /// should be the same as the number of parameters in the prototype.
  DexAnnotation** parameter_annotations;
} DexMethod;

/** \fn void dxc_free_method(DexMethod* method)
 *  \brief Frees all data associated with this method. Does not attempt to free
 *  the passed pointer.
 */
extern
void dxc_free_method(DexMethod* method);

/** \fn int dxc_is_sentinel_method(DexMethod* method)
 *  \brief Returns true if this method marks the end of a method list.
 */
extern
int dxc_is_sentinel_method(const DexMethod* method);

/** \fn void dxc_make_sentinel_method(DexMethod* method)
 *  \brief Marks this method as the end of a list.
 */
extern
void dxc_make_sentinel_method(DexMethod* method);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_METHOD_H
