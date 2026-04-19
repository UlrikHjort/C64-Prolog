/***************************************************************************
--                      Prolog/C64 - fileio
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
#include "fileio.h"
#include "memory.h"
#include "atom.h"
#include "heap.h"
#include "trail.h"
#include "db.h"
#include "write.h"
#include "lexer.h"
#include "parser.h"
#include <cbm.h>
#include <conio.h>
#include <string.h>

#define FILE_LFN  2   /* logical file number for data transfers */

/* ------------------------------------------------------------------ */
/* Save all database clauses to disk                                   */
/* ------------------------------------------------------------------ */

void file_save(const char *name) {
    char diskname[24];
    uint8_t i, ci;

    /* Build "@:name,S,W" - the '@' prefix overwrites an existing file */
    diskname[0] = '@';
    diskname[1] = ':';
    for (i = 0; name[i] && i < 16; i++) diskname[2 + i] = name[i];
    diskname[2 + i] = ',';
    diskname[3 + i] = 'S';
    diskname[4 + i] = ',';
    diskname[5 + i] = 'W';
    diskname[6 + i] = '\0';

    if (cbm_open(FILE_LFN, 8, 2, diskname)) {
        cputs("save err.\r\n"); return;
    }

    cbm_k_ckout(FILE_LFN);

    for (ci = 0; ci < clause_count; ci++) {
        if (clauses[ci].body == MKATM(ATOM_TRUE)) {
            /* Fact: head. */
            write_term(clauses[ci].head);
        } else {
            /* Rule: head:-body. */
            write_term(clauses[ci].head);
            write_str(":-");
            write_term(clauses[ci].body);
        }
        write_char('.');
        write_char('\r');   /* C64 sequential file line separator */
    }

    cbm_k_clrch();
    cbm_close(FILE_LFN);
}

/* ------------------------------------------------------------------ */
/* Load clauses from disk                                              */
/* ------------------------------------------------------------------ */

/* Translate one byte from ASCII encoding to C64 PETSCII.
 * Called only when a file is detected as ASCII (first char in $61-$7A).
 * Letters, underscore, pipe and backslash have different codes in PETSCII. */
static uint8_t ascii_to_petscii(uint8_t c) {
    if (c >= 0x61u && c <= 0x7Au) return (uint8_t)(c - 0x20u); /* a-z -> $41-$5A */
    if (c >= 0x41u && c <= 0x5Au) return (uint8_t)(c + 0x80u); /* A-Z -> $C1-$DA */
    if (c == 0x5Fu) return 0xA4u;  /* _ */
    if (c == 0x7Cu) return 0xDDu;  /* | */
    if (c == 0x5Cu) return 0xBFu;  /* \ */
    return c;
}

void file_load(const char *name) {
    /* Re-use INBUF_BASE.  The REPL's do_parse has already finished by the
     * time solve() calls file_load, so overwriting the input buffer here
     * is safe.  parse_vars_reset() at the end cleans up the var table so
     * print_bindings() sees 0 vars when we return. */
    char    *buf = (char *)INBUF_BASE;
    uint8_t  n;
    uint8_t  c, st;
    uint8_t  is_ascii;   /* 1 = file uses ASCII encoding, needs conversion */
    Cell      term;
    uint16_t  hp_save;

    if (cbm_open(FILE_LFN, 8, 2, name)) {
        cputs("load err.\r\n"); return;
    }
    if (cbm_k_chkin(FILE_LFN)) {
        cputs("chkin err.\r\n"); cbm_close(FILE_LFN); return;
    }

    /* Detect encoding from the first non-whitespace byte.
     * ASCII files have lowercase letters in $61-$7A; PETSCII uses $41-$5A. */
    do {
        c  = cbm_k_chrin();
        st = cbm_k_readst();
        if (st & 0x40) goto done;
    } while (c <= ' ');

    is_ascii = (c >= 0x61u && c <= 0x7Au) ? 1u : 0u;

    for (;;) {
        /* Convert and store first byte of clause */
        if (is_ascii) c = ascii_to_petscii(c);
        n = 0;
        buf[n++] = (char)c;

        /* Read remainder of clause up to '.' */
        while (n < (uint8_t)(INBUF_BYTES - 2u)) {
            c  = cbm_k_chrin();
            st = cbm_k_readst();
            if (c == '\r' || c == '\n') c = ' ';
            else if (is_ascii) c = ascii_to_petscii(c);
            buf[n++] = (char)c;
            if (c == '.' || (st & 0x40)) break;
        }
        buf[n] = '\0';

        /* Ensure clause ends with '.' for the parser */
        if (buf[n - 1] != '.') { buf[n] = '.'; buf[n + 1] = '\0'; }

        /* Parse and assert */
        hp_save    = hp;
        parse_error = 0;
        parse_vars_reset();
        lex_init(buf);
        term = parse_term(1200u);

        if (parse_error) {
            hp = hp_save;
        } else if (!db_add(term)) {
            cputs("db full.\r\n");
            goto done;
        }

        if (st & 0x40) break;   /* EOF reached inside last clause */

        /* Skip whitespace before next clause */
        do {
            c  = cbm_k_chrin();
            st = cbm_k_readst();
            if (st & 0x40) goto done;
        } while (c <= ' ');
    }

done:
    cbm_k_clrch();
    cbm_close(FILE_LFN);
    parse_vars_reset();
}
