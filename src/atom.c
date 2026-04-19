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
#include "atom.h"
#include <string.h>

/*
 * Atom table layout:
 *
 *   ATOM_IDX_BASE ($6000): uint16_t offset[128]
 *     offset[i] = byte offset within ATOM_STR_BASE where atom i's entry lives.
 *
 *   ATOM_STR_BASE ($6100): packed entries
 *     each entry: [1 byte: length n][n bytes: string][1 byte: NUL]
 *
 * Both regions are in fixed memory, not in cc65's BSS.
 */

uint8_t  atom_count = 0;
static uint16_t atom_top = 0;   /* next free byte offset in ATOM_STR_BASE */

#define atom_off   ATOM_IDX_BASE   /* uint16_t* -- the offset table */
#define atom_base  ATOM_STR_BASE   /* uint8_t*  -- the string pool   */

static const char *builtin_names[ATOM_PREDEF] = {
    /* 0-11: core */
    "true", "fail", "[]", ".",
    "is",   "=",    "call", "!",
    "write","nl",   ",",   ";",
    /* 12-28: operators */
    ":-", "->", "\\+", "\\=",
    "=:=", "=\\=", "<", ">", "=<", ">=",
    "+", "-", "*", "/", "mod", "|", "not"
};

void atom_init(void) {
    uint8_t i;
    atom_count = 0;
    atom_top   = 0;
    for (i = 0; i < ATOM_PREDEF; i++) {
        atom_intern(builtin_names[i], (uint8_t)strlen(builtin_names[i]));
    }
}

uint8_t atom_intern(const char *s, uint8_t n) {
    uint8_t  i;
    uint8_t *entry;

    /* Search existing atoms */
    for (i = 0; i < atom_count; i++) {
        entry = atom_base + atom_off[i];
        if (entry[0] == n && memcmp(entry + 1, s, n) == 0)
            return i;
    }

    /* Add new atom */
    if (atom_count >= MAX_ATOMS) return 0xFF;
    if ((uint16_t)(atom_top + n + 2u) > ATOM_STR_BYTES) return 0xFF;

    atom_off[atom_count] = atom_top;
    atom_base[atom_top]             = n;
    memcpy(atom_base + atom_top + 1, s, n);
    atom_base[atom_top + 1 + n]     = 0;        /* NUL terminator */
    atom_top = (uint16_t)(atom_top + n + 2u);

    return atom_count++;
}

const char *atom_str(uint8_t a) {
    if (a >= atom_count) return "?";
    return (const char *)(atom_base + atom_off[a] + 1);
}
