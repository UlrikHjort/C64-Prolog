/***************************************************************************
--                      Prolog/C64 - interp
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
#ifndef INTERP_H
#define INTERP_H

#include "memory.h"

/*
 * Solve a Prolog goal.  Returns 1 on success, 0 on failure.
 *
 * Backtracking is handled internally; the first solution found is
 * returned.  Variable bindings are left in the heap/trail on success
 * and must be cleaned up by the caller when no longer needed.
 */
uint8_t solve(Cell goal);

/*
 * Copy a term to the heap with fresh variables.
 * Used to instantiate a stored clause before trying it.
 * The copy is placed at the current hp position.
 */
Cell copy_term(Cell c);

#endif /* INTERP_H */
