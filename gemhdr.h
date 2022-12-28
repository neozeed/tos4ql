/**
*
* This file contains macro definitions for use with the Atari specific
* functions gemdos,bios and xbios (see manual section 5.5)
*
**/

/* GEMDOS functions (trap #1) */

#define PTERM0       0x0
#define CCONIN       0x1
#define CCONOUT      0x2
#define CAUXIN		 0x3
#define CAUXOUT      0x4
#define CPRNOUT      0x5
#define CRAWIO       0x6
#define CRAWCIN      0x7
#define CNECIN       0x8
#define CCONWS       0x9
#define CCONRS		0x0A
#define CCONIS		0x0b
#define DSETDRV		0x0e
#define CCONOS		0x10
#define CPRNOS		0x11
#define CAUXIS		0x12
#define CAUXOS		0x13
#define DGETDRV		0x19
#define FSETDTA		0x1a
#define SUPER		0x20
#define TGETDATE	0x2a
#define TSETDATE	0x2b
#define TGETTIME	0x2c
#define TSETTIME	0x2d
#define FGETDTA		0x2f
#define SVERSION	0x30
#define PTERMRES	0x31
#define DFREE		0x36
#define DCREATE		0x39
#define DDELETE		0x3a
#define DSETPATH	0x3b
#define FCREATE		0x3c
#define FOPEN		0x3d
#define FCLOSE		0x3e
#define FREAD		0x3f
#define FWRITE		0x40
#define FDELETE		0x41
#define FSEEK		0x42
#define FATTRIB		0x43
#define FDUP		0x45
#define FFORCE		0x46
#define DGETPATH	0x47
#define MALLOC		0x48
#define MFREE		0x49
#define MSHRINK		0x4a   /* NOTE: Null parameter added */
#define PEXEC		0x4b
#define PTERM		0x4c
#define FSFIRST		0x4e
#define FSNEXT		0x4f
#define FRENAME		0x56
#define FDATIME		0x57

/* BIOS and GEMDOS error codes */
#define E_OK 0
#define ERROR -1
#define EDRVNR -2
#define EUNCMD -3
#define E_CRC -4
#define EBADRQ -5
#define E_SEEK -6
#define EMEDIA -7
#define ESECNF -8
#define EPAPER -9
#define EWRITF -10
#define EREADF -11

#define EWRPRO -13
#define E_CHNG -14
#define EUNDEV -15
#define EBADSF -16
#define EOTHER -17

/* GEMDOS errors */
#define EINVFN -32
#define EFILNF -33
#define EPTHNF -34
#define ENHNDL -35
#define EACCDN -36
#define EIHNDL -37

#define ENSMEM -39
#define EIMBA -40

#define EDRIVE -46
#define ENMFIL -47

#define ERANGE -64
#define EINTRN -65
#define EPLFMT -66
#define EGSBF -67
