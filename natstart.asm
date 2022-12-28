*
* Routine to jump to the start up of an ATARI program
* called by C as at_start( progstart, basepage, memgot)
*
	SECTION TEXT
    XREF aye6,regsave,supsave
    XREF gemdos,_exit
    XDEF atstart,gemcall,rteins,adderr

atstart
    move.l 4(a7),a0            * Program start address
    move.l 8(a7),d1            * Get basepage
    move.l d1,d0               * Save basepage
    add.l 12(a7),d1            * Add in memory obtained
    move.l (a7)+,a1            * Save old return address
    move.l d1,a7               * Set up new stack
    move.l d0,-(a7)            * Push basepage on new stack as parameter
    move.l a1,-(a7)            * Push old return address
    jmp (a0)                   * Start program
*
*
* Routine executed in supervisor mode to call GEMDOS emulator
* Saves registers in programs private save area, cleans up Supervisor stack
* then goes into user mode and calls GEMDOS. On return it restores registers
* and, enters supervisor mode, then returns to the program via RTE.
*
gemcall
    move.l a6,aye6             * Save old a6
    move.l regsave,a6          * Point new a6 at top of save area
    movem.l d1-d7/a0-a5,-(a6)  * Save all the registers
    move.w (a7)+,-(a6)         * Save status register pushed on stack
    move.l (a7)+,-(a6)         * Save return address from supervisor stack
*
* Now go to user mode and call GEMDOS
*
    and #$DFFF,SR            * Remove supervisor bit from status register
*
* We are now in user mode
*
    jsr gemdos                 * Do the GEMDOS emulation
*
* Now go to supervisor mode and restore the registers
*
    trap #0
    lea.l supsave,a6
    move.l (a6)+,-(a7)              * Restore old return address
    move.w (a6)+,-(a7)              * Set old status register on stack
    movem.l (a6)+,d1-d7/a0-a5       * Restore old registers
    move.l aye6,a6                  * Restore old a6
    rte

*
* Routine to handle bad traps etc.
*
* Address error routine
*
adderr
	move.w	(a7)+,d1		* Remove access word
	move.l	(a7)+,d1		* Remove current cycle address
	move.w	(a7)+,d1		* Rmove Instruction register
	lea.l	EADD,a1			* Get error message
	move.w	EADDLEN,d2
	bra.s	nnn
rteins
	lea.l	OERR,a1
	move.w	OERRLEN,d2
nnn
	move.w	(a7)+,d1		   * Remove old status reg
	move.l	(a7)+,d1		   * Remove old pc
*
* Write out an error message
*
	move.l	EMCHAN,a0
	clr.l	d3
	moveq	#7,d0
	trap	#3
*
* Now go to user mode and call C function
*
    and #$D2FF,SR              * Remove supervisor bit from status register
*								 Plus reset interrupt level to 2
*
* We are now in user mode
*
	move.l	#-1,d1
	move.l	d1,d3
	moveq	#5,d0
	trap	#1					* Remove ourselves

	SECTION DATA
	XDEF EMCHAN
EADDLEN
	DC.W	42
EADD 
	DC.B	'PDQC INTERNAL Out of memory - program fail'
OERRLEN
	DC.W	26
OERR
	DC.B	'PDQC General program fault'
EMCHAN
	DC.L	0
    END
