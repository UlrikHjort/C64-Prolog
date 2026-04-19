/***************************************************************************
--                      Prolog/C64 - heap
--
--           Copyright (C) 2026 By Ulrik Hørlyk Hjort
--
-- Permission is hereby granted, free of charge, to any person obtaining
-- a copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to
-- the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
-- LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
-- WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-- ***************************************************************************/
#ifndef HEAP_H
#define HEAP_H

#include "memory.h"

/*
 * Allocate n consecutive cells on the heap.
 * Returns the word-index of the first allocated cell.
 * Halts with an error message on overflow.
 */
uint16_t heap_alloc(uint8_t n);

/* Read / write a single heap cell by word-index. */
#define heap_get(i)      (HEAP_BASE[(i)])
#define heap_set(i, v)   (HEAP_BASE[(i)] = (Cell)(v))

/* Initialise heap state (hp = hb = 0). */
void heap_init(void);

#endif /* HEAP_H */
