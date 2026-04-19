/***************************************************************************
--                      Prolog/C64 - main
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
#include <conio.h>
#include <cbm.h>
#include <string.h>
#include "memory.h"
#include "heap.h"
#include "trail.h"
#include "unify.h"
#include "atom.h"
#include "lexer.h"
#include "parser.h"
#include "write.h"
#include "db.h"
#include "interp.h"

/* ------------------------------------------------------------------ */
/* Line input (handles C64 DEL key = CHR$(20))                         */
/* ------------------------------------------------------------------ */

static uint8_t readline(char *buf, uint8_t max) {
    uint8_t n = 0;
    char c;
    for (;;) {
        c = cgetc();
        if (c == 13) { cbm_k_chrout(13); buf[n] = '\0'; return n; }
        if ((c == 20 || c == 8) && n > 0) { n--; cbm_k_chrout(20); continue; }
        if (c >= 32 && n < (uint8_t)(max - 1)) { buf[n++] = c; cbm_k_chrout((uint8_t)c); }
    }
}

/* ------------------------------------------------------------------ */
/* Parse one term from the input buffer                                 */
/* ------------------------------------------------------------------ */

static Cell do_parse(char *buf, uint8_t len) {
    /* Append '.' terminator if absent */
    if (len == 0 || buf[len-1] != '.') {
        buf[len]   = '.';
        buf[len+1] = '\0';
    }
    parse_error  = 0;
    parse_vars_reset();
    lex_init(buf);
    return parse_term(1200u);
}

/* ------------------------------------------------------------------ */
/* Print variable bindings after a successful query                     */
/* ------------------------------------------------------------------ */

static void print_bindings(void) {
    uint8_t i, n;
    n = parse_var_count();
    if (n == 0) { write_str("true."); cbm_k_chrout(13); return; }
    for (i = 0; i < n; i++) {
        const char *name = parse_var_name(i);
        if (name[0] == '_') continue;   /* skip anonymous */
        write_str(name); write_str(" = ");
        write_term(parse_var_cell(i));
        cbm_k_chrout(13);
    }
    write_str("true."); cbm_k_chrout(13);
}

/* ------------------------------------------------------------------ */
/* Is term a clause (top-level :- or plain head)?                       */
/* ------------------------------------------------------------------ */

static uint8_t is_clause(Cell term) {
    if (TAG(term) == TAG_STR) {
        Cell fc = heap_get(VAL(term));
        if (CELL_ATOM(fc) == ATOM_NECK && CELL_ARITY(fc) == 2) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

void main(void) {
    char    *inbuf = INBUF_BASE;
    Cell     term;
    uint16_t hp_query;
    uint16_t tp_query;
    uint8_t  len;

#define NL cbm_k_chrout(13)

    cbm_k_chrout(147);  /* PETSCII CLR: clear screen + home cursor */
    cbm_k_chrout(14);   /* switch to lowercase character set */

    write_str("prolog/c64"); NL;
    write_str("----------"); NL;
    write_str("?- to query, head:-body. to assert"); NL;
    NL;

    heap_init();
    trail_init();
    atom_init();
    db_init();

    for (;;) {
        write_str("?- ");
        len = readline(inbuf, (uint8_t)INBUF_BYTES);
        if (len == 0) continue;

        /* Save query-time heap/trail state */
        hp_query = hp;
        tp_query = tp;

        term = do_parse(inbuf, len);

        if (parse_error) {
            hp = hp_query;
            write_str("error."); NL; NL;
            continue;
        }

        /* ---- Clause assertion ---- */
        if (is_clause(term)) {
            if (db_add(term)) {
                write_str("ok."); NL; NL;
            } else {
                write_str("db full."); NL; NL;
                hp = hp_query;
            }
            continue;
        }

        /* ---- Query execution ---- */
        {
            uint8_t ok = solve(term);
            if (ok) {
                print_bindings();
            } else {
                write_str("false."); NL;
            }
        }
        NL;

        /* Reclaim query heap */
        untrail(tp_query);
        hp_restore(hp_query);
    }
}
