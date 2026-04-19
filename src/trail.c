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
#include "trail.h"
#include "heap.h"
#include <conio.h>

uint16_t tp = 0;   /* trail pointer: next free trail entry index */
uint16_t hb = 0;   /* heap backtrack mark: hp at last choice point */

void trail_init(void) {
    tp = 0;
    hb = 0;
}

void bind(uint16_t var_idx, Cell val) {
    /*
     * Conditional trail: only record bindings for variables that were
     * allocated BEFORE the last choice point (var_idx < hb).  Variables
     * allocated after the last choice point live above hb and will be
     * discarded by heap trimming on backtrack -- no trail entry needed.
     *
     * During Phase 1 (no choice points) hb == 0, so var_idx < 0 is
     * always false and nothing is trailed.  That is correct: there is
     * nothing to undo when there is no backtrack point.
     */
    if (var_idx < hb) {
        if (tp >= TRAIL_WORDS) {
            cputs("TRAIL OVERFLOW\r");
            for (;;) {}
        }
        TRAIL_BASE[tp++] = var_idx;
    }
    heap_set(var_idx, val);
}

void untrail(uint16_t mark) {
    while (tp > mark) {
        uint16_t idx = TRAIL_BASE[--tp];
        heap_set(idx, MKREF(idx));   /* restore to unbound self-reference */
    }
}
