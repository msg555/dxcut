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
#include <stdlib.h>

#include <dxcut/try_block.h>

void dxc_free_try_block(DexTryBlock* try_block) {
  if(!try_block) return;
  if(try_block->handlers) {
    DexHandler* ptr;
    for(ptr = try_block->handlers;
        !dxc_is_sentinel_handler(ptr); ptr++) dxc_free_handler(ptr);
  }
  if(try_block->catch_all_handler) {
    dxc_free_handler(try_block->catch_all_handler);
  }
  free(try_block->handlers);
  free(try_block->catch_all_handler);
}

int dxc_is_sentinel_try_block(const DexTryBlock* try_block) {
  return try_block->handlers == NULL;
}

void dxc_make_sentinel_try_block(DexTryBlock* try_block) {
  try_block->handlers = NULL;
}
