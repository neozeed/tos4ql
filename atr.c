#include <std.h>
#include <stdio.h>
#include <qdos.h>
#include <ctype.h>
#include "basepage.h"

extern long emchan; /* Error message channel */
IMPORT TEXT _pdir_[];
/* IMPORT char *fgetchid(), *mt_alchp(); */
IMPORT LONG mt_free(), gemcall(), rteins(), adderr();
LONG _MNEED = 2L*1024L; /* 2K of heap */
LONG _STACK = 2L*1024L; /* 2K of stack */
long preponly = 0; /* Preprocess only flag */
/* Special debug stuff */
#if DEBUG
TEXT testbuf[20];
IMPORT FILE *serdb;
#endif

struct QLVECTABLE vec_tab = 
			   {&adderr, /* Address error */
				&rteins, /* Illegal instruction */
				&rteins, /* Divide by zero */
				&rteins, /* CHK instruction */
				&rteins, /* TRAPV instruction */
				&rteins, /* privilege violation */
				&rteins, /* Trace exception */
				&rteins, /* Interrupt level 7 */
				&gemcall, /* Trap #5 */
				&rteins, /* &badcall, All other traps 6 */
				&rteins, /* &badcall,  7 */
				&rteins, /* &badcall,  8 */
				&rteins, /* &badcall,  9 */
				&rteins, /* &badcall,  10 */
				&rteins, /* &badcall,  11 */
				&rteins, /* &badcall,  12 */
				&rteins, /* &bios, &badcall,  13 */
				&rteins, /* &xbios, &badcall,  14 */
				&rteins /* &badcall  15 */
				};

/********************************************************
 * Area to save registers and stack frame when doing the*
 * GEMDOS emulation in user mode.						*
 ********************************************************/

struct frame
	{
	LONG fr_d1, fr_d2, fr_d3, fr_d4, fr_d5, fr_d6, fr_d7;
	LONG fr_a0, fr_a1, fr_a2, fr_a3, fr_a4, fr_a5;
	LONG fr_ret;
	WORD fr_sr;
	} supsave = { 0 };

struct frame *regsave = &supsave + 1;

LONG aye6; /* Place to store a6 before using it to point at regsave */

/********************************************************
 * Routine to print an error and exit.					*
 ********************************************************/

VOID error( err, str)
int err;
TEXT *str;
{
	fprintf( stderr, str);
	exit(err);
}

/********************************************************
 * Routine to parse a QL filename and replace all 		*
 * possible directory nodes with \.						*
 ********************************************************/

void und_mod( name )
FAST char *name;
{
	if(!name)
		return;
	for( ; *name; name++)
		if( *name == '_' )
			{
			*name = '\\';
			if( name[1] == '_' )
				name++; /* Leave one of two _'s in a row */
			}
}
				
/********************************************************
 * Program to run Lattice C for the ATARI ST on the QL.	*
 * Passes command line parameters but redirects I/P and	*
 * O/P before jumping to the program.					*
 ********************************************************/

int main( argc, argv)
register int argc;
register char *argv[];
{
	char *pstart; /* Start address of program */
	register char *basepage, *cl_pointer, *spare;
	int stk_supplied = 0, heap_supplied = 0;
	long freemem, memgot;
	register short i, len;
	register char *p;
	int outopt = FALSE, is_pass1;

	if( argc == 1)
		error(ERR_BP, "No arguements\n");
	/* Initialise the ATARI programs handles */
	init_handles();
	/* Grab 40 K of low memory for screen space and slave blocks */
	if(!(spare = mt_alchp( (40L*1024L), &memgot, -1L)))
		error( ERR_OM, "Failed to allocate system memory\n");
	/* Now we have the program file open we can grab all available memory
		then load the program into it. */
	freemem = mt_free();
	/* Now try and allocate it all */
	/* Fudge factor on bloody thor 16 */
	if(!(basepage = mt_alchp( freemem - (2L*1024L), &memgot, -1L )))
		error( ERR_OM, "Failed to allocate memory for program\n");
	memgot -= 16; /* Always returns 16 bytes more than asked for */
	mt_rechp( spare ); /* Free low memory area */

	/* Now try and load the program into the space */
	if( i = atari_load( argv[1], basepage, memgot )) {
		/* atari_load does its own error messages */
		mt_rechp( basepage );
		error( i, "Failed to read atari program\n");
	}

	/* Now place the command line into the basepage */
	cl_pointer = basepage + P_CMDLIN + 1;
 	*cl_pointer = '\0';
	
	/* Add extra arguements to lattice C for my benefit */
	if( (is_pass1 = !strfnd( "P1", argv[1], FALSE)) || 
						!strfnd( "P2", argv[1], FALSE)) {
		/* We are running lattice C - ensure we have enough stack */
		/* Modify the last arguement to remove '_''s */
		und_mod( argv[argc-1] );
		for( i = 2; i < argc; i++) {
			/* Deal with any memory options */
			if((*argv[i] == '=') || (*argv[i] == '%')) {
					if(*argv[i] == '=')
						stk_supplied = 1;
					else
						heap_supplied = 1;
					strcat( cl_pointer, argv[i]);
					argv[i] = NULL;
			}
			if(*argv[i] == '-' && (toupper(argv[i][1]) == 'O'))
				outopt = 1;
		}
		if( !stk_supplied ) /* No stack size specified */
			strcat( cl_pointer, "=4096 ");
		if( !heap_supplied ) /* No heap size specified */
			strcat( cl_pointer, "%50000 ");
	}

	/* Process the rest of the options */
	for( i = 2; i < argc && (*argv[i] == '-'); i++) {
		if(!argv[i])
			continue;
		p = &argv[i][1];
		if((toupper(*p) == 'Q')||(toupper(*p) == 'O')) {
			*p = 'o'; /* Ensure q option can be used in pass 1 */
			len = strlen( argv[i] );
			if( argv[i][len - 2] == '_' )
				argv[i][len - 2] = '.'; /* Modify extension */
			/* Ensure output options are in GEMDOS directory format */
			und_mod( argv[i] );
		}
		else if(( toupper(*p) == 'P') && is_pass1)
			preponly = 1;
		strcat( cl_pointer, argv[i]);
		strcat( cl_pointer, " ");
	}

	if( is_pass1 ) {
		/* Program being run is Lattice C first phase */
		strcat( cl_pointer, "-n ");
		strcat( cl_pointer, "-i");
		strcat( cl_pointer, _pdir_);
		strcat( cl_pointer, "include_ ");
	} else {
		/* Must be pass 2 */
		if( !outopt ) {
			/* No output option - add a -o<input name>.o */
			strcat( cl_pointer, "-o");
			/* Get the last component of the name */
			spare = &argv[argc-1][ strlen( argv[argc-1] ) - 1];
			for( ; *spare != '\\' && spare > argv[argc-1]; spare--)
				;
			if( *spare == '\\')
				spare++;
			strcat( cl_pointer, spare);
			strcat( cl_pointer, ".o ");
		}
	}

	for( ; i < argc; i++) {
		strcat( cl_pointer, argv[i]);
		strcat( cl_pointer, " ");
	}

	/* Set the length of the command line */
	*(basepage + P_CMDLIN) = strlen( cl_pointer );
	mt_trapv( -1L, &vec_tab);

#if DEBUG
	/* Pause the program here to allow QMON to grab it */
	fputs( "Ready for debug\n", serdb);
	fgets( testbuf, 10, serdb);
#endif

	if( (emchan = fgetchid( stdout )) != fgetchid( stderr ))
		fclose( stderr ); /* Don't need this for Lattice */

	/* Now start the program - with basepage as parameter */
	pstart = basepage + P_PROGSTART;
	atstart( pstart, basepage, memgot);
	mt_rechp( basepage );
}
