/***************************************************************************
--                      Prolog/C64 - db
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
#include "db.h"
#include "heap.h"
#include "unify.h"
#include "atom.h"

ClauseEntry clauses[MAX_CLAUSES];
uint8_t     clause_count = 0;
uint16_t    clause_top   = 0;

void db_init(void) {
    clause_count = 0;
    clause_top   = 0;
}

void hp_restore(uint16_t mark) {
    hp = (mark < clause_top) ? clause_top : mark;
}

/* Extract functor atom and arity from a head term. */
static void head_fa(Cell head, uint8_t *f, uint8_t *a) {
    Cell h = deref(head);
    if (TAG(h) == TAG_ATM) {
        *f = CELL_ATOM(h);
        *a = 0;
    } else if (TAG(h) == TAG_STR) {
        Cell fc = heap_get(VAL(h));
        *f = CELL_ATOM(fc);
        *a = CELL_ARITY(fc);
    } else {
        *f = 0xFF;
        *a = 0;
    }
}

uint8_t db_add(Cell term) {
    Cell head, body;
    uint8_t f, a;

    if (clause_count >= MAX_CLAUSES) return 0;

    /* Decompose :-(Head, Body) or plain fact */
    if (TAG(term) == TAG_STR) {
        Cell fc = heap_get(VAL(term));
        if (CELL_ATOM(fc) == ATOM_NECK && CELL_ARITY(fc) == 2) {
            head = heap_get(VAL(term) + 1);
            body = heap_get(VAL(term) + 2);
            goto store;
        }
    }
    head = term;
    body = MKATM(ATOM_TRUE);

store:
    head_fa(head, &f, &a);
    if (f == 0xFF) return 0;

    clauses[clause_count].functor = f;
    clauses[clause_count].arity   = a;
    clauses[clause_count].head    = head;
    clauses[clause_count].body    = body;
    clause_count++;

    clause_top = hp;   /* lock this clause into the permanent heap */
    return 1;
}

uint8_t db_first(uint8_t f, uint8_t a) {
    uint8_t i;
    for (i = 0; i < clause_count; i++)
        if (clauses[i].functor == f && clauses[i].arity == a) return i;
    return 0xFF;
}

uint8_t db_next(uint8_t idx, uint8_t f, uint8_t a) {
    uint8_t i;
    for (i = (uint8_t)(idx + 1); i < clause_count; i++)
        if (clauses[i].functor == f && clauses[i].arity == a) return i;
    return 0xFF;
}
