#include <std.h>
#include <qdos.h>

IMPORT TEXT _dir_[]; /* Current default directory */
IMPORT TEXT _pdir_[]; /* Current program directory */
IMPORT LONG _oserr;
IMPORT BOOL isdevice();

struct ql_header
   {
	LONG d_length;      /* file length      */
	UBYTE d_access;      /* file access type   */
	UBYTE d_type;        /* file type      */
	LONG d_datalen;	 /* data length for execable files */
	LONG d_reserved; /* Unused */
	WORD d_szname;    /* size of name      */
	TEXT d_name[36];    /* name area      */
	LONG d_update;	/* Date of last update */
	LONG d_refdate;
	LONG d_backup; /* Disk data finishes here */
	};

/********************************************************
 * Routine to read an entire file ito memory. Either is	*
 * given place to load it or allocates it from QL heap	*
 ********************************************************/

COUNT myfs_load(name, bp, aloced, dirf)
TEXT *name; /* Name of file to load */
BYTE *bp; /* Either place to load file or NULL if allocation wanted */
BYTE **aloced; /* If bp == NULL then return place allocated in this ,
					else if bp != NULL then this is length already allocated */
BOOL dirf; /* If TRUE try to read from program directory first */
{
	BYTE *chan;
	BOOL alspace = FALSE;
	struct REGS in, out;
	struct ql_header hdr;
	TEXT fname[128];
	LONG dummy;

	if(!isdevice( name, &dummy ))
		{/* Not a device type - add default directory (program or data) */
		strcpy( fname, dirf ? _pdir_ : _dir_);
		strcat( fname, name );
		chan = io_open( fname, OLD_SHARE);
		if( _oserr )
			{/* Failed to open file - try other directory */
			strcpy( fname, dirf ? _dir_ : _pdir_);
			strcat( fname, name);
			chan = io_open( fname, OLD_SHARE);
			}
		}
	else
		chan = io_open( name, OLD_SHARE);
	if(_oserr) /* Can't open file */
		return (COUNT)_oserr;
	/* Now try and read the file header - to see how much space we need */
	in.D0 = 0x47;
	in.D2 = sizeof( struct ql_header );
	in.D3 = -1; /* Timeout */
	in.A0 = chan;
	in.A1 = (char *)&hdr;
	if(_oserr = (long)qdos3( &in, &out))
		{
		io_close( chan );
		return (COUNT) _oserr;
		}
	/* Now make sure we have enough space for the file (or allocate some) */
	if( bp )
		{/* Space was passed to the routine - ensure there's enough */
		if( hdr.d_length > (LONG)aloced )
			{
			io_close( chan );
			return (COUNT)ERR_OM;
			}
		}
	else
		{/* No space - allocate some */
		if(!(*aloced = (BYTE *)mt_alchp( hdr.d_length, &dummy, -1L)))
			{
			io_close( chan );
			return _oserr;
			}
		bp = *aloced;
		alspace=TRUE;/* Note that we allocated space so we can free if read error*/
		}
	/* Now do the read call */
	in.D0 = 0x48;
	in.D2 = hdr.d_length;
	in.D3 = -1;
	in.A0 = chan;
	in.A1 = bp;
	_oserr = (long)qdos3(&in, &out);
	io_close( chan );
	if(alspace && _oserr)
		{
		mt_free( bp );
		return _oserr;
		}
	return (COUNT)(out.A1 - bp);
}
