/***************************************************************************
--                      Prolog/C64 - lexer
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
#include "lexer.h"
#include "atom.h"
#include <string.h>

Token lex_tok;

static char *lbuf;   /* current buffer */
static uint8_t lpos; /* position in buffer */

/* ------------------------------------------------------------------ */
/* Character classification                                             */
/* ------------------------------------------------------------------ */

static uint8_t is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static uint8_t is_digit(char c) {
    return c >= '0' && c <= '9';
}
static uint8_t is_alnum(char c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}
static uint8_t is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
/* Graphic operator characters (can form multi-char operator tokens) */
static uint8_t is_graphic(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '\\' || c == '^' || c == '<' || c == '>' ||
           c == '=' || c == '~' || c == '?' || c == '@' ||
           c == '#' || c == '&';
}

/* ------------------------------------------------------------------ */
/* Helper: intern a length-bounded string as a token atom              */
/* ------------------------------------------------------------------ */

static void set_atom(const char *s, uint8_t n) {
    uint8_t a = atom_intern(s, n);
    if (a == 0xFF) { lex_tok.type = TOK_ERR; return; }
    lex_tok.type = TOK_ATOM;
    lex_tok.atom = a;
}

/* ------------------------------------------------------------------ */
/* lex_next: advance to the next token                                  */
/* ------------------------------------------------------------------ */

void lex_next(void) {
    char c;
    char buf[16];
    uint8_t n;

    /* Skip whitespace and comments */
restart:
    while (is_space(lbuf[lpos])) lpos++;

    /* Line comment: % to end-of-buffer */
    if (lbuf[lpos] == '%') {
        while (lbuf[lpos] && lbuf[lpos] != '\n') lpos++;
        goto restart;
    }

    c = lbuf[lpos];

    /* ---- EOF ---- */
    if (c == '\0') { lex_tok.type = TOK_EOF; return; }

    /* ---- Punctuation ---- */
    if (c == '(') { lpos++; lex_tok.type = TOK_LPAREN; return; }
    if (c == ')') { lpos++; lex_tok.type = TOK_RPAREN; return; }
    if (c == '[') { lpos++; lex_tok.type = TOK_LBRACK; return; }
    if (c == ']') { lpos++; lex_tok.type = TOK_RBRACK; return; }
    if (c == '|') { lpos++; lex_tok.type = TOK_BAR;    return; }

    /* ---- Comma (also an operator, but keep as ATOM) ---- */
    if (c == ',') {
        lpos++;
        lex_tok.type = TOK_ATOM;
        lex_tok.atom = ATOM_COMMA;
        return;
    }

    /* ---- ! (cut) ---- */
    if (c == '!') {
        lpos++;
        lex_tok.type = TOK_ATOM;
        lex_tok.atom = ATOM_CUT;
        return;
    }

    /* ---- ; (semicolon operator) ---- */
    if (c == ';') {
        lpos++;
        lex_tok.type = TOK_ATOM;
        lex_tok.atom = ATOM_SEMI;
        return;
    }

    /* ---- End-of-clause dot: '.' followed by whitespace or EOF ---- */
    if (c == '.') {
        char next = lbuf[lpos + 1];
        if (next == '\0' || is_space(next)) {
            lpos++;
            lex_tok.type = TOK_DOT;
            return;
        }
        /* Otherwise it's a graphic sequence starting with '.' */
    }

    /* ---- Integers ---- */
    if (is_digit(c)) {
        int16_t v = 0;
        while (is_digit(lbuf[lpos])) {
            v = (int16_t)(v * 10 + (lbuf[lpos] - '0'));
            lpos++;
        }
        lex_tok.type = TOK_INT;
        lex_tok.ival = v;
        return;
    }

    /* ---- Variables: start with uppercase or '_' ---- */
    if (c == '_' || (c >= 'A' && c <= 'Z')) {
        n = 0;
        while (is_alnum(lbuf[lpos]) && n < 15) {
            lex_tok.vname[n++] = lbuf[lpos++];
        }
        while (is_alnum(lbuf[lpos])) lpos++; /* consume rest silently */
        lex_tok.vname[n] = '\0';
        lex_tok.vlen  = n;
        lex_tok.type  = TOK_VAR;
        return;
    }

    /* ---- Lowercase atoms ---- */
    if (c >= 'a' && c <= 'z') {
        n = 0;
        while (is_alnum(lbuf[lpos]) && n < 15) buf[n++] = lbuf[lpos++];
        while (is_alnum(lbuf[lpos])) lpos++;
        set_atom(buf, n);
        return;
    }

    /* ---- Quoted atom: 'text' ---- */
    if (c == '\'') {
        lpos++;
        n = 0;
        while (lbuf[lpos] && lbuf[lpos] != '\'') {
            if (n < 15) buf[n++] = lbuf[lpos];
            lpos++;
        }
        if (lbuf[lpos] == '\'') lpos++;
        set_atom(buf, n);
        return;
    }

    /* ---- Graphic / operator sequences ---- */
    if (is_graphic(c) || c == ':' || c == '\\' || c == '.') {
        n = 0;
        while ((is_graphic(lbuf[lpos]) || lbuf[lpos] == ':'
                || lbuf[lpos] == '\\') && n < 15) {
            buf[n++] = lbuf[lpos++];
        }
        /* Handle '.' specially: if part of a graphic sequence, include it */
        set_atom(buf, n);
        return;
    }

    /* Unknown character: skip and try again */
    lpos++;
    lex_tok.type = TOK_ERR;
}

void lex_init(char *buf) {
    lbuf = buf;
    lpos = 0;
    lex_next();   /* prime the lookahead */
}
