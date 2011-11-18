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
/*! \file handler.h
 */
#ifndef __DXCUT_HANDLER_H
#define __DXCUT_HANDLER_H
#include <dxcut/dex.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// The name of the type of exception being caught.  For catch-all handlers
  /// this field will be NULL.
  ref_str* type;
  
  /// The address into the bytecode where this handler begins.
  dx_uint addr;
} DexHandler;

/** \fn void dxc_free_handler(DexHandler* handler);
 *  \brief Frees all data associated with this handler.  Does not attempt to
 *  free the passed pointer.
 */
extern
void dxc_free_handler(DexHandler* handler);

/** \fn int dxc_is_sentinel_handler(DexHandler* handler)
 *  \brief Returns true if this handler marks the end of a handler list.
 */
extern
int dxc_is_sentinel_handler(const DexHandler* handler);

/** \fn void dxc_make_sentinel_handler(DexHandler* handler)
 *  \brief Marks the passed handler as the end of a list.
 */
extern
void dxc_make_sentinel_handler(DexHandler* handler);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_HANDLER_H
