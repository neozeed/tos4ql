#include <std.h>
#include <stdio.h>
#include <qdos.h>
#include <ctype.h>

#define OPTBUF_LEN 512
#define PAUSE_OPTION 'h'

long _MNEED = 4L*1024L;
long _STACK = 2L*1024L;
char _PROG_NAME[] = "PDQC compiler driver V1R0";

char flistbuf[4L*1024L]; /* Buffer for list of names */
/* Possible options for passes one and two */
char *opts[] = { "bcdDeilnqpux%=", "for%=" };
extern int is_noclose( int );

/******************************************************
 * Program to run passes one and two of the Lattice C *
 * compiler, given options, a file name or list of    *
 * files.                                             *
 ******************************************************/

/******************************************************
 * Routine to decide if an option is allowable.       *
 ******************************************************/

int is_opt( c )
FAST char c;
{
	FAST int is1 = 0, is2 = 0;
    FAST char *p;

    for( p = opts[0]; *p; p++)
        if( c == *p )
			{
            is1 = 1;
			break;
			}
    for( p = opts[1]; *p; p++)
        if( c == *p )
			{
            is2 = 2;
			break;
			}
    return (is1 | is2); /* Returns 0 if none, 1 if is1, 2 if is2, and 3 if is both */
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

/*******************************************************
 * Routine to get a list of files from a wildcard.     *
 *******************************************************/

int get_flist( s1, flist, size)
register char *s1, *flist;
int size;
{
    if(strpbrk( s1, "?*[")) /* It's a wildcard, get the list of data files */
    	return getfnl( s1, flist, size, QDR_DATA);

    /* Not a wildcard */
    strcpy( flist, s1);
    return 1; /* Only one file */
}

/**********************************************
 * Routine to display an error message and    *
 * terminate.                                 *
 **********************************************/

void error( err, str)
int err;
char *str;
{
    fputs( str, stderr);
    exit( err );
}

/********************************************************
 * Routine to check if this compilation set should be	*
 * aborted.												*
 ********************************************************/

int no_continue()
{
	long chan = fgetchid( stdin );
	char c;
	int err;

	fputs("Compilation failed\nPress <Enter> to exit, any key to continue\n",stderr);
	sd_cure( chan, -1); /* Ensure a cursor is enabled */
	if( err = io_fbyte( chan, -1, &c))
		exit( err );
	return ( c == '\n');
}

/********************************************************
 * Main routine to do the compilations, managing all	*
 * arguements.											*
 ********************************************************/

int main( argc, argv)
int argc;
FAST char *argv[];
{
    char optbuf1[OPTBUF_LEN], optbuf2[OPTBUF_LEN], fname[42], qname[42];
	char sname[42], pname[42], p2name[42], pdqcname[42];
	FAST char *spare;
	char *p;
    FAST char *argvsf, *p1 = fname;
    FAST int i, sf, len;
    int prep_only = FALSE, pause_opt = FALSE;
    int ret = 0, n;

    optbuf1[0] = optbuf2[0] = qname[0] = pname[0] = p2name[0] = pdqcname[0] = '\0';
    /* Collect all the options for both passes */
    for( sf = 1; sf < argc && *argv[sf] == '-'; sf++)
            { /* Stop after getting options */
			argvsf = argv[sf];

            if( argvsf[1] == PAUSE_OPTION )
                {/* Pause between files if error */
                pause_opt = TRUE;
                continue;
                }

			if( argvsf[1] == 'g' )
				{ /* Set directory to read pdqc and passes of compiler from */
				strcpy( pname, &argvsf[2]);
				strcpy( p2name, pname);
				strcpy( pdqcname, pname);
				continue;
				}

            switch( is_opt( argvsf[1] ))
				{/* Returns 0 1 2 or 3 */
				case 1: /* Option for pass 1 */
					p = optbuf1;
					break;
				case 2: /* Option for pass 2 */
					p = optbuf2;
					break;
				case 3: /* Must be either -% or -= */
	                strcat( optbuf1, &argvsf[1]); /* Remove the '-' */
	                strcat( optbuf2, &argvsf[1]); /* Remove the '-' */
					continue; /* Get the next option */
				default:
					fputs( argvsf, stderr);
	                error( 3, " :unrecognised option for pass 1 or 2\n");
				}

            /* Recognised option, put it into option string */
            if( strlen(p) + strlen(argvsf)+2 >= OPTBUF_LEN )
                error( 4, "Too many options\n");

			/* Check if we should take note (-p or -q) */
            if( toupper( argvsf[1] ) == 'P')
                prep_only = TRUE;
			else if( toupper( argvsf[1] ) == 'Q')
				strcpy( qname, &argvsf[2]);

            strcat( p, argvsf); /* Copy all the option */
            strcat( p, " "); /* Add a space at the end of the option */
            }

    if( sf >= argc )
        error( 2, "No filenames to compile\n");

	/* Clear screen before we start if the channel wasn't passed to us */
	if(isatty(fileno(stdout)) && !is_noclose(fileno(stdout)))
		sd_clear( fgetchid( stdout), -1);

	/* Close stdin if not needed */
	if(!isatty(fileno(stdout)) && !isatty(fileno(stderr)))
		{
		fclose(stdin);
		pause_opt = FALSE;
		}

	/* Set up the place to load the passes from */ 
	strcat( pname, "p1");
	strcat( p2name, "p2");
	strcat( pdqcname, "pdqc");

    for( ; sf < argc; sf++)
        { /* All other arguements must not be files to compile */
        n = get_flist( argv[sf], flistbuf, sizeof(flistbuf));
        for( i = 0, p = flistbuf; i < n; i++, p += strlen(p)+1)
            {
			strcpy( p1, p); /* Use local copy of name */
            len = strlen(p1);
            if( p1[len-2] == '_' && toupper( p1[len-1]) == 'C')
                p1[len-2] = '\0'; /* Remove any _C suffix */

			/* Tell stdout what we are compiling */
			fputs(  "Compiling :", stdout);
			puts( p1 );
			fflush( stdout );

            /* Call the compiler pass 1 */
            if( execl( pdqcname, -1, "pdqc", pname, optbuf1, p1, NULL))
                {
                if( pause_opt && no_continue())
                    exit( 2 );
                ret = 1;
				continue;
                }

			/* Change filename if -q option in pass 1 */
            if(!prep_only)
				{
				/* Now prepare for pass 2 */
				und_mod( p1 ); /* Get spare pointer to point at last component
								  of input name to pass 1. Eg. flp1_cprog_test,
								  spare should point to test. If a -q option was
								  used this should be prepended to the input 
								  filename.
							   */
				spare = &p1[strlen(p1) - 1];
				for( ; *spare != '\\' && spare > p1; spare--)
					;
				if( *spare == '\\')
					spare++;

				if( *qname )
					{/* Strange output used in pass 1, ensure input
						matches in pass 2 */
					if( qname[strlen(qname)-1] == '_')
						{ /* Add last component of real file name onto -q
							directory name */
						strcpy( sname, qname);
						strcat( sname, spare);
						}
					else
						{
						strcpy( sname, qname); /* Just use qname as input */
						/* Remove any trailing _q */
						len = strlen( sname );
						if( sname[len-2] == '_' && (toupper(sname[len-1])=='Q'))
							sname[len-2] = '\0';
						}
					}
				else /* No qname, just use end of filename */
					strcpy( sname, p1 /* spare */);

				if(execl( pdqcname, -1, "pdqc", p2name, optbuf2, sname, NULL))
                	{
	                if( pause_opt && no_continue())
    	                exit(1);
        	        ret = 1;
            	    }
				}
            }
        }
    return ret;
}
