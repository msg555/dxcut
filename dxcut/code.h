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
/*! \file code.h
 */
#ifndef __DXCUT_CODE_H
#define __DXCUT_CODE_H
#include <dxcut/dalvik.h>
#include <dxcut/dex.h>
#include <dxcut/debug_info.h>
#include <dxcut/try_block.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// The number of registers used by this code.
  dx_ushort registers_size;
  
  /// The number of words of incoming arguments to this method.
  dx_ushort ins_size;
  
  /// The number of words of outgoing argument space for this method.
  dx_ushort outs_size;
  
  /// Pointer to the debug information for this method or NULL if there is no
  /// information available.
  DexDebugInfo* debug_information;
  
  /// A sentinel terminated list of try bytecode ranges and the catch handlers
  /// that contain them.
  DexTryBlock* tries;
  
  /// The number of instructions for this code piece.
  dx_uint insns_count;
  
  /// The instructions for this code piece.
  DexInstruction* insns;
} DexCode;

/** \fn void dxc_free_code(DexCode* code)
 * \brief Frees all data allocated for this code structure. Does not attempt to
 * free the passed pointer.
 */
extern
void dxc_free_code(DexCode* code);

#ifdef __cplusplus
}
#endif
#endif
