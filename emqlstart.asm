        SECTION TEXT
;
; Startup code for Lattice C on the QL
;
; (c) Metacomco 1985 - Modified for QL 1987 by Jeremy Allison.
;
SPACE   EQU     $20                     ; space character
TAB     EQU     $09                     ; tab character
NEWLINE EQU     $0a                     ; newline
SQUOTE	EQU		$27						; Single quote
DQUOTE	EQU		$22						; Double quote
;
ERR_BP  EQU     -15                     ; bad parameter
;
NAM_SIZ EQU     32                      ; max size of name
MIN_MEM EQU     $0800                   ; 2k default memory allocation
MIN_STK EQU     $0400                   ; 1k minimum stack
SYS_MEM EQU     $8000                   ; 16k left for QDOS
;
;
        XREF    _MAIN
        XREF    _STACK
        XREF    _MNEED
        XREF    RBRK
		XREF	_EXIT
;
;
;       Enter here with address of base page on stack above return address.
;
;
		XDEF	C
		XDEF	_DPNAME
C		bra.s	START					; Jump past job header
;
; Job header
;
		DC.L	0
		DC.W	$4AFB
;
; Default program name plus space for expansion
;
_DPNAME
		DC.W	26
		DC.B	'PDQC compiler module V1R01'

RELLEN	DC.L	BSS_START - C			; Start of relocation info
;
START
		sub.l	d2,d2					; Get a long of zero
		lea.l	C(pc),a0				; Start of job in memory
		move.l	a0,d1					; Save it
		add.l	RELLEN(pc),a0			; Start of relocatin info
		move.l	(a0)+,a1				; Get first long word of relocation info
		beq.s	AFTEREL					; No relocation info
		move.l	d2,-4(a0)				; Clear first word of BSS
		add.l	d1,0(a1,d1.l)			; Start relocation
LOOP_IT
		move.b	(a0)+,d0				; Get next relocation byte
		beq.s	AFTEREL					; End of relocation stream
		move.b	d2,-1(a0)				; Clear it as part of BSS
		cmp.b	#1,d0					; If equal to 1 add 254 to next long to reloc
		bne.s	FIX_UP					; No, just fix up word at this location
		add.l	#254,a1					; Add 254
		bra.s	LOOP_IT					; Get next byte
FIX_UP
		add.l	d0,a1					; Add amount to relocation location
		add.l	d1,0(a1,d1.l)			; Relocate
		bra.s	LOOP_IT					; Get next byte
AFTEREL
		cmp.l	a0,a7					; Reached stack pointer yet ?
		beq.s	DO_CHAN					; Yes
		move.b	d2,(a0)+				; Clear BSS byte
		bra.s	AFTEREL
;
; Deal with any channels open for the job
;
DO_CHAN
		clr.l	d0
		move.l	a7,a4				; Don't move stack pointer
		move.l	a4,_SP				; Save the stack pointer
		move.w	(a4)+,d0			; Get the number of channels open
		asl.l	#2,d0				; Multiply no. of channels by channel len 4
		add.l	d0,a4				; Move pointer past stacked channels
;
; Now deal with the parameter line (if any )
;
		clr.l	d0
		move.l	a4,a0				; Point a0 at length word
		move.l	a0,a1				; Save place to put com string
		move.w	(a0)+,d0			; Length of command line
		move.l	d0,d2				; Save actual length
COMLOOP
		dbra	d0,MOVCOM			; Move command line down
		bra.s	AFTRCOM				; End of command line
MOVCOM
		move.b	(a0)+,(a1)+
		bra.s	COMLOOP
AFTRCOM
		move.b	#0,(a1)				; Terminate command line properly
;
;
		move.l	a4,a0				; Restore start of command line
		dbra	d2,PARSCOM			; See if there was a command line
		bra		ENDCOM
;
; Parse command line for special characters
;
PARSCOM
		bra.s	ENDLOOP				; Check for end of line
SCAN
		bsr		WHILESPACE			; Go past any white space
		CMPI.B  #'<',(A0)			; start of input file name?
		bne.s	NT1					; No
        bsr     INP_NAM				; Yes deal with it
NT1
        CMPI.B  #'>',(A0)			; start of output file name?
		bne.s	NT4
        bsr     OUT_NAM
NT4
		cmp.b	#SQUOTE,(a0)			; Single quote
		bne.s	NT5
		bsr		PAST_QUOTE
NT5
		cmp.b	#DQUOTE,(a0)			; Double quote
		bne.s	NT6
		bsr		PAST_QUOTE
NT6
        bsr     WHILEWORD			; Get past a non white space
ENDLOOP
		cmp.b	#0,(a0)
		bne.s	SCAN
;
; allocate space for memory allocation and stack
;
; Calculate actual free space
;
ENDCOM	moveq	#6,d0
		trap	#1						; QDOS call to find maximum free space
		sub.l	_STACK,d1				; Returned in d1
		tst.l	_MNEED		; See what memory was asked for
		bmi.s	NASK		; Negative amount - subtract it from total memory
		bne.s	PASK		; Positive amount - this is amount we will have
		sub.l	#SYS_MEM,d1	; Zero asked for - memory is TOTAL - SYS_MEM
		bra.s	MEM2
;
NASK	add.l	_MNEED,d1	; _MNEED is negative here - so add it 
		bra.s	MEM2
;
PASK	move.l	_MNEED,d1
;
MEM2:
        cmp.l   #MIN_MEM,d1             ; make sure we get at least the minimum
        bgt.s   MEM1
        move.l  #MIN_MEM,d1             ; set minimum memory size
MEM1:
		move.l	d1,_MSIZE
		add.l	_STACK,d1
		addq.l	#1,d1					; Make code space even
		lsr.l	#1,d1
		add.l	d1,d1					; d1 is now code space required
		move.l	d1,d7					; Save code space needed in d7
		moveq	#0,d0
		trap	#1						; Get this job's id into d1.l
		move.l  a0,_SYS_VAR				; Save address of system variables
		move.l  d2,_DOS					; Save QDOS version number
		move.l	d7,d2					; Restore data space
		move.l	d2,d4					; Save data space
		moveq	#0,d3					; d4 is not smashed by create job trap
		move.l	d3,a1					; Set no data space in new job
		moveq	#1,d0
;
; Code space is to be used as this jobs heap
;
		trap	#1						; Create job to act as data
;
; Secondary job exists to act as heap
; This means QL code at present cannot be position independant as
; DATA,UDATA and heap+stack are not continuous
;
		tst.l	d0
		bne.s	ABORT					; Create job failed
		move.l	a0,_MBASE				; a0 is base of area allocated
		move.l	a0,_MNEXT
		lea.l	0(a0,d4.l),a7			; Set up the stack for the job
		lea.l	C,a5
		add.l	#$8000,a5				; Set a5 for relative data access
		jsr		RBRK
		move.l	a4,-(a7)				; Send parameter line to _main
		jsr		_MAIN
		add.l	#4,a7
		moveq.l	#0,d0
ABORT:
		move.l	d0,-(a7)
ABLOOP: jsr		_EXIT
		bra.s	ABLOOP

        XDEF    XCEXIT
XCEXIT:
        MOVE.L  4(A7),D0                ; save return status
        BRA.S   ABORT
;
; END OF STARTUP CODE - SUPPORT SUBROUTINES FOLLOW
;
;
; Subroutine to go past any white space ( moving a0).
;
WHILESPACE
		cmp.b	#SPACE,(a0)
		beq.s	SLOOP
		cmp.b	#TAB,(a0)
		beq.s	SLOOP
		rts
SLOOP	add.l	#1,a0				; Move a0 on
		bra.s	WHILESPACE

;
; Subroutine to go past any non while space
;
WHILEWORD
		cmp.b	#SPACE,(a0)
		beq.s	WOUT
		cmp.b	#TAB,(a0)
		beq.s	WOUT
		cmp.b	#0,(a0)
		beq.s	WOUT
		add.l	#1,a0
		bra.s	WHILEWORD
WOUT
		rts
;
; Subroutine to go from one quote (single or double) to another
;
PAST_QUOTE
		move.b	(a0)+,d6		; Get either single or double quote
PLOOP
		cmp.b	(a0),d6			; Have we found another ?
		beq.s	POUT			; yes - return after incrementing a0
		cmp.b	#0,(a0)			; are we at end of text ?
		beq.s	DPOUT			; yes - return - do not increment a0
		addq.l	#1,a0
		bra.s	PLOOP
POUT	addq.l	#1,a0
DPOUT	rts
;
;	Subroutines to put the names of the input and output files into static
;
OUT_NAM LEA.L    _ONAME,A1
        BRA     INP01
;
INP_NAM LEA     _INAME,A1
INP01   MOVEQ   #NAM_SIZ-1,D1           ; set size of name field
        ADDQ.L  #1,A0                   ; get past "<" or ">"
INP02   
        CMPI.B  #SPACE,(A0)             ; end of field?
        BEQ     INOUT
        CMPI.B  #TAB,(A0)
        BEQ     INOUT
        CMPI.B  #0,(A0)
        BEQ     INOUT
        MOVE.B  (A0)+,(A1)+
        DBF     D1,INP02
INOUT
		rts
;
		CNOP	0,4		; Align to long word boundary for
						; relocation items.
LAST:
;
	SECTION DATA

        XDEF    _DOS,_SP,_INAME,_ONAME,_ENAME
        XDEF    _MBASE,_MNEXT,_MSIZE,_TSIZE,_OSERR,_SYS_VAR,ERRNO
;
DATASTART:
_DOS    DC.L    0                       ; DOS version number
_SP		DC.L	0
_INAME  DC.B    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
; input file name
_ONAME  DC.B    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
_ENAME  DC.B    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
; output file name
_MBASE  DC.L    0                       ; base of memory pool
_MNEXT  DC.L    0                       ; next available memory location
_MSIZE  DC.L    0                       ; size of memory pool
_TSIZE  DC.L    0                       ; total size?
_SYS_VAR DC.L	0
_OSERR  DC.L    0
ERRNO DC.L		0
;
	SECTION UDATA
	CNOP 0,2
BSS_START:
	end;
