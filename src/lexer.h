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
#ifndef LEXER_H
#define LEXER_H

#include "memory.h"

/* Token types */
#define TOK_EOF    0
#define TOK_ATOM   1   /* atom or operator; .atom = atom index */
#define TOK_VAR    2   /* variable;         .vname / .vlen     */
#define TOK_INT    3   /* integer;          .ival               */
#define TOK_LPAREN 4
#define TOK_RPAREN 5
#define TOK_LBRACK 6
#define TOK_RBRACK 7
#define TOK_BAR    8   /* | */
#define TOK_DOT    9   /* '.' followed by whitespace or EOF    */
#define TOK_ERR    10  /* syntax error                          */

typedef struct {
    uint8_t type;
    uint8_t atom;      /* TOK_ATOM: atom index                  */
    int16_t ival;      /* TOK_INT:  value                       */
    char    vname[16]; /* TOK_VAR:  raw name                    */
    uint8_t vlen;      /* TOK_VAR:  length of name              */
} Token;

/* Current token (updated by lex_next()). */
extern Token lex_tok;

/*
 * Point the lexer at a NUL-terminated string buffer and lex the first token.
 * Call lex_next() to advance.
 */
void lex_init(char *buf);

/* Advance to the next token (stored in lex_tok). */
void lex_next(void);

#endif /* LEXER_H */
