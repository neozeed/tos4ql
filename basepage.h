/* Basepage defined constants - and structure */
#define P_CMDLIN 0x80
#define P_ENV 0x2C
#define P_RESERVED 0x28
#define P_PROGSTART 0x100

struct basepage
{
	LONG p_lowtpa; /* -> base of TPA */
	LONG p_hitpa; /* -> end of TPA */
	LONG p_tbase; /* base of text segment */
	LONG p_tlen; /* size of text segment */
	LONG p_dbase; /* base of data segment */
	LONG p_dlen; /* size of data segment */
	LONG p_bbase; /* base of BSS segment */
	LONG p_blen; /* size of BSS segment */
	LONG p_dta; /* Disk Transfer Address */
	LONG p_parent; /* -> parent's basepage */
};
