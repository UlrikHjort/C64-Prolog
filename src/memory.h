/***************************************************************************
--                      Prolog/C64 - memory
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
#ifndef MEMORY_H
#define MEMORY_H

/*
 * memory.h -- Core cell type, tag encoding, and fixed memory regions.
 *
 * Every term is a 16-bit Cell.  Low 2 bits = tag; upper 14 bits = value.
 *
 *  TAG_REF  heap word-index; self-referential = unbound variable.
 *  TAG_STR  heap word-index of the functor cell that starts a structure.
 *  TAG_ATM  Atom / functor-cell descriptor.
 *           bits 7:0  = atom-table index (0..127)
 *           bits 13:8 = arity            (0..63,  0 = plain atom)
 *  TAG_INT  Signed 14-bit integer (-8192..8191).
 *
 * Memory map  (code + BSS live in $0801-$4FFF; trail and heap above):
 *
 *   $0801-$4FFF  code, rodata, data, BSS, C software stack (~19 KB)
 *   $5000-$57FF  trail stack            (2 KB, 1024 uint16_t entries)
 *   $5800-$5FFF  choice-point stack     (2 KB)
 *   $6000-$67FF  environment stack      (2 KB)
 *   $6800-$8FFF  heap / global stack    (10 KB, 5120 Cell words)
 *   $9000-$90FF  atom index table       (256 B, 128 uint16_t offsets)
 *   $9100-$93FF  atom string storage    (768 B)
 *   $9400-$95FF  I/O input buffer       (512 B)
 */

#include <stdint.h>

typedef uint16_t Cell;

/* --- Tags --- */
#define TAG_REF  0
#define TAG_STR  1
#define TAG_ATM  2
#define TAG_INT  3

/* --- Extraction --- */
#define TAG(c)         ((uint8_t)((c) & 0x03u))
#define VAL(c)         ((uint16_t)((c) >> 2))

/* --- Constructors --- */
#define MKREF(idx)     ((Cell)(((uint16_t)(idx) << 2) | TAG_REF))
#define MKSTR(idx)     ((Cell)(((uint16_t)(idx) << 2) | TAG_STR))
#define MKFUN(a, ar)   ((Cell)(((uint16_t)(((uint8_t)(ar) << 8) | (uint8_t)(a)) << 2) | TAG_ATM))
#define MKATM(a)       MKFUN((a), 0)
#define MKINT(n)       ((Cell)(((uint16_t)(int16_t)(n) << 2) | TAG_INT))

/* --- Field access --- */
#define CELL_ATOM(c)   ((uint8_t)(VAL(c) & 0xFFu))
#define CELL_ARITY(c)  ((uint8_t)(VAL(c) >> 8))
#define CELL_INT(c)    ((int16_t)(c) >> 2)

/* --- Fixed memory regions --- */
#define TRAIL_BASE     ((uint16_t *)0x5000u)
#define TRAIL_WORDS    1024u

#define CP_BASE        ((uint8_t *)0x5800u)
#define CP_BYTES       2048u

#define EP_BASE        ((uint8_t *)0x6000u)
#define EP_BYTES       2048u

#define HEAP_BASE      ((Cell *)0x6800u)
#define HEAP_WORDS     5120u           /* 10 KB / 2 bytes */

/* Atom region split:
 *   $9000-$90FF  index table  (128 x uint16_t = 256 bytes)
 *   $9100-$93FF  string pool  (768 bytes)
 */
#define ATOM_IDX_BASE  ((uint16_t *)0x9000u)
#define ATOM_IDX_MAX   128u
#define ATOM_STR_BASE  ((uint8_t  *)0x9100u)
#define ATOM_STR_BYTES 768u

#define INBUF_BASE     ((char *)0x9400u)
#define INBUF_BYTES    512u

/* --- Interpreter registers (defined in heap.c / trail.c) --- */
extern uint16_t hp;
extern uint16_t tp;
extern uint16_t hb;

#endif /* MEMORY_H */
