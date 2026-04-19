/***************************************************************************
--                      Prolog/C64 - write
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
#include "write.h"
#include "unify.h"
#include "atom.h"
#include "heap.h"
#include <cbm.h>

/* All output goes through the KERNAL's CHROUT so scrolling works.
   cbm_k_ckout / cbm_k_clrch in fileio.c switch the channel for file I/O. */

void write_char(char c) {
    cbm_k_chrout((uint8_t)c);
}

void write_str(const char *s) {
    while (*s) write_char(*s++);
}

/* ------------------------------------------------------------------ */

static void write_uint(uint16_t u) {
    char buf[6];
    uint8_t i = 0;
    if (u == 0) { write_char('0'); return; }
    while (u > 0) {
        buf[i++] = (char)('0' + (uint8_t)(u % 10));
        u = (uint16_t)(u / 10);
    }
    while (i > 0) write_char(buf[--i]);
}

static void write_int(int16_t n) {
    if (n < 0) {
        write_char('-');
        write_uint((uint16_t)(-n));
    } else {
        write_uint((uint16_t)n);
    }
}

static void write_list_tail(Cell tail) {
    Cell tfc;
    uint16_t ti;

    for (;;) {
        tail = deref(tail);

        if (TAG(tail) == TAG_ATM && CELL_ATOM(tail) == ATOM_NIL) {
            write_char(']');
            return;
        }

        if (TAG(tail) == TAG_STR) {
            ti  = VAL(tail);
            tfc = heap_get(ti);
            if (CELL_ATOM(tfc) == ATOM_DOT && CELL_ARITY(tfc) == 2) {
                write_char(',');
                write_term(heap_get(ti + 1));
                tail = heap_get(ti + 2);
                continue;
            }
        }

        write_char('|');
        write_term(tail);
        write_char(']');
        return;
    }
}

void write_term(Cell c) {
    uint8_t  at, ar, i;
    uint16_t idx;
    Cell fc;

    c = deref(c);

    switch (TAG(c)) {

        case TAG_REF:
            write_char('_'); write_char('G');
            write_uint(VAL(c));
            break;

        case TAG_INT:
            write_int(CELL_INT(c));
            break;

        case TAG_ATM:
            write_str(atom_str(CELL_ATOM(c)));
            break;

        case TAG_STR:
            idx = VAL(c);
            fc  = heap_get(idx);
            at  = CELL_ATOM(fc);
            ar  = CELL_ARITY(fc);

            if (at == ATOM_DOT && ar == 2) {
                write_char('[');
                write_term(heap_get(idx + 1));
                write_list_tail(heap_get(idx + 2));
                break;
            }

            write_str(atom_str(at));
            if (ar > 0) {
                write_char('(');
                for (i = 1; i <= ar; i++) {
                    if (i > 1) write_char(',');
                    write_term(heap_get(idx + i));
                }
                write_char(')');
            }
            break;
    }
}
