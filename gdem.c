/********************************************************
 * Routine to emulate GEMDOS calls on a QL by converting*
 * all trap #1's in the code to trap #5 and then calling*
 * this routine in user mode.							*
 ********************************************************/

#include <std.h>
#include <stdio.h>
#include <qdos.h>
#include "gemhdr.h"

/* Definition for lattice C */
#define LATC 1

#ifdef LATC
extern long preponly;
#endif

#define NO_HANDLES 40
#define ALLOC_BEGIN 6
#define NOT_OPEN 0x00FFL
/* GEMDOS handle definitions */
#define AT_STDIN 0
#define AT_STDOUT 1
#define AT_AUX 2
#define AT_PRN 3
/* Standard character handle definitions */
#define CON_PORT 0
#define SER_PORT 1
#define PRT_PORT 2

IMPORT LONG _oserr;
IMPORT LONG getchid(), fgetchid();
IMPORT BOOL iscon();
IMPORT BYTE *mt_alchp(), *mt_free();
BOOL usepar = FALSE; /* If TRUE a par_ device is available */
HANDLE g_fds[NO_HANDLES];
HANDLE g_stdevs[3] = { NOT_OPEN, NOT_OPEN, NOT_OPEN };

#if DEBUG
FILE *serdb = stderr;
#endif

/* Table to translate QL to ATARI error codes */
LONG errarray[] = { ERROR,
					EBADRQ,
					ENSMEM,
					E_SEEK,
					EREADF,
					ENHNDL,
					EFILNF,
					EACCDN,
					EACCDN,
					0L,
					EWRITF,
					EPTHNF,
					E_CRC,
					EBADSF,
					EINVFN,
					ERROR,
					ERROR,
					ERROR,
					ERROR,
					EWRPRO,
					ERROR};
#if 0
/* Table to translate ATARI drive names to QL names */

TEXT *qlname_table[] = { "flp1_", /* A:\ */
						 "flp2_", /* B:\ */
						 "mdv1_", /* C:\ */
						 "mdv2_", /* D:\ */
						 "ram1_", /* E:\ */
						 "ram2_", /* F:\ */
						 "ram3_", /* G:\ */
						 "ram4_", /* H:\ */
						 "ram5_", /* I:\ */
						 "ram6_", /* J:\ */
						 "ram7_", /* K:\ */
						 "ram8_"  /* L:\ */
					   };
#endif

/********************************************************
 * Translate QL to ATARI error codes 					*
 ********************************************************/

LONG ql_to_aterr( errno )
LONG errno;
{
	return errarray[(-errno) - 1];
}

/********************************************************
 * Set up the standard handles in the handles table		*
 ********************************************************/

VOID init_handles()
{
	WORD i, isincon, isoutcon;

	/* Set up all the handle tables as NOT_OPEN */
	for( i = 0; i < NO_HANDLES; i++)
		g_fds[i] = NOT_OPEN;

#ifndef LATC
	isincon = iscon( fgetchid( stdin ), -1);
	isoutcon = iscon( fgetchid( stdout ), -1);
	/* Set up ATARI stdin */
	if( isincon )
		{/* stdin is a con_ channel - replace it */
		fclose( stdin );
		g_fds[AT_STDIN] = -1; /* Signal CON_ channel being used as ATARI stdin */
		}
	else
		{ /* stdin is not a con_ channel - preserve it */
		g_fds[AT_STDIN] = fileno( stdin );
		}
#else
	g_fds[AT_STDIN] = fileno( stdin );
#endif

	/* Set up ATARI stdout */
#ifndef LATC
	if(isoutcon )
		{/* stdout is a con_ channel - replace it */
		fclose( stdout );
		g_fds[AT_STDOUT] = -1; /* Signal CON_ channel being used as ATARI stdout */
		}
	else
		{/* stdout is not a con_ channel - preserve it */
		g_fds[AT_STDOUT] = fileno( stdout );
		}
#else
	g_fds[AT_STDOUT] = fileno( stdout );
#endif

	/* Set the ATARI AUX: port channel to point at the serial channel */
	g_fds[AT_AUX] = -2;
	/* Set the ATARI print channel to point at the prt channel */
	g_fds[AT_PRN] = -3;

#if 0
	serdb = fopen( "CON_512x128a0x128", "w");
#endif
}

/********************************************************
 * Routine to allocate a handle							*
 ********************************************************/

HANDLE alloc_handle()
{
	HANDLE at_fd;

	for( at_fd = ALLOC_BEGIN; g_fds[at_fd] != NOT_OPEN && at_fd < NO_HANDLES; 
			at_fd++)
				;
	return at_fd;
}

#if 0
/********************************************************
 * Routine to ensure a character device (con, prt, ser)	*
 * is open.												*
 ********************************************************/

HANDLE devs_open( ch_fd )
HANDLE ch_fd; /* Character file descriptor */
{
	HANDLE ql_fd; /* QL file descriptor handle */
	LONG stconchan, dummy;

	switch( ch_fd )
		{
		case CON_PORT:
#ifndef LATC
			/* A CON_ channel is not open - open one now */
			if((ql_fd = open( "CON_512x256a0x0", O_RDWR))<0)
				break;
			/* Make sure the CON_ channel is full screen and clear it */
			stconchan = getchid(ql_fd);
			sd_clear( stconchan, 0);
			/* Enable the cursor in the CON_ channel */
			sd_cure( stconchan, -1);
			break;
#endif
		case PRT_PORT:
#ifndef LATC
			/* Try and open the PRT device, then the PAR_ device, else use ser1 */
			if( isdevice( "PRT", &dummy ))
				if(( ql_fd = open( "PRT", O_WRONLY))<0)
					break;
			if( isdevice( "PAR_", &dummy))
				if((ql_fd = open( "PAR_", O_WRONLY))<0)
					break;
			else
				ql_fd = open( "ser1", O_WRONLY);
			break;
#endif
#ifndef LATC
		case SER_PORT:
			ql_fd = open( "ser2", O_RDWR);
#endif
			break;
		}
	if( ql_fd != -1) /* Was open succesfull */
		return ql_fd;

	return ql_to_aterr( _oserr );
}
#endif

/********************************************************
 * Routine to return the file descriptor this program is*
 * using to access a file, given an ATARI file handle.	*
 ********************************************************/

HANDLE get_handle( at_fd )
HANDLE at_fd; /* ATARI file handle */
{
	HANDLE ch_fd; /* A real character device handle */

	if(at_fd < -3 || at_fd > NO_HANDLES)
		return EIHNDL;
	if(at_fd >= 0)
		{/* standard or file handle asked for (character handles are negative) */
		if( g_fds[at_fd] != NOT_OPEN)
			{/* This file is open */
			if( g_fds[at_fd] >= 0) /* Non character handle */
				return g_fds[at_fd];
			at_fd = g_fds[at_fd]; /* Make at_fd point at character handles */
			}
		else
			return EIHNDL;
		}
	/* If we get to here at_fd is negative - and is a standard character handle */
	at_fd += 1;
	at_fd = -at_fd; /* Make it into offset into g_stdevs table */
#if 0
	if( g_stdevs[at_fd] == NOT_OPEN)
		{/* This character device has not been opened yet - open it */
		if((ch_fd = devs_open( at_fd ))<0)
			return ch_fd;
		g_stdevs[at_fd] = ch_fd; /* This handle now open */
		}
#endif
	return g_stdevs[at_fd];
}

/********************************************************
 * Routine to close any allocated character device		*
 * handles												*
 ********************************************************/

VOID close_devs()
{
	FAST WORD i;

	for( i = 0; i < 3; i++)
		if( g_stdevs[i] != NOT_OPEN)
			close( g_stdevs[i]);
}

/********************************************************
 * Routine to see if a name is an ATARI device.			*
 ********************************************************/
#if 0
LONG at_device( name )
TEXT *name;
{
	TEXT *t;

	for( t = name; *t; t++)
		*t = toupper(*t);
	if( !strcmp( name, "CON:"))
		return 0x0FFFFL;
	if (!strcmp( name, "AUX:"))
		return 0x0FFFEL;
	if(!strcmp( name, "PRN:"))
		return 0x0FFFDL;
	return 0L;
}
#endif
/********************************************************
 * Routine to check if an ATARI handle has any input.	*
 ********************************************************/
#if 0
LONG is_ready( at_fh )
HANDLE at_fh;
{
	HANDLE ql_fd;
	LONG ichan;
	LONG ret;
	BOOL isc;

	if((ql_fd = get_handle( at_fh ))<0)
		return (LONG)ql_fd;
	if(!(isc = iscon( ichan = getchid( ql_fd ), -1)))
		return 0xFFFFL; /* Non - con devices are always ready (assem_ttp fix) !*/
	ret = io_pend( ichan, 0);
	return (ret ? 0 : 0xFFFFL);
}
#endif
/********************************************************
 * Read a character from the specified QDOS channel.	*
 ********************************************************/

LONG rd_char( ql_fd )
HANDLE ql_fd;
{
	LONG ichan = getchid( ql_fd );
	LONG err;
	TEXT ch;

	if( err = io_fbyte( ichan, -1, &ch))
		return ql_to_aterr( err );
	return (((LONG)ch)&0xFF);
}

/********************************************************
 * BIOS character write - may be expanded to handle VT52*
 * emulator.											*
 ********************************************************/

VOID bios_wrchar( ochan, ch)
LONG ochan;
TEXT ch;
{
	switch(ch )
		{
		case 0xD: /* Carraige return */
			sd_tab( ochan, -1, 0);
			break;
		case 0xA: /* New line - ensure it's not held pending */
			io_sbyte( ochan, -1, ch);
			sd_donl( ochan, -1 );
			break;
		default:
			io_sbyte( ochan, -1, ch);
			break;
		}
#if 0 /* DEBUG */
	fprintf(serdb, "      Ch = %x |%c|\r\n", ch, ch);
#endif
}

/********************************************************
 * Write a character to the specified QDOS channel		*
 ********************************************************/

LONG wr_char( ql_fd, ch )
HANDLE ql_fd;
TEXT ch;
{
	LONG ochan = getchid( ql_fd );
	/* For special lattice O/P always use bios_wrchar */
#if 0
	if( ql_fd == g_stdevs[CON_PORT])
#endif
		bios_wrchar( ochan, ch); /* Filter out undesireables */
#if 0
	else
		io_sbyte( ochan, -1, ch);
#endif
	return 0L;
}

/********************************************************
 * Routine to convert ATARI file names to QL ones.		*
 ********************************************************/

VOID fix_name( name )
TEXT *name;
{
	TEXT *n, nbuf[60];
	register int i;

#ifdef LATC
	/* Ensure names ending with .bin become .o */
	i = strlen( name );
	if( (i > 4) && ( strfnd( ".BIN", name, 0) == i-4))
		{
		name[i-3] = 'O';
		name[i-2] = '\0';
		}
#endif
#if 0
/* Remove this to cut down on space */
	if( name[1] == ':')
		{/* Start the ATARI name with a QL device */
		strcpy( nbuf, qlname_table[*name - ((*name > 'Z') ? 'a' : 'A')]);
		/* Copy the rest of the name (missing \ if it occurs after a 'A:' ) */
		strcat( nbuf, &name[ name[2] == '\\' ? 3 : 2]);
		}
	else
#endif
		strcpy( nbuf, name);
	for( n = nbuf; *n; n++)
		if( strfnd( "..\\", n,1) == 0)
			n++; /* After n++ at end of loop, n will point at \ */
		else if( *n == '.' || *n == '\\' || *n == '/')
			*n = '_'; /* Replace all ATARI & UNIX separaters with QL ones */
#if 0
	/* Don't need this now */
	/* Now strip out all multiple '_' s */
	for( n = nbuf; *n; n++)
		while( *n == '_' && n[1] == '_')
			strcpy( n, n+1);
#endif
	strcpy( name, nbuf);
}

/********************************************************
 * Routine to write a string to the console O/P channel	*
 * taking account of any screen display changes.		*
 ********************************************************/

VOID con_write( ochan, bp, len)
BYTE *ochan; /* QL screen channel */
TEXT *bp; /* Start of text to write */
COUNT len; /* Number of bytes to write */
{
	FAST COUNT i;

	for( i = 0; i < len; i++, bp++)
		bios_wrchar( ochan, *bp); /* Filter out undesireables */
}

#if DEBUG
static char out[512];

void dowait()
{
	void dp();
	char c;

	dp("Wait for keypress\r\n");
	io_fbyte((long)getchid(0),(int)-1,&c);
}

VOID dp( str, fn, p1, p2, p3)
TEXT *str;
LONG fn, p1, p2, p3;
{
	int i;

	sprintf(out, str, fn, p1, p2, p3);
	for(i = 0; out[i]; i++)
		bios_wrchar(getchid(1), out[i]);
}

LONG dbp( param )
LONG param;
{
	int i;

	sprintf( out, "Return from gemdos with %lx\n\r", param);
	for(i = 0; out[i]; i++)
		bios_wrchar(getchid(1), out[i]);

	return param;
}
#define RETURN(x) return (dbp(x))
#else

#define RETURN(x) return (x)

#endif /* DEBUG */

/********************************************************
 * The GEMDOS emulator routine -all GEMDOS functions are*
 * handled here.										*
 ********************************************************/

LONG gemdos( param )
LONG param;
{
	WORD *p = (WORD *)&param, fnno, mode;
	HANDLE ql_fd, at_fd;
	LONG nr, ch;
	BOOL ret;
	BYTE *bp;
	TEXT name[60], *n;
	LONG ichan, ochan, num;

	fnno = *p++;

#if DEBUG /* DEBUG */
	dp( "GEMDOS fn no %x\n\r", fnno);
#endif

	switch( fnno )
		{
		case PTERM0:
			close_devs();
			exit(0);
#if 0
		case CCONIN:
			/* Get stdin */
			if((ql_fd = get_handle( AT_STDIN ))<0) {
				RETURN((LONG)ql_fd);
			}
			if((ch = rd_char( ql_fd ))<0L) {
				RETURN((LONG)ch);
			}
			/* Echo to stdout */
			if((ql_fd = get_handle( AT_STDOUT ))<0) {
				RETURN((LONG)ql_fd);
			}
			RETURN(wr_char( ql_fd, (TEXT)ch));
#endif
		case CCONOUT:
			if((ql_fd = get_handle(AT_STDOUT ))<0){
				RETURN(0L);
			}
			RETURN(wr_char( ql_fd, (TEXT)*p ));
#if 0
		case CAUXIN:
			if(( ql_fd = get_handle( AT_AUX ))<0) {
				RETURN((LONG)ql_fd);
			}
			RETURN(rd_char( ql_fd ));
		case CAUXOUT:
			if((ql_fd = get_handle( AT_AUX ))<0) {
				RETURN((LONG)ql_fd);
			}
			RETURN(wr_char( ql_fd, (TEXT)*p ));
		case CPRNOUT:
			if((ql_fd = get_handle( AT_PRN ))<0) {
				RETURN((LONG)ql_fd);
			}
			wr_char( ql_fd, (TEXT)*p );
			RETURN(-1L);
		case CRAWIO:
			if( (*p & 0xFF)==0xFF) { /* Read */
				if((ret = is_ready( AT_STDIN ))<0) {
					RETURN((LONG)ret);
				}
				if(!ret) {
					RETURN(0L); /* No input available */
				}
				if(( ql_fd = get_handle( AT_STDIN ))<0) {
					RETURN((LONG)ql_fd);
				}
				RETURN(rd_char( ql_fd ));
			} else {
				if((ql_fd = get_handle( AT_STDOUT))<0) {
					RETURN((LONG)ql_fd);
				}
				RETURN(wr_char( ql_fd, (TEXT)*p));
			}
		case CRAWCIN:
		case CNECIN:
			if(( ql_fd = get_handle( fnno == CRAWCIN ? -1 : AT_STDIN ))<0) {
				RETURN((LONG)ql_fd);
			}
			RETURN(rd_char( ql_fd ));
#endif
		case CCONWS: /* Write string to standard out */
			bp = (BYTE *)*(LONG *)p;
			for( n = (TEXT *)bp; *n; n++)
				;
			if((ql_fd = get_handle( AT_STDOUT))<0) {
				RETURN((LONG)ql_fd);
			}
			ochan = getchid( ql_fd );

#if 0 /* Special for lattice C */
			if(ochan != g_stdevs[CON_PORT])
				{
				io_sstrg( ochan, bp, (BYTE *)n - bp, -1);
				sd_donl( ochan );
				}
			else /* Do special console write */
#endif
				con_write( ochan, bp, (BYTE*)n - bp);
			RETURN(0L);
#if 0
		case CCONRS: /* Read a string LF terminated */
			bp = (BYTE *)*(LONG *)p;
			if((ql_fd = get_handle( AT_STDIN ))<0) {
				RETURN((LONG)ql_fd);
			}
			ichan = getchid( ql_fd );
			if((io_fline( ichan, -1, &bp[2], ((LONG)*bp)&0xFF, &nr))<0)
				{
				sd_curs( ichan, -1);
				sd_cure(ichan,-1);/* Disable then enable cursor to get round bug */
				}
			else /* Linefeed read correctly - don't add it */
				nr--;
			bp[1] = nr; /* Don't include LF in characters read */
			bp[2 + nr] = '\0';
			RETURN(nr);
		case CCONIS:
			RETURN(is_ready( AT_STDIN ));
/* DSETDRV missed here */
		case CCONOS:
		case CPRNOS:
			RETURN(0x0FFFF); /* stdout and prn are always ready */
		case CAUXIS:
			RETURN(is_ready( AT_AUX ));
		case CAUXOS: /* AUX output is always ready */
			RETURN(0x0FFFFL);
/* DGETDRV, FSETDTA, SUPER, TGETDATE, TSETDATE, TGETTIME, TSETTIME, FGETDTA,
   SVERSION, PTERMRES, DFREE, DCREATE, DDELETE, DSETPATH missing here */
#endif
		case FCREATE:
		case FOPEN:
			bp = (BYTE *)*(LONG *)p;
			strcpy( name, bp);
#if 0
			if( ret = at_device( name ))
				RETURN(ret); /* Asked for a character device */
#endif

			/* Change to cater for names such as A:\ */
			fix_name( name );

#ifdef DEBUG
			dp("opened name |%s|\r\n", name);
#endif

			/* Find an empty handle for this file */
			if((nr = alloc_handle()) >= NO_HANDLES) {
				RETURN((LONG)ENHNDL);
			}
			if( fnno == FCREATE)
				{
				if((g_fds[nr] = open( name, O_RDWR|O_CREAT|O_TRUNC))<0)
					{
					g_fds[nr] = NOT_OPEN;
					RETURN(ql_to_aterr( _oserr ));
					}
				}
			else /* FOPEN */
				{
				p += 2;
				switch( *p )
					{
					case 0:
						mode = O_RDONLY;
						break;
					case 1:
					case 2:
						mode = O_RDWR;
						break;
					}
				if(( g_fds[nr] = open( name, mode))<0)
					{
					g_fds[nr] = NOT_OPEN;
					RETURN(ql_to_aterr( _oserr ));
					}
				}
			RETURN( nr );
		case FCLOSE:
			if( *p < 0) {
				RETURN(0L); /* Can't close character devices */
			}
			if( g_fds[*p] >= 0) {
				/* Handle is not attatched to a character device */
				if(close( g_fds[*p] )) {
					RETURN(ql_to_aterr( _oserr ));
				}
			}
			/* Either file is closed or handle was attatched to character
				device - which cannot be shut - so just set it NOT_OPEN */
			g_fds[*p] = NOT_OPEN;
			RETURN( 0L );
		case FREAD:
			if((ql_fd = get_handle( at_fd = *p ))<0) {
				RETURN((LONG)ql_fd);
			}
			p++;
			num = *(LONG *)p;
			p += 2;
			bp = (BYTE *)*(LONG *)p;
#ifdef DEBUG
			dowait();
#endif
			nr = (LONG)read( ql_fd, bp, num);
#ifdef DEBUG
			dp( "read call %x, %x, %x - returned  %d\r\n", ql_fd, bp, num,nr);
#endif
			if( nr < 0) {
				RETURN(ql_to_aterr( _oserr ));
			}
			RETURN( nr );
		case FWRITE:
			if((ql_fd = get_handle( at_fd = *p ))<0) {
				RETURN((LONG)ql_fd);
			}
			p++;
			num = *(LONG *)p;
			p += 2;
			bp = (BYTE *)*(LONG *)p;
			/* Special for Lattice C */
			if(ql_fd == g_stdevs[CON_PORT] || at_fd == AT_STDOUT || preponly)
				{ /* Write to a CON_ port */
				con_write( getchid(ql_fd), bp, num);
				nr = num;
				}
			else
				{/* Write to anything but CON_ */
				nr = (LONG)write( ql_fd, bp, num);
				if( nr < 0) {
					RETURN(ql_to_aterr( _oserr ));
				}
				}
			RETURN( nr );
		case FDELETE:
			bp = (BYTE *)*(LONG *)p;
			strcpy( name, bp);
#if 0
			if( ret = at_device( name ))
				RETURN(0L);
#endif
			/* Change to cater for names such as A:\ */
			fix_name( name );

			if((nr = (LONG)remove( name ))<0) {
				RETURN(ql_to_aterr( _oserr ));
			}
			RETURN(0L);
		case FSEEK:
			num = *(LONG *)p;
			p += 2;
			if((ql_fd = get_handle( *p ))<0) {
				RETURN(ql_fd);
			}
			p++;
			mode = *p;
/* Special lattice debug */
#if DEBUG
			dp( "    Seek - parameters %x, %x, %x\r\n", ql_fd, num, mode);
#endif
			if((nr = (LONG)lseek( ql_fd, num, mode))<0) {
				RETURN(ql_to_aterr( _oserr ));
			}
			RETURN(nr);
/* FATTRIB missing here */
#if 0
		case FDUP:
			if( *p < 0 || *p > 5)
				RETURN((LONG)EIHNDL);
			/* Find an empty handle to duplicate into */
			if((nr = alloc_handle()) > NO_HANDLES)
				RETURN((LONG)ENHNDL);
			g_fds[nr] = g_fds[*p];
			RETURN(nr);
		case FFORCE:
			if( *p < 0 || *p > 5)
				RETURN((LONG)EIHNDL);
			at_fd = *p++;
			if( g_fds[*p] == NOT_OPEN)
				RETURN((LONG)EIHNDL);
			g_fds[at_fd] = g_fds[*p];
			RETURN(0L);
#endif
/* DGETPATH missing here */
		case MALLOC:
			nr = *(LONG *)p;
			if( nr == -1L)
				{/* Return size of largest free block */
				RETURN((LONG)mt_free());
				}
			if(!(bp = mt_alchp( nr, &ch, -1L))) {
				RETURN(ENSMEM);
			}
			if((ch -16) < nr) {
				RETURN(ENSMEM);
			}
			RETURN((LONG)bp);
		case MFREE:
			nr = *(LONG *)p;
			mt_rechp( (char *)nr );
			RETURN( 0L);
		case MSHRINK: /* Reduce common heap allowance */
			p++; /* Get past word of zero */
			bp = (BYTE *)*(LONG *)p;
			p += 2;
			nr = *(LONG *)p;
			mt_shrink( bp, nr);
			RETURN(0L);
/* PEXEC missing here */
		case PTERM:
			close_devs();
			exit(*p);
/* FSFIRST, FSNEXT, FRENAME, FDATIME missing here */
		}
	fprintf( stderr, "GEMDOS function %x not implemented\n", fnno);
#if 0 /* was debug */
	fgets( name, 20, serdb);
#endif
	RETURN(0L);
}
#if 0
/********************************************************
 * BIOS and XBIOS calls.								*
 ********************************************************/

LONG bios( param )
LONG param;
{
	WORD *p = (WORD *)&param;

	fprintf( stderr, "BIOS call function no. %d\n", *p);
	return 0L;
}

LONG xbios( param )
LONG param;
{
	WORD *p = (WORD *)&param;

	fprintf( stderr, "XBIOS call function no. %d\n", *p);
	return 0L;
}

/********************************************************
 * Function to be called on bad trap etc.				*
 ********************************************************/

VOID bad()
{
	FOREVER
		fprintf( stderr, "Bad trap call\n");
}

#endif
