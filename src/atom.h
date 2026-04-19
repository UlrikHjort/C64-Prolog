/***************************************************************************
--                      Prolog/C64 - atom
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
#ifndef ATOM_H
#define ATOM_H

#include "memory.h"

#define MAX_ATOMS   ATOM_IDX_MAX   /* 128 */

/*
 * Pre-defined atoms, guaranteed to be interned at indices 0..ATOM_PREDEF-1.
 * The order here must match builtin_names[] in atom.c exactly.
 */

/* Core */
#define ATOM_TRUE    0
#define ATOM_FAIL    1
#define ATOM_NIL     2    /* [] */
#define ATOM_DOT     3    /* '.' */
#define ATOM_IS      4
#define ATOM_EQ      5    /* = */
#define ATOM_CALL    6
#define ATOM_CUT     7    /* ! */
#define ATOM_WRITE   8
#define ATOM_NL      9
#define ATOM_COMMA   10   /* , */
#define ATOM_SEMI    11   /* ; */

/* Operators needed by lexer / parser */
#define ATOM_NECK    12   /* :- */
#define ATOM_ARROW   13   /* -> */
#define ATOM_NOT     14   /* \+ */
#define ATOM_NEQ     15   /* \= */
#define ATOM_EQAR    16   /* =:= */
#define ATOM_NEQAR   17   /* =\= */
#define ATOM_LT      18   /* < */
#define ATOM_GT      19   /* > */
#define ATOM_LE      20   /* =< */
#define ATOM_GE      21   /* >= */
#define ATOM_PLUS    22   /* + */
#define ATOM_MINUS   23   /* - */
#define ATOM_TIMES   24   /* * */
#define ATOM_DIV     25   /* / */
#define ATOM_MOD     26   /* mod */
#define ATOM_BAR     27   /* | */
#define ATOM_NOT2    28   /* not  (alias for \+) */

#define ATOM_PREDEF  29

void    atom_init(void);
uint8_t atom_intern(const char *s, uint8_t n);
const char *atom_str(uint8_t a);

extern uint8_t atom_count;

#endif /* ATOM_H */
