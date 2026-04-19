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
#include "interp.h"
#include "db.h"
#include "heap.h"
#include "trail.h"
#include "unify.h"
#include "atom.h"
#include "write.h"
#include "fileio.h"
#include <cbm.h>

/* ------------------------------------------------------------------ */
/* Result codes: bit0 = success, bit1 = cut encountered               */
/* ------------------------------------------------------------------ */
#define RES_FAIL     0   /* 00 */
#define RES_OK       1   /* 01 */
#define RES_CUT_FAIL 2   /* 10 */
#define RES_CUT_OK   3   /* 11 */

#define RES_SUCCESS(r)  ((r) & 1)
#define RES_CUT(r)      ((r) & 2)

/* ------------------------------------------------------------------ */
/* Clause copy (rename variables)                                      */
/* ------------------------------------------------------------------ */

#define MAX_REN 16

typedef struct { uint16_t old; Cell fresh; } RenEntry;
static RenEntry ren[MAX_REN];
static uint8_t  ren_n;

static Cell copy_cell(Cell c) {
    Cell d;
    uint16_t idx, base, i;
    uint8_t  ar;
    Cell fc;

    d = deref(c);

    switch (TAG(d)) {
        case TAG_REF:
            /* Unbound variable: look up or create fresh */
            idx = VAL(d);
            for (i = 0; i < ren_n; i++)
                if (ren[i].old == idx) return ren[i].fresh;
            if (ren_n < MAX_REN) {
                uint16_t ni = heap_alloc(1);
                Cell nc = MKREF(ni);
                heap_set(ni, nc);
                ren[ren_n].old   = idx;
                ren[ren_n].fresh = nc;
                ren_n++;
                return nc;
            }
            return d;   /* table full: reuse (rare) */

        case TAG_ATM:
        case TAG_INT:
            return d;

        case TAG_STR:
            fc   = heap_get(VAL(d));
            ar   = CELL_ARITY(fc);
            base = heap_alloc((uint8_t)(ar + 1));
            heap_set(base, fc);
            for (i = 0; i < ar; i++)
                heap_set((uint16_t)(base + 1 + i),
                         copy_cell(heap_get((uint16_t)(VAL(d) + 1 + i))));
            return MKSTR(base);
    }
    return d;
}

Cell copy_term(Cell c) {
    ren_n = 0;
    return copy_cell(c);
}

static void copy_clause(Cell head, Cell body,
                        Cell *new_head, Cell *new_body) {
    ren_n    = 0;
    *new_head = copy_cell(head);
    *new_body = copy_cell(body);
}

/* ------------------------------------------------------------------ */
/* Arithmetic evaluator                                                 */
/* ------------------------------------------------------------------ */

static uint8_t eval(Cell expr, int16_t *out) {
    Cell e = deref(expr);
    int16_t v1, v2;
    uint8_t at;

    if (TAG(e) == TAG_INT) { *out = CELL_INT(e); return 1; }

    if (TAG(e) == TAG_STR) {
        Cell fc = heap_get(VAL(e));
        uint8_t ar = CELL_ARITY(fc);
        at = CELL_ATOM(fc);

        if (ar == 2) {
            if (!eval(heap_get(VAL(e)+1), &v1)) return 0;
            if (!eval(heap_get(VAL(e)+2), &v2)) return 0;
            switch (at) {
                case ATOM_PLUS:  *out = (int16_t)(v1 + v2); return 1;
                case ATOM_MINUS: *out = (int16_t)(v1 - v2); return 1;
                case ATOM_TIMES: *out = (int16_t)(v1 * v2); return 1;
                case ATOM_DIV:   if (!v2) return 0;
                                 *out = (int16_t)(v1 / v2); return 1;
                case ATOM_MOD:   if (!v2) return 0;
                                 *out = (int16_t)(v1 % v2); return 1;
            }
        }
        if (ar == 1 && at == ATOM_MINUS) {
            if (!eval(heap_get(VAL(e)+1), &v1)) return 0;
            *out = (int16_t)(-v1); return 1;
        }
    }

    /* Plain integer atom should not occur, but handle 0-arity */
    if (TAG(e) == TAG_ATM) { *out = 0; return 1; }

    return 0;  /* unbound or non-numeric */
}

/* ------------------------------------------------------------------ */
/* Goal decomposition helpers                                           */
/* ------------------------------------------------------------------ */

/* Is cell g a 0-arity atom with index 'at'? */
#define IS_ATOM0(g, at) \
    (TAG(g) == TAG_ATM && CELL_ATOM(g) == (at))

/* Is cell g a compound with functor 'at'/ar? */
#define IS_STR(g, at, ar) \
    (TAG(g) == TAG_STR \
     && CELL_ATOM(heap_get(VAL(g))) == (at) \
     && CELL_ARITY(heap_get(VAL(g))) == (ar))

#define ARG(g, i)  heap_get((uint16_t)(VAL(g) + 1 + (i)))

/* Build a conjunction cell (A, B) on the heap */
static Cell make_conj(Cell a, Cell b) {
    uint16_t base = heap_alloc(3);
    heap_set(base,     MKFUN(ATOM_COMMA, 2));
    heap_set(base + 1, a);
    heap_set(base + 2, b);
    return MKSTR(base);
}

/* ------------------------------------------------------------------ */
/* Core interpreter: solve_g(goal, continuation)                        */
/*                                                                      */
/* Returns RES_OK / RES_FAIL / RES_CUT_OK / RES_CUT_FAIL              */
/* ------------------------------------------------------------------ */

/* Tail-call trampoline: update goal/cont and loop instead of recursing.
   Only genuinely non-tail calls (user-defined clause body, negation probe,
   cut continuation) recurse.                                               */
static uint8_t solve_g(Cell goal, Cell cont) {
    Cell     g;
    Cell     a, b;
    Cell     t_assert, perm_assert, arg;
    uint8_t  f, ar;
    uint16_t hp_mark, tp_mark;
    int16_t  v1, v2;
    uint8_t  ci, r;
    Cell     nh, nb;
    Cell     fc;
    static uint8_t atom_load   = 0xFF;
    static uint8_t atom_save   = 0xFF;
    static uint8_t atom_assert = 0xFF;

restart:
    g = deref(goal);

    /* ---- Conjunction: tail-call into left branch with extended cont ---- */
    if (IS_STR(g, ATOM_COMMA, 2)) {
        a = ARG(g, 0);
        b = ARG(g, 1);
        goal = a;
        cont = IS_ATOM0(cont, ATOM_TRUE) ? b : make_conj(b, cont);
        goto restart;
    }

    /* ---- true ---- */
    if (IS_ATOM0(g, ATOM_TRUE)) {
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- fail ---- */
    if (IS_ATOM0(g, ATOM_FAIL)) return RES_FAIL;

    /* ---- ! (cut): must recurse to capture result before OR-ing cut bit ---- */
    if (IS_ATOM0(g, ATOM_CUT)) {
        r = solve_g(cont, MKATM(ATOM_TRUE));
        return (uint8_t)(r | 2);
    }

    /* ---- nl ---- */
    if (IS_ATOM0(g, ATOM_NL)) {
        cbm_k_chrout(13);   /* PETSCII RETURN: next line + scroll */
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- write(X) ---- */
    if (IS_STR(g, ATOM_WRITE, 1)) {
        write_term(deref(ARG(g, 0)));
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- =(A, B) unification ---- */
    if (IS_STR(g, ATOM_EQ, 2)) {
        if (!unify(ARG(g, 0), ARG(g, 1))) return RES_FAIL;
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- \=(A, B) ---- */
    if (IS_STR(g, ATOM_NEQ, 2)) {
        hp_mark = hp; tp_mark = tp;
        r = (uint8_t)unify(ARG(g, 0), ARG(g, 1));
        untrail(tp_mark); hp_restore(hp_mark);
        if (r) return RES_FAIL;
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- \+(G) negation as failure ---- */
    if (IS_STR(g, ATOM_NOT, 1) || IS_STR(g, ATOM_NOT2, 1)) {
        hp_mark = hp; tp_mark = tp;
        r = (uint8_t)(solve_g(ARG(g, 0), MKATM(ATOM_TRUE)) & 1);
        untrail(tp_mark); hp_restore(hp_mark);
        if (r) return RES_FAIL;
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- is(X, Expr) ---- */
    if (IS_STR(g, ATOM_IS, 2)) {
        if (!eval(ARG(g, 1), &v1)) return RES_FAIL;
        if (!unify(ARG(g, 0), MKINT(v1))) return RES_FAIL;
        if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
        goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
    }

    /* ---- arithmetic comparisons ---- */
    if (TAG(g) == TAG_STR) {
        fc = heap_get(VAL(g));
        if (CELL_ARITY(fc) == 2) {
            f = CELL_ATOM(fc);
            if (f == ATOM_EQAR || f == ATOM_NEQAR ||
                f == ATOM_LT   || f == ATOM_GT    ||
                f == ATOM_LE   || f == ATOM_GE) {
                if (!eval(ARG(g,0), &v1)) return RES_FAIL;
                if (!eval(ARG(g,1), &v2)) return RES_FAIL;
                r = 0;
                switch (f) {
                    case ATOM_EQAR:  r = (v1 == v2); break;
                    case ATOM_NEQAR: r = (v1 != v2); break;
                    case ATOM_LT:    r = (v1 <  v2); break;
                    case ATOM_GT:    r = (v1 >  v2); break;
                    case ATOM_LE:    r = (v1 <= v2); break;
                    case ATOM_GE:    r = (v1 >= v2); break;
                }
                if (!r) return RES_FAIL;
                if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
                goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
            }
        }
    }

    /* ---- call(G) ---- */
    if (IS_STR(g, ATOM_CALL, 1)) {
        goal = ARG(g, 0); goto restart;
    }

    /* ---- load(File) / save(File) ---- */
    if (TAG(g) == TAG_STR) {
        if (atom_load == 0xFF) atom_load = atom_intern("load", 4);
        if (atom_save == 0xFF) atom_save = atom_intern("save", 4);
        fc = heap_get(VAL(g));
        if (CELL_ARITY(fc) == 1 &&
            (CELL_ATOM(fc) == atom_load || CELL_ATOM(fc) == atom_save)) {
            arg = deref(ARG(g, 0));
            if (TAG(arg) != TAG_ATM) return RES_FAIL;
            if (CELL_ATOM(fc) == atom_load)
                file_load(atom_str(CELL_ATOM(arg)));
            else
                file_save(atom_str(CELL_ATOM(arg)));
            if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
            goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
        }
    }

    /* ---- assert(T) ---- */
    if (TAG(g) == TAG_STR) {
        if (atom_assert == 0xFF) atom_assert = atom_intern("assert", 6);
        fc = heap_get(VAL(g));
        if (CELL_ATOM(fc) == atom_assert && CELL_ARITY(fc) == 1) {
            t_assert = deref(ARG(g, 0));
            ren_n = 0;
            perm_assert = copy_cell(t_assert);
            clause_top = hp;
            if (!db_add(perm_assert)) return RES_FAIL;
            if (IS_ATOM0(cont, ATOM_TRUE)) return RES_OK;
            goal = cont; cont = MKATM(ATOM_TRUE); goto restart;
        }
    }

    /* ---- User-defined predicate ---- */
    if (TAG(g) == TAG_ATM) {
        f  = CELL_ATOM(g);
        ar = 0;
    } else if (TAG(g) == TAG_STR) {
        fc = heap_get(VAL(g));
        f  = CELL_ATOM(fc);
        ar = CELL_ARITY(fc);
    } else {
        return RES_FAIL;
    }

    for (ci = db_first(f, ar); ci != 0xFF; ci = db_next(ci, f, ar)) {
        hp_mark = hp;
        tp_mark = tp;
        copy_clause(clauses[ci].head, clauses[ci].body, &nh, &nb);
        if (unify(g, nh)) {
            r = solve_g(nb, cont);
            if (RES_SUCCESS(r)) return RES_OK;
            if (RES_CUT(r))     return RES_FAIL;
        }
        untrail(tp_mark);
        hp_restore(hp_mark);
    }
    return RES_FAIL;
}

/* ------------------------------------------------------------------ */
/* Public interface                                                     */
/* ------------------------------------------------------------------ */

uint8_t solve(Cell goal) {
    return (uint8_t)(solve_g(goal, MKATM(ATOM_TRUE)) & 1);
}
