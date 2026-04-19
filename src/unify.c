/***************************************************************************
--                      Prolog/C64 - unify
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
#include "unify.h"
#include "trail.h"
#include "heap.h"

uint8_t unify(Cell a, Cell b) {
    uint8_t ta, tb;
    uint8_t i, ar;
    uint16_t ai, bi;
    Cell fa, fb;

    a = deref(a);
    b = deref(b);

    if (a == b) return 1;   /* identical cells: same bound var, atom, or int */

    ta = TAG(a);
    tb = TAG(b);

    if (ta == TAG_REF) {
        bind(VAL(a), b);
        return 1;
    }
    if (tb == TAG_REF) {
        bind(VAL(b), a);
        return 1;
    }

    if (ta != tb) return 0;  /* different types */

    /* Both are non-REF cells with the same tag */
    switch (ta) {

        case TAG_ATM:
        case TAG_INT:
            /* Already handled by the a==b check above; reaching here means fail. */
            return 0;

        case TAG_STR:
            ai = VAL(a);
            bi = VAL(b);
            fa = heap_get(ai);    /* functor cell of a */
            fb = heap_get(bi);    /* functor cell of b */
            if (fa != fb) return 0;   /* different functor or arity */
            ar = CELL_ARITY(fa);
            for (i = 1; i <= ar; i++) {
                if (!unify(heap_get(ai + i), heap_get(bi + i)))
                    return 0;
            }
            return 1;
    }

    return 0;
}
