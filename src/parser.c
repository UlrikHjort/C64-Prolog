/***************************************************************************
--                      Prolog/C64 - parser
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
#include "parser.h"
#include "lexer.h"
#include "atom.h"
#include "heap.h"
#include <string.h>
#include <conio.h>

uint8_t parse_error = 0;

/* ------------------------------------------------------------------ */
/* Variable table                                                       */
/* ------------------------------------------------------------------ */

#define MAX_VARS 16

typedef struct {
    char    name[12];
    uint8_t nlen;
    Cell    cell;
} VarEntry;

static VarEntry var_tab[MAX_VARS];
static uint8_t  var_cnt;

void        parse_vars_reset(void) { var_cnt = 0; }
uint8_t     parse_var_count(void)  { return var_cnt; }
const char *parse_var_name(uint8_t i) { return var_tab[i].name; }
Cell        parse_var_cell(uint8_t i) { return var_tab[i].cell; }

static Cell get_var(const char *name, uint8_t nlen) {
    uint8_t  i;
    uint16_t idx;
    Cell c;
    uint8_t  copy;

    if (nlen == 1 && name[0] == '_') {
        idx = heap_alloc(1);
        heap_set(idx, MKREF(idx));
        return MKREF(idx);
    }
    for (i = 0; i < var_cnt; i++) {
        if (var_tab[i].nlen == nlen &&
            memcmp(var_tab[i].name, name, nlen) == 0)
            return var_tab[i].cell;
    }
    if (var_cnt >= MAX_VARS) { parse_error = 1; return MKATM(ATOM_FAIL); }
    idx = heap_alloc(1);
    heap_set(idx, MKREF(idx));
    c = MKREF(idx);
    copy = (nlen < 11u) ? nlen : 11u;
    memcpy(var_tab[var_cnt].name, name, copy);
    var_tab[var_cnt].name[copy] = '\0';
    var_tab[var_cnt].nlen = nlen;
    var_tab[var_cnt].cell = c;
    var_cnt++;
    return c;
}

/* ------------------------------------------------------------------ */
/* Operator table                                                       */
/* ------------------------------------------------------------------ */

#define OP_FX   1
#define OP_FY   2
#define OP_XFX  3
#define OP_XFY  4
#define OP_YFX  5

typedef struct {
    uint8_t  atom;
    uint8_t  type;
    uint16_t prec;
} OpDef;

static const OpDef infix_ops[] = {
    { ATOM_NECK,  OP_XFX, 1200 },
    { ATOM_SEMI,  OP_XFY, 1100 },
    { ATOM_ARROW, OP_XFY, 1050 },
    { ATOM_COMMA, OP_XFY, 1000 },
    { ATOM_EQ,    OP_XFX,  700 },
    { ATOM_NEQ,   OP_XFX,  700 },
    { ATOM_IS,    OP_XFX,  700 },
    { ATOM_EQAR,  OP_XFX,  700 },
    { ATOM_NEQAR, OP_XFX,  700 },
    { ATOM_LT,    OP_XFX,  700 },
    { ATOM_GT,    OP_XFX,  700 },
    { ATOM_LE,    OP_XFX,  700 },
    { ATOM_GE,    OP_XFX,  700 },
    { ATOM_PLUS,  OP_YFX,  500 },
    { ATOM_MINUS, OP_YFX,  500 },
    { ATOM_TIMES, OP_YFX,  400 },
    { ATOM_DIV,   OP_YFX,  400 },
    { ATOM_MOD,   OP_YFX,  400 },
    { 0xFF, 0, 0 }
};

static const OpDef prefix_ops[] = {
    { ATOM_NECK,  OP_FX,  1200 },
    { ATOM_NOT,   OP_FY,   900 },
    { ATOM_NOT2,  OP_FY,   900 },
    { ATOM_MINUS, OP_FY,   200 },
    { ATOM_PLUS,  OP_FY,   200 },
    { 0xFF, 0, 0 }
};

static const OpDef *find_infix(uint8_t atom) {
    const OpDef *p;
    for (p = infix_ops; p->atom != 0xFF; p++)
        if (p->atom == atom) return p;
    return 0;
}

static const OpDef *find_prefix(uint8_t atom) {
    const OpDef *p;
    for (p = prefix_ops; p->atom != 0xFF; p++)
        if (p->atom == atom) return p;
    return 0;
}

static uint16_t op_rbp(const OpDef *op) {
    if (op->type == OP_XFY) return op->prec;
    return (uint16_t)(op->prec - 1u);
}

/* ------------------------------------------------------------------ */
/* Heap term builders                                                   */
/* ------------------------------------------------------------------ */

static Cell mkstr(uint8_t at, uint8_t ar, Cell *args) {
    uint16_t base;
    uint8_t i;
    if (ar == 0) return MKATM(at);
    base = heap_alloc((uint8_t)(ar + 1));
    heap_set(base, MKFUN(at, ar));
    for (i = 0; i < ar; i++)
        heap_set((uint16_t)(base + 1u + i), args[i]);
    return MKSTR(base);
}

static Cell mkcons(Cell head, Cell tail) {
    Cell args[2];
    args[0] = head; args[1] = tail;
    return mkstr(ATOM_DOT, 2, args);
}

/* ------------------------------------------------------------------ */
/* Forward declarations (mutual recursion)                              */
/* ------------------------------------------------------------------ */

static Cell parse_primary(void);
Cell parse_term(uint16_t max_prec);  /* exported, no static */

/* ------------------------------------------------------------------ */
/* Error helper / expect                                               */
/* ------------------------------------------------------------------ */

static Cell perr(const char *msg) {
    cputs("SYNTAX: "); cputs(msg); cputs("\r\n");
    parse_error = 1;
    return MKATM(ATOM_FAIL);
}

static void expect(uint8_t tok_type) {
    if (lex_tok.type == tok_type) {
        lex_next();
    } else {
        parse_error = 1;
        cputs("SYNTAX: unexpected token\r\n");
    }
}

/* ------------------------------------------------------------------ */
/* parse_list: called after '[' has been consumed                       */
/* ------------------------------------------------------------------ */

static Cell parse_list(void) {
    Cell head, tail;

    if (lex_tok.type == TOK_RBRACK) {
        lex_next();
        return MKATM(ATOM_NIL);
    }

    head = parse_term(999u);
    if (parse_error) return MKATM(ATOM_FAIL);

    if (lex_tok.type == TOK_BAR) {
        lex_next();
        tail = parse_term(999u);
        if (!parse_error) expect(TOK_RBRACK);
    } else if (lex_tok.type == TOK_ATOM && lex_tok.atom == ATOM_COMMA) {
        lex_next();
        tail = parse_list();
    } else {
        tail = MKATM(ATOM_NIL);
        expect(TOK_RBRACK);
    }

    if (parse_error) return MKATM(ATOM_FAIL);
    return mkcons(head, tail);
}

/* ------------------------------------------------------------------ */
/* parse_primary                                                        */
/* ------------------------------------------------------------------ */

static Cell parse_primary(void) {
    Cell c, args[8];
    uint8_t at, ar;
    const OpDef *pfx;
    uint16_t rp;

    switch (lex_tok.type) {

        case TOK_INT:
            c = MKINT(lex_tok.ival);
            lex_next();
            return c;

        case TOK_VAR:
            c = get_var(lex_tok.vname, lex_tok.vlen);
            lex_next();
            return c;

        case TOK_ATOM:
            at = lex_tok.atom;
            lex_next();

            /* functor( args ) */
            if (lex_tok.type == TOK_LPAREN) {
                lex_next();
                ar = 0;
                if (lex_tok.type != TOK_RPAREN) {
                    args[ar++] = parse_term(999u);
                    while (!parse_error
                           && lex_tok.type == TOK_ATOM
                           && lex_tok.atom == ATOM_COMMA
                           && ar < 8) {
                        lex_next();
                        args[ar++] = parse_term(999u);
                    }
                }
                expect(TOK_RPAREN);
                if (parse_error) return MKATM(ATOM_FAIL);
                return mkstr(at, ar, args);
            }

            /* prefix operator */
            pfx = find_prefix(at);
            if (pfx) {
                rp = (pfx->type == OP_FY) ? pfx->prec
                                           : (uint16_t)(pfx->prec - 1u);
                args[0] = parse_term(rp);
                if (parse_error) return MKATM(ATOM_FAIL);
                return mkstr(at, 1, args);
            }

            return MKATM(at);

        case TOK_LPAREN:
            lex_next();
            c = parse_term(1200u);
            if (!parse_error) expect(TOK_RPAREN);
            return c;

        case TOK_LBRACK:
            lex_next();
            return parse_list();

        default:
            return perr("expected term");
    }
}

/* ------------------------------------------------------------------ */
/* parse_term: Pratt / precedence-climbing                              */
/* ------------------------------------------------------------------ */

Cell parse_term(uint16_t max_prec) {
    Cell left, args[2];
    const OpDef *op;

    left = parse_primary();
    if (parse_error) return left;

    for (;;) {
        if (lex_tok.type != TOK_ATOM) break;
        op = find_infix(lex_tok.atom);
        if (!op || op->prec > max_prec) break;

        lex_next();
        args[0] = left;
        args[1] = parse_term(op_rbp(op));
        if (parse_error) return left;
        left = mkstr(op->atom, 2, args);
    }

    return left;
}
