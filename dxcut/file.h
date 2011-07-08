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
/*! \file file.h
 */
#ifndef __DXCUT_FILE_H
#define __DXCUT_FILE_H
#include <stdio.h>
#include <dxcut/class.h>
#ifdef __cplusplus
extern "C" {
#endif

/// \enum OdexFlags
/// \brief Flags for odex files
typedef enum {
  /// Indicates that we tried to verify all classes.
  DEX_FLAG_VERIFIED = 1,
  /// Indicates that the file is byte swapped to big endian.
  DEX_FLAG_BIG = 2,
  /// Indicates that the field accesses have been optimized.
  DEX_FLAG_FIELDS = 4,
  /// Indicates that the method invocations have been optimized.
  DEX_FLAG_INVOCATIONS = 8,
} OdexFlags;

typedef struct {
  /// Dex identifier used for identification perposes.  This is a sha1 hash of
  /// the original dex file prior to optimizations.
  dx_ubyte* id;

  /// Optimized dex flags, used only for odex files.  The sum of some of the
  /// OdexFlags.
  dx_uint flags;

  /// The version of the odex file.
  dx_uint odex_version;

  // These fields come from the deps section.
  
  /// The time that the original classes.dex was last modified.
  dx_uint dex_mod_time;
  /// The checksum of the original classes.dex file.
  dx_uint dex_crc;
  /// The version of the vm that optimized this dex file.
  dx_uint vm_version;
  /// A NULL terminated list of dependencies for the odex file.
  ref_strstr* deps;
  /// A NULL terminated list of sha1 hashes of the dependencies.  Should have
  /// the same length as deps.
  dx_ubyte** dep_shas;

  // These fields come from the aux section.
  
  /// Indicates what format the aux table was stored in.  See src/aux.c for more
  /// details.
  dx_uint aux_format;
  /// Indicates whether the file had a class lookup table / whether a class
  /// lookup table should be written.
  dx_uint has_class_lookup;
  /// Indicates whether the file had register maps / whether register maps
  /// should be written.
  dx_uint has_register_maps;
  /// Indicates whether the file had a reducing index map / whether a reducing
  /// index map should be written.
  dx_uint has_reducing_index_map;
  /// Indicates whether the file had an expanding index map / whether an
  /// expanding index map should be written.  Note that you cannot have both a
  /// reducing and expanding index map.
  dx_uint has_expanding_index_map;
} OdexData;

typedef struct {
  /// Sentinel terminated list of classes in this file.
  DexClass* classes;

  /// Metadata on the file present only for odex files.  Should be NULL if the
  /// file was a normal dex file.
  OdexData* metadata;

  /// Gives the original method table for this dex file.  This will be ignored
  /// by dxc_file_write().
  dx_uint method_table_size;
  ref_method* method_table;

  /// Gives the original field table for this dex file.  This will be ignored
  /// by dxc_file_write().
  dx_uint field_table_size;
  ref_field* field_table;

  /// Gives the original type table for this dex file.  This will be ignored
  /// by dxc_file_write().
  dx_uint type_table_size;
  ref_str** type_table;
} DexFile;

/** \fn DexFile* dxc_read_file(FILE* fin)
 *  \brief Read in the dex file given by the input stream.
 */
extern
DexFile* dxc_read_file(FILE* fin);

/** \fn DexFile* dxc_read_buffer(void* buf, dx_uint size)
 *  \brief Read in the dex file residing in memory at the address given by buf
 *  with length size.
 */
extern
DexFile* dxc_read_buffer(void* buf, dx_uint size);

/** \fn void dxc_write_file(DexFile* dex, FILE* fout)
 *  \brief Write out the DexFile structure to a file.
 */
extern
void dxc_write_file(DexFile* dex, FILE* fout);

/** \fn void dxc_free_file(DexFile* dex)
 *  \brief Free the entire DexFile structure including the given pointer itself.
 */
extern
void dxc_free_file(DexFile* dex);

/** \fn void dxc_free_odex_data(OdexData* data);
 *  \brief Free the OdexData structure including the given pointer itself.
 */
extern
void dxc_free_odex_data(OdexData* data);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_FILE_H
