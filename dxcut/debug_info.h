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
/*! \file debug_info.h
 */
#ifndef DXCUT_DEBUG_INFO_H
#define DXCUT_DEBUG_INFO_H
#include <dxcut/dex.h>
#ifdef __cplusplus
extern "C" {
#endif

/** \enum enum DexDebugOpCode
 * See the debug_info_item of
 * http://www.netmite.com/android/mydroid/dalvik/docs/dex-format.html for more
 * information. All debug instruction lists should end with the debug opcode
 * DBG_END_SEQUENCE.
 */
typedef enum DexDebugOpCode {
  DBG_END_SEQUENCE = 0x00,
  DBG_ADVANCE_PC = 0x01,
  DBG_ADVANCE_LINE = 0x02,
  DBG_START_LOCAL = 0x03,
  DBG_START_LOCAL_EXTENDED = 0x04,
  DBG_END_LOCAL = 0x05,
  DBG_RESTART_LOCAL = 0x06,
  DBG_SET_PROLOGUE_END = 0x07,
  DBG_SET_EPILOGUE_BEGIN = 0x08,
  DBG_SET_FILE = 0x09,
  DBG_FIRST_SPECIAL = 0x0a
} DexDebugOpCode;

typedef struct {
  /// The opcode for this debug instruction.
  dx_ubyte opcode;
  /// Opcode specific values for this debug instruction.
  union {
    dx_uint addr_diff; /* DBG_ADVANCE_PC */
    dx_uint line_diff; /* DBG_ADVANCE_LINE */
    struct {
      dx_uint register_num;
      ref_str* name; /* NULL if not present. */
      ref_str* type; /* NULL if not present. */
      ref_str* sig; /* NULL if not present. */
    }* start_local; /* DBG_START_LOCAL, DBG_START_LOCAL_EXTENDED */
    dx_uint register_num; /* DBG_END_LOCAL, DBG_RESTART_LOCAL */
    ref_str* name; /* DBG_SET_FILE. NULL if not present */
  } p;
} DexDebugInstruction;

typedef struct {
  /// The initial value of the state machine's line register.
  dx_uint line_start;

  /// NULL terminated list of strings giving the names of the parameters for
  /// this function.  If the ith parameter name is not available the string will
  /// be empty.
  ref_strstr* parameter_names;

  /// The actual byte code of the state machine.  This array is ended the
  /// instruction with the opcode DBG_END_SEQUENCE.
  DexDebugInstruction* insns;
} DexDebugInfo;

/** \fn void dxc_free_debug_info(DexDebugInfo* debug_info)
 *  \brief Free all data associated for this debug info structure.  Does not
 *  attempt to free the passed pointer.
 */
extern
void dxc_free_debug_info(DexDebugInfo* debug_info);

#ifdef __cplusplus
}
#endif
#endif // DXCUT_DEBUG_INFO
