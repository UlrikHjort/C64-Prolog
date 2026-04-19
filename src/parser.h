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
#ifndef PARSER_H
#define PARSER_H

#include "memory.h"

/*
 * Parse a complete Prolog term from the current lexer state.
 * Returns the Cell for the parsed term, built on the heap.
 *
 * parse_error is set to 1 on a syntax error; the returned Cell is
 * then meaningless.  Reset parse_error to 0 before calling.
 */
Cell parse_term(uint16_t max_prec);

extern uint8_t parse_error;

/*
 * Variable table for the current parse.
 * Call parse_vars_reset() before parsing each new clause/query.
 * parse_var_count() returns the number of variables found.
 * parse_var_name(i) / parse_var_cell(i) access the i-th variable.
 */
void        parse_vars_reset(void);
uint8_t     parse_var_count(void);
const char *parse_var_name(uint8_t i);
Cell        parse_var_cell(uint8_t i);

#endif /* PARSER_H */
