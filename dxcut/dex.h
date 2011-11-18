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
/*! \file dex.h
 *  \brief Includes type information for the dxcut library.
 */
#ifndef __DXCUT_DEX_H
#define __DXCUT_DEX_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
typedef long long int          dx_long;
typedef unsigned long long int dx_ulong;
typedef int                    dx_int;
typedef unsigned int           dx_uint;
typedef short                  dx_short;
typedef unsigned short         dx_ushort;
typedef signed char            dx_byte;
typedef unsigned char          dx_ubyte;
#else
typedef __int64_t     dx_long;
typedef __uint64_t    dx_ulong;
typedef __int32_t     dx_int;
typedef __uint32_t    dx_uint;
typedef __int16_t     dx_short;
typedef __uint16_t    dx_ushort;
typedef signed char   dx_byte;
typedef unsigned char dx_ubyte;
#endif /* WIN32 */

/// Represents a reference counted string used throughout the api.  Do not
/// modify these structures directly.  Use dxc_induct_str to make a ref_str out
/// of an existing char* or use dxc_copy_str to increment the refrence count and
/// use dxc_free_str to decrement the reference count and possibly free the
/// pointer.
typedef struct {
  dx_uint cnt;
  char s[1];
} ref_str;

/*! \fn ref_str* dxc_induct_str(const char* s)
 *  \brief Copies the string pointed to s and creates a ref_str with the
 *  reference count set to 1.
 */
extern
ref_str* dxc_induct_str(const char* s);

/*! \fn ref_str* dxc_copy_str(ref_str* s)
 *  \brief Returns a copy of the ref_str s.
 *
 *  What this really does is simply increment the reference count on s and
 *  returns s.
 */
extern
ref_str* dxc_copy_str(ref_str* s);

/*! \fn void dxc_free_str(ref_str* s)
 *  \brief Decrements the reference count on s and deletes s if the reference
 *  count has gone to 0.
 */
extern
void dxc_free_str(ref_str* s);

/// Represents a reference counted list of strings used throughout the api.
/// This list is NULL terminated.  Use dxc_create_strstr and dxc_copy_strstr as
/// you would with ref_str.
typedef struct {
  dx_uint cnt;
  ref_str* s[1];
} ref_strstr;

/*! \fn ref_strstr* dxc_create_strstr(dx_uint sz)
 *  \brief Creates a list of sz ref_str* each initialized to NULL.
 *  Initializes the reference count to 1.
 */
extern
ref_strstr* dxc_create_strstr(dx_uint sz);

/*! \fn ref_strstr* dxc_copy_strstr(ref_strstr* s)
 *  \brief Returns a copy of the ref_strstr s
 *
 *  What this really does is simply incrementthe reference count on s and
 *  returns s.
 */
extern
ref_strstr* dxc_copy_strstr(ref_strstr* s);

/*! \fn void dxc_free_strstr(ref_strstr* s)
 *  \brief Decrements the reference coutn on s and deletes s if the reference
 *  count has gone to 0.
 */
extern
void dxc_free_strstr(ref_strstr* s);

typedef struct ref_field {
  ref_str* defining_class;
  ref_str* name;
  ref_str* type;
} ref_field;

typedef struct ref_method {
  ref_str* defining_class;
  ref_str* name;
  ref_strstr* prototype;
} ref_method;

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_DEX_H
