/***************************************************************************
--                      Prolog/C64 - trail
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
#ifndef TRAIL_H
#define TRAIL_H

#include "memory.h"

/* Initialise trail state (tp = hb = 0). */
void trail_init(void);

/*
 * Bind heap[var_idx] to val.
 * Trails the binding if var_idx was allocated before the last choice point
 * (i.e. if var_idx < hb), so it can be undone on backtrack.
 */
void bind(uint16_t var_idx, Cell val);

/*
 * Undo all bindings trailed after 'mark' by resetting each trailed cell
 * to a self-referential REF (unbound variable).
 */
void untrail(uint16_t mark);

#endif /* TRAIL_H */
