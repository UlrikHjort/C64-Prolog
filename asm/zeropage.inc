;
; zeropage.inc -- Zero-page scratch locations for hand-written assembly.
;
; cc65 runtime reserves $02-$1B (26 bytes) for its own ZP variables.
; Our scratch starts at $1C.
;
; Each assembly routine that uses these bytes saves/restores only what
; it clobbers.  Since cc65 does NOT preserve ZP temps across calls,
; there is no conflict with the C compiler's use of ptr1/ptr2/tmp1 etc.
;
; Layout:
;   $1C  deref_slo  - saved cell lo  (used by deref.s)
;   $1D  deref_shi  - saved cell hi  (used by deref.s)
;   $1E  deref_nlo  - new cell lo    (used by deref.s)
;   $1F  deref_nhi  - new cell hi    (used by deref.s)
;
; Future phases will add interpreter registers (HP, TP, HB, X0-X7)
; starting at $20 once they move from C globals to ZP for speed.
;

deref_slo = $1C
deref_shi = $1D
deref_nlo = $1E
deref_nhi = $1F
