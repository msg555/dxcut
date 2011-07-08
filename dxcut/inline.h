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
/*! \file inline.h
 */
#ifndef __DXCUT_INLINE_H
#define __DXCUT_INLINE_H
#include <dxcut/dalvik.h>
#ifdef __cplusplus
extern "C" {
#endif

/*! \fn int dxc_perform_inline(DexInstruction* insn)
 *  \brief convert the invoke instruction to an inline instruction if possible.
 *  Returns true if the instruction was inlined.
 */
extern
int dxc_perform_inline(DexInstruction* insn);

/*! \fn void dxc_remove_inline(DexInstruction* insn)
 *  \brief Convert an inlined function call to a normal invoke function call.
 */
extern
void dxc_remove_inline(DexInstruction* insn);

#ifdef __cplusplus
}
#endif
#endif // __DXCUT_INLINE_H
