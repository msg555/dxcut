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
/*! \file class.h
 */
#ifndef __DXCUT_CLASS_H
#define __DXCUT_CLASS_H
#include <dxcut/access_flags.h>
#include <dxcut/annotation.h>
#include <dxcut/field.h>
#include <dxcut/method.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// The name of the class.
  ref_str* name;
  
  /// Whatever access flags are set for this class.  Has information on scope as
  /// well as other flags.  See access_flags.h for more information.
  DexAccessFlags access_flags;
  
  /// The class name that this class inherits from or NULL if it is a root of an
  /// inheritance tree (like Object).
  ref_str* super_class;
  
  /// A NULL terminated list of interfaces that this class inherits from.
  ref_strstr* interfaces;
  
  /// The name of the source file that this class came from originally or NULL
  /// if this information is not present.
  ref_str* source_file;
  
  /// A sentinel terminated list of Annotations that are applied directly to
  /// this class.
  DexAnnotation* annotations;
  
  /// A sentinel terminated list of values to initialized static fields declared
  /// in this class.  The elements should be in the same order as the field
  /// list.
  DexValue* static_values;
  
  /// A sentinel terminated list of static fields associated with this class.
  /// See static_values for their initializer list.
  DexField* static_fields;
  
  /// A sentinel terminated list of non-static fields associated with this
  /// class.
  DexField* instance_fields;
  
  /// A sentinel terminated list of non-virtual methods (any of static, private,
  /// or constructor).
  DexMethod* direct_methods;
  
  /// A sentinel terminated list of virtual methods.
  DexMethod* virtual_methods;
} DexClass;

/** \fn const char* dxc_type_nice(const char* type)
 * Convert type names from the format they are in the dex file to a
 * more readable fully qualified name.  This returns a static buffer.
 */
extern
const char* dxc_type_nice(const char* type);

/** \fn const char* dxc_type_name(const char* type)
 * Converts fully qualified class names (and primitive types as well) to the
 * form that they representing in dex files.  This returns a static buffer.
 */
extern
const char* dxc_type_name(const char* nice_type);

/** \fn void dxc_free_class(DexClass* cl)
 *  \brief Frees all data allocated for this class. Does not attempt to free the
 *  passed pointer.
 */
extern
void dxc_free_class(DexClass* cl);

/** \fn int dxc_is_sentinel_class(DexClass* cl)
 *  \brief Returns true if this class marks the end of a class list.
 */
extern
int dxc_is_sentinel_class(DexClass* cl);

/** \fn void dxc_make_sentinel_class(DexClass* cl)
 *  \brief Marks the passed class as the end of a list.
 */
extern
void dxc_make_sentinel_class(DexClass* cl);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_CLASS_H
