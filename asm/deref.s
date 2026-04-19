;
; deref.s -- Cell __fastcall__ deref(Cell c)
;
; Follows a REF chain on the heap until reaching either:
;   - a non-REF cell (bound or structured term), or
;   - a self-referential REF cell (unbound variable).
;
; cc65 __fastcall__ convention for a uint16_t argument:
;   Entry : A = c.lo,      X = c.hi
;   Exit  : A = result.lo, X = result.hi
;   Clobbers: Y, ptr1 (cc65 ZP scratch at $04-$05), deref_s*, deref_n*
;
; Heap layout:
;   HEAP_BASE = $6800  (Cell array, 2 bytes per cell)
;   Cell at word-index i lives at byte address $6800 + i*2.
;
; Address computation:
;   For a REF cell c, val(c) = c >> 2 (word index).
;   byte_addr = $6800 + val(c)*2 = $6800 + (c >> 1).
;   We compute (c >> 1) by a single 16-bit right-shift of the cell value,
;   then add the HEAP_BASE high byte ($68) to the high byte of the result.
;   The HEAP_BASE low byte is $00, so no addition is needed for the low byte.
;

.include "zeropage.inc"

.importzp ptr1          ; cc65 ZP scratch pointer, 2 bytes at $04-$05
                        ; ptr1 = $04 (lo), ptr1+1 = $05 (hi)
                        ; Used with (ptr1),Y indirect addressing.

HEAP_HI = $68           ; High byte of HEAP_BASE ($6800)

.export _deref

.proc _deref

loop:
    ; ---------------------------------------------------------------
    ; Test tag: low 2 bits of A (the cell's lo byte).
    ; TAG_REF = 0 means BOTH low bits are zero.
    ; ---------------------------------------------------------------
    pha                         ; save cell lo on hardware stack
    and #$03                    ; isolate tag bits
    bne not_ref                 ; tag != 0 -> not a REF, return as-is

    pla                         ; restore cell lo (we know it is a REF)

    ; ---------------------------------------------------------------
    ; Save the current cell (it may be the answer if self-referential).
    ; ---------------------------------------------------------------
    sta deref_slo               ; deref_slo = cell lo
    stx deref_shi               ; deref_shi = cell hi

    ; ---------------------------------------------------------------
    ; Compute byte address of heap[val(c)]:
    ;   byte_addr = HEAP_BASE + (c >> 1)
    ;
    ; Shift deref_shi:deref_slo right by 1 (16-bit):
    ;   step 1: lsr A  where A = cell hi -> A = hi>>1, carry = hi bit0
    ;   step 2: ror A  where A = cell lo -> A = (carry<<7)|(lo>>1)
    ; ---------------------------------------------------------------
    lda deref_shi
    lsr a                       ; A = hi>>1, carry = bit0 of hi
    sta ptr1+1                  ; ptr1+1 = offset hi (before HEAP_HI add)
    lda deref_slo
    ror a                       ; A = (carry<<7)|(lo>>1) = offset lo
    sta ptr1                    ; ptr1 = offset lo

    ; Add HEAP_BASE hi byte to ptr1+1 (lo byte is $00, no-op).
    lda ptr1+1
    clc
    adc #HEAP_HI
    sta ptr1+1                  ; ptr1+1 = final address hi

    ; ptr1:ptr1+1 is now the byte address of the cell on the heap.

    ; ---------------------------------------------------------------
    ; Read the 16-bit cell stored at (ptr1).
    ; ---------------------------------------------------------------
    ldy #0
    lda (ptr1),y                ; load lo byte
    sta deref_nlo
    ldy #1
    lda (ptr1),y                ; load hi byte
    sta deref_nhi

    ; ---------------------------------------------------------------
    ; Self-reference check: if heap[val(c)] == c, c is unbound.
    ; ---------------------------------------------------------------
    lda deref_nlo
    cmp deref_slo
    bne follow
    lda deref_nhi
    cmp deref_shi
    beq unbound                 ; both bytes equal -> self-reference

follow:
    ; Follow the pointer: loop with the cell we just read.
    lda deref_nlo
    ldx deref_nhi
    jmp loop

unbound:
    ; Return the original (unbound) REF cell.
    lda deref_slo
    ldx deref_shi
    rts

not_ref:
    ; Not a REF: return the cell unchanged.
    ; A was overwritten by 'and #$03'; restore lo from stack.
    pla                         ; A = original cell lo
    ; X still holds the original cell hi (never touched in this path).
    rts

.endproc
