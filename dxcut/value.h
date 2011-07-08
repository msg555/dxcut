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
/** \file value.h
 */
#ifndef __DXCUT_VALUE_H
#define __DXCUT_VALUE_H
#include <dxcut/dex.h>
#ifdef __cplusplus
extern "C" {
#endif

/** \enum DexValueType
 *  Gives the type of value encoded in a ::DexValue structure.
 */
typedef enum DexValueType {
  /// A byte encoded in ::DexValue .value.val_byte.
  VALUE_BYTE = 0x00,
  /// A short encoded in ::DexValue .value.val_short.
  VALUE_SHORT = 0x02,
  /// A char encoded in ::DexValue .value.val_char.
  VALUE_CHAR = 0x03,
  /// An int encoded in ::DexValue .value.val_int.
  VALUE_INT = 0x04,
  /// A long encoded in ::DexValue .value.val_long.
  VALUE_LONG = 0x06,
  /// A float encoded in ::DexValue .value.val_float.
  VALUE_FLOAT = 0x10,
  /// A double encoded in ::DexValue .value.val_double.
  VALUE_DOUBLE = 0x11,
  /// A string encoded in ::DexValue .value.val_string.
  VALUE_STRING = 0x17,
  /// A type encoded in ::DexValue .value.val_type.
  VALUE_TYPE = 0x18,
  /// A field encoded in ::DexValue .value.val_field.
  VALUE_FIELD = 0x19,
  /// A method encoded in ::DexValue .value.val_method.
  VALUE_METHOD = 0x1a,
  /// A enum encoded in ::DexValue .value.val_enum.
  VALUE_ENUM = 0x1b,
  /// A sentinel terminated array encoded in ::DexValue .value.val_array.
  VALUE_ARRAY = 0x1c,
  /// An annotation encoded in ::DexValue .value.val_annotation.  This field
  /// should never be NULL if ::DexValue .type is VALUE_ANNOTATION.
  VALUE_ANNOTATION = 0x1d,
  /// The NULL Object reference.
  VALUE_NULL = 0x1e,
  /// A boolean encoded in ::DexValue .value.val_boolean.
  VALUE_BOOLEAN = 0x1f,

  VALUE_SENTINEL = 0xff
} DexValueType;

struct dex_annotation_t;

struct dex_value_t {
  DexValueType type;
  union {
    dx_byte val_byte;
    dx_short val_short;
    dx_ushort val_char;
    dx_int val_int;
    dx_long val_long;
    float val_float;
    double val_double;
    ref_str* val_str;
    ref_str* val_type;
    ref_field val_field;
    ref_method val_method;
    ref_field val_enum;
    /* A sentinel terminated array of DexValues. */
    struct dex_value_t* val_array; 
    struct dex_annotation_t* val_annotation;
    int val_boolean;
  } value;
};

typedef struct dex_value_t DexValue;

/** \fn const char* dxc_value_nice(DexValue* value)
 *  \brief  This returns a nicely formated version of the DexValue's contents in
 *  the form of a static buffer.
 */
extern
const char* dxc_value_nice(DexValue* value);

/** \fn void dxc_free_value(DexValue* value)
 *  \brief Frees all data associated with this value. Does not attempt to free
 *  the passed pointer.
 */
extern
void dxc_free_value(DexValue* value);

/** \fn int dxc_is_sentinel_value(DexValue* value)
 *  \brief Returns true if this value marks the end of a value list.
 */
extern
int dxc_is_sentinel_value(DexValue* value);

/** \fn void dxc_make_sentinel_value(DexValue* value)
 *  \brief Makes the passed value the end of a value list.
 */
extern
void dxc_make_sentinel_value(DexValue* value);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_VALUE_H
