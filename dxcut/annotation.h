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
/*! \file annotation.h
 *  \brief Contains java annotation information
 *
 * Contains the classes for storing annotation information and an enumeration
 * with the possible annotation visibilities.  Annotations that are stored
 * in a DexValue do not have associated visibilities and should have their
 * corresponding visibility field set to VISIBILITY_NONE.
 */
#ifndef __DXCUT_ANNOTATION_H
#define __DXCUT_ANNOTATION_H
#include <dxcut/dex.h>
#include <dxcut/value.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum DexAnnotationVisibility {
  /// Annotation is only meant to be visible at compile time.
  VISIBILITY_BUILD = 0x00,
  /// Annotation is visible at runtime.
  VISIBILITY_RUNTIME = 0x01,
  /// Annotation is visible at runtime but only to the underlying system.
  VISIBILITY_SYSTEM = 0x02,
  /// If this annotation came from a DexValue it won't have a visibility
  /// associated with it.
   VISIBILITY_NONE = 0xFF
} DexAnnotationVisibility;


typedef struct {
  /// The name of the parameter. For annotations that contain one parameter this
  /// is "value".
  ref_str* name;
  /// The value associated with this parameter.
  DexValue value;
} DexNameValuePair;

struct dex_annotation_t {
  /// Indicates the intended visibility of this annotation.
  DexAnnotationVisibility visibility;
  
  /// Indicates the type of the element.
  ref_str* type;

  /// A sentinel-terminated list of name value pair parameters for this
  /// annotation.
  DexNameValuePair* parameters;
};

typedef struct dex_annotation_t DexAnnotation;

/** \fn void dxc_free_annotation(DexAnnotation* annotation)
 *  \brief Frees all data allocated for this annotation.  Does not attempt to
 *         free the passed pointer.
 */
extern
void dxc_free_annotation(DexAnnotation* annotation);

/** \fn int dxc_is_sentinel_annotation(DexAnnotation* annotation)
 *  \brief Returns true if this annotation marks the end of an annotation list.
 */
extern
int dxc_is_sentinel_annotation(DexAnnotation* annotation);

/** \fn void dxc_make_sentinel_annotation(DexAnnotation* annotation)
 *  \brief Marks the passed annotation as the end of a list.
 */
extern
void dxc_make_sentinel_annotation(DexAnnotation* annotation);

/** \fn void dxc_free_parameter(DexNameValuePair* parameter)
 *  \brief Frees all data allocated for this name value pair.
 *         Does not attempt to free the passed pointer.
 */
extern
void dxc_free_parameter(DexNameValuePair* parameter);

/** \fn int dxc_is_sentinel_parameter(DexNameValuePair* parameter)
 *  \brief Returns true if this name value pair marks the end of a
 *         name value pair list.
 */
extern
int dxc_is_sentinel_parameter(DexNameValuePair* parameter);

/** \fn void dxc_make_sentinel_parameter(DexNameValuePair* parameter)
 *  \brief Marks the passed named value pair as the end of a list.
 */
extern
void dxc_make_sentinel_parameter(DexNameValuePair* parameter);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_ANNOTATION_H
