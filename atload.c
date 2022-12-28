#include <std.h>
#include <stdio.h>
#include <qdos.h>
#include "basepage.h"

struct ex_header
	{
	WORD ex_magic;
	LONG ex_textlen;
	LONG ex_datalen;
	LONG ex_bsslen;
	LONG ex_symbsize;
	LONG ex_res1, ex_res2;
	WORD ex_res3;
	};

/********************************************************
 * Routine to load and relocate the ATARI program in	*
 * memory. Gives error messages but doesn't close the	*
 * file or release memory.								*
 ********************************************************/

COUNT atari_load( name, basepage, memgot)
TEXT *name; /* Program name */
struct basepage *basepage; /* Basepage of program */
LONG memgot; /* Amount of memory to put program in */
{
	FAST UBYTE *pstart = (UBYTE *)basepage + P_PROGSTART;
	FAST LONG fixup, amread;
	UBYTE nxtfix;
	FAST UBYTE *pos;
	FAST UWORD *trapp;
	struct ex_header header;

	if((amread =myfs_load( name, pstart, memgot - 256, 1))<0)
		{
		fprintf( stderr, "Failed to load %s\n", name);
		return (COUNT)amread;
		}
	/* Copy the header into the structure */
	movmem( pstart, &header, sizeof(struct ex_header));
	/* Check it really is a header */
	if( header.ex_magic != 0x601a)
		{
		fprintf( stderr, "Not a real ATARI program - header magic number missing\n");
		return (COUNT)ERR_BP;
		}
	/* Check the required size will fit in the memory available */
	if( header.ex_textlen + header.ex_datalen + header.ex_bsslen >= memgot -256)
		{
		fprintf(stderr, "Not enough memory to run program\n");
		return (COUNT)ERR_OM;
		}
	/* Move down the text and data segments in memory to overwrite the header */
	movmem( pstart + sizeof( struct ex_header ), pstart, 
			(amread - sizeof(struct ex_header )));
	/* Point to start of relocation information */
	pos = pstart + header.ex_textlen + header.ex_datalen + header.ex_symbsize;
	/* Relocate atari program */
	fixup = *(LONG *)pos;
	pos += 4;
	if( fixup )
		{/* There is relocation info */
		do
			{
			*(LONG *)(pstart + fixup) += (LONG)pstart;
			do
				{
				nxtfix = *pos++;
				if( nxtfix == 1)
					fixup += 0xFE;
				}
			while( nxtfix == 1);
			fixup += nxtfix;
			}
		while (nxtfix);
		}
	/* Clear the memory used by symbol table and relocation info (is part of BSS)*/
	setmem( pstart + header.ex_textlen + header.ex_datalen,
		amread - header.ex_textlen - header.ex_datalen,
		0);
	/* Now go through and change all the trap calls */
	for(trapp = (UWORD *)pstart;
		(LONG)trapp - (LONG)pstart < header.ex_textlen; trapp++)
			if( *trapp == 0x4e41 ) /* GEMDOS call */
				*trapp = 0x4e45; /* Change trap #1's to trap #5's */
			else if (*trapp == 0x4e42) /* GEM call */
				*trapp = 0x4e46; /* Change trap #2's to trap #6's */
	/* Now set up the basepage */
	basepage->p_lowtpa = (LONG)basepage;
	basepage->p_hitpa = (LONG)((BYTE *)basepage + memgot);
	basepage->p_tbase = (LONG)pstart;
	basepage->p_tlen = header.ex_textlen;
	basepage->p_dbase = (LONG)(pstart + header.ex_textlen);
	basepage->p_dlen = header.ex_datalen;
	basepage->p_bbase = basepage->p_dbase + header.ex_datalen;
	basepage->p_blen = header.ex_bsslen;
	basepage->p_dta = (LONG)((BYTE *)basepage + P_CMDLIN);
	basepage->p_parent = NULL;
	*(LONG*)((BYTE*)basepage + P_ENV) = NULL;
	/* Ready to execute task */
	return 0;
}
