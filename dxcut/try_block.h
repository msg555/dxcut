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
/** \file try_block.h
 */
#ifndef __DXCUT_TRY_BLOCK
#define __DXCUT_TRY_BLOCK
#include <dxcut/dex.h>
#include <dxcut/handler.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// The start address in the method code block that this try block covers.
  dx_uint start_addr;
  
  /// The last code unit covered (inclusive) is start_addr + insn_count - 1.
  dx_ushort insn_count;
  
  /// A sentinel terminated list of handlers with specified catch types.  The
  /// 'type' field should not be NULL for each of these entries.  These should
  /// be listed in the order the vm should test in.
  DexHandler* handlers;
  
  /// The handler to be used when no other handlers are appropriate or NULL if
  /// the handler does not exist.  The 'type' field for this handler should be
  /// NULL if present.
  DexHandler* catch_all_handler;
} DexTryBlock;

/** \fn void dxc_free_try_block(DexTryBlock* try_block)
 *  \brief Frees all data ssociated with this try block. Does not attempt to
 *  free the passed pointer.
 */
extern
void dxc_free_try_block(DexTryBlock* try_block);

/** \fn int dxc_is_sentinel_try_block(DexTryBlock* try_block)
 *  \brief Returns true if this try block marks the end of a try block list.
 */
extern
int dxc_is_sentinel_try_block(DexTryBlock* try_block);

/** \fn void dxc_make_sentinel_try_block(DexTryBlock* try_block)
 *  \brief Marks the passed try block as the end of a list.
 */
extern
void dxc_make_sentinel_try_block(DexTryBlock* try_block);

#ifdef __cplusplus
}
#endif
#endif
