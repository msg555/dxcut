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
/*! \file access_flags.h
 */
#ifndef __DXCUT_ACCESS_FLAGS_H
#define __DXCUT_ACCESS_FLAGS_H
#ifdef __cplusplus
extern "C" {
#endif

/** \enum DexAccessFlags
 *  \brief See the access_flags section of
 *  http://www.netmite.com/android/mydroid/dalvik/docs/dex-format.html for flag
 *  definitions.
 */
typedef enum DexAccessFlags {
  ACC_PUBLIC = 0x1,
  ACC_PRIVATE = 0x2,
  ACC_PROTECTED = 0x4,
  ACC_STATIC = 0x8,
  ACC_FINAL = 0x10,
  ACC_SYNCHRONIZED = 0x20,
  ACC_VOLATILE = 0x40,
  ACC_BRIDGE = 0x40,
  ACC_TRANSIENT = 0x80,
  ACC_VARARGS = 0x80,
  ACC_NATIVE = 0x100,
  ACC_INTERFACE = 0x200,
  ACC_ABSTRACT = 0x400,
  ACC_STRICT = 0x800,
  ACC_SYNTHETIC = 0x1000,
  ACC_ANNOTATION = 0x2000,
  ACC_ENUM = 0x4000,
  ACC_UNUSED = 0x8000,
  ACC_CONSTRUCTOR = 0x10000,
  ACC_DECLARED_SYNCHRONIZED = 0x20000
} DexAccessFlags;

/** \fn char* dxc_access_flags_nice(DexAccessFlags flags)
 *  \brief Returns a space separated test version of the access flags in the
 *         form of a static buffer.
 *
 * TODO: can we make this thread safe?
 */
extern
char* dxc_access_flags_nice(DexAccessFlags flags);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_ACCESS_FLAGS_H
