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
#ifndef DB_H
#define DB_H

#include "memory.h"

#define MAX_CLAUSES 48

typedef struct {
    uint8_t functor;   /* atom index of predicate name */
    uint8_t arity;
    Cell    head;      /* head term (cell in permanent heap) */
    Cell    body;      /* body term; MKATM(ATOM_TRUE) = fact  */
} ClauseEntry;

extern ClauseEntry clauses[MAX_CLAUSES];
extern uint8_t     clause_count;

/*
 * clause_top: heap word-index boundary.
 * Everything below clause_top is permanent clause storage and must
 * never be reclaimed by query backtracking.
 */
extern uint16_t clause_top;

void    db_init(void);

/*
 * Add a permanent clause.
 * 'term' must already be a stable heap cell (copied to permanent area).
 * Returns 1 on success, 0 if table is full.
 */
uint8_t db_add(Cell term);

/* Find first / next clause index for a given functor/arity (0xFF = none). */
uint8_t db_first(uint8_t functor, uint8_t arity);
uint8_t db_next(uint8_t idx,     uint8_t functor, uint8_t arity);

/* Restore hp, never going below clause_top. */
void hp_restore(uint16_t mark);

#endif /* DB_H */
