#include <std.h>
#include <ctype.h>

/************************************************
 * Routine to open all the necessary channels   *
 * before starting the users C program.         *
 ************************************************/

int _parse_args( cl, argv_ptr )
register char *cl;   /* Command line */
register char ***argv_ptr; /* Address of argv pointer */
{
	extern char _DPNAME[], *_MNEXT;
	extern long _MSIZE;
	register char sc;
	register int wasesc = FALSE,spec = FALSE;
	register int my_argc;
	char **my_argv = (char **)_MNEXT; /* Base of heap */

	/* First set up argv[0] */
	my_argv[0] = (_DPNAME + 2); /* Set it to point after QDOS length field */
	my_argv[0][*(short int *)_DPNAME] = '\0'; /* Terminate C string correctly */

    my_argc = 1;
	for( ;1; )
        { /* Parse the command line */
        while( *cl && isspace( *cl))
            cl++; /* Get past any white space */
        while( *cl == '<' || *cl == '>' || ( *cl == '2' && cl[1] == '>'))
            { /* Don't use arguements already used */
            while( *cl && !isspace(*cl))
                cl++;
            while(*cl && isspace(*cl))
                cl++; /* Get past space after unused arguement */
            }
        if( *cl == '\\')
            cl++, wasesc = TRUE; /* Next arguement is escaped */
        if(!*cl)
            break; /* End of line reached */
		if( !wasesc && (*cl == 0x27 || *cl == '"'))
			spec = TRUE, sc = *cl++; /* Arguement is not to be split */
        if(!*cl)
            break; /* End of line reached */
        my_argv[my_argc++] = cl;
		if( spec )
			{ /* Parse until terminating character ( either ' or " ) reached */
			while( *cl && *cl != sc)
				cl++;
			}
		else
			{
	        while( *cl && !isspace(*cl))
    	        cl++; /* Go past the word */
			}
        if(!*cl)
            break;
        *cl++ = '\0'; /* Put correct terminator on word */
		spec = wasesc = FALSE;
        }
	/* Set the last arguement as NULL */
	my_argv[my_argc] = NULL;

	/* Now set up the heap to show what space has been taken */
	*argv_ptr = my_argv; /* Start of args */

	_MNEXT += ((my_argc + 1) << 2);
	_MSIZE -= ((my_argc + 1) << 2);

	return my_argc;
}
