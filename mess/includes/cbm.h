#ifndef __CBM_H_
#define __CBM_H_

#include <stdlib.h>

#include "driver.h"

/* must be defined until some driver init problems are solved */
#define NEW_GAMEDRIVER

/* global header file for 
 * vc20
 * c16
 * c64 
 * c128
 * c65*/

#if 1
/* quick (and unsafe as sprintf) snprintf */
#define snprintf cbm_snprintf
#endif
int DECL_SPEC cbm_snprintf (char *str, size_t size, const char *format,...);

#define memset16 cbm_memset16
void *cbm_memset16 (void *dest, int value, size_t size);

/**************************************************************************
 * Logging
 * call the XXX_LOG with XXX_LOG("info",(errorlog,"%fmt\n",args));
 * where "info" can also be 0 to append .."%fmt",args to a line.
 **************************************************************************/
#define LOG(LEVEL,N,M,A)  \
        { \
	  if(LEVEL>=N) { \
	    if( M ) \
              logerror("%11.6f: %-24s",timer_get_time(), (char*)M );\
	    logerror A; \
	  } \
        }

/* debugging level here for all on or off */
#if 1
# ifdef VERBOSE_DBG
#  undef VERBOSE_DBG
# endif
# if 1
#  define VERBOSE_DBG 0
# else
#  define VERBOSE_DBG 1
# endif
#else
# define PET_TEST_CODE
#endif

#if VERBOSE_DBG
#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)
#else
#define DBG_LOG(n,m,a)
#endif

typedef int bool;

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

void cbm_quick_exit (int id);
int cbm_quick_init (int id);
/* pet with basic 1 */
int cbm_pet1_quick_open (int id, int mode, void *arg);
int cbm_pet_quick_open (int id, int mode, void *arg);
int cbm_quick_open (int id, int mode, void *arg);
int cbmb_quick_open (int id, int mode, void *arg);
int cbm500_quick_open (int id, int mode, void *arg);
int cbm_c65_quick_open (int id, int mode, void *arg);

#define IODEVICE_CBM_PET1_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_pet1_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_PET_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_pet_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBMB_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbmb_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM500_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm500_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_C65_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_c65_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

/* use to functions to parse, load the rom images into memory
   and then use the cbm_rom var */
int cbm_rom_init(int id);
void cbm_rom_exit(int id);

typedef struct {
#define CBM_ROM_ADDR_UNKNOWN 0
#define CBM_ROM_ADDR_LO -1
#define CBM_ROM_ADDR_HI -2
	int addr, size;
	UINT8 *chip;
} CBM_ROM;

extern CBM_ROM cbm_rom[0x20];

#define IODEVICE_CBM_ROM(extensions, idfunc) \
{\
   IO_CARTSLOT,        /* type */\
   2,                  /* in reality 1 *//* count */\
   extensions,            /*file extensions */\
   NULL,               /* private */\
   idfunc,             /* id */\
   cbm_rom_init,       /* init */\
   cbm_rom_exit,       /* exit */\
   NULL,               /* info */\
   NULL,               /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

/* prg file format
 * sfx file format
 * sda file format
 * 0 lsb 16bit address
 * 2 chip data */

/* p00 file format (p00 .. p63, s00 .. s63, ..)
 * 0x0000 C64File
 * 0x0007 0
 * 0x0008 Name in commodore encoding?
 * 0x0018 0 0
 * 0x001a lsb 16bit address
 * 0x001c data */

/* t64 file format
 * introduced in c64s?
 * 
 */

/* d64 file format
 * vc1541 disk image
 * 256 byte sectors
 * track 1 sector 0,1,..20
 * 2
 * ..
 * 17
 * track 18 sector 0,..18
 * ..
 * 24
 * 25 sector 0,..17
 * ..
 * 30
 * 31 sector 0,..16
 * 35
 * larger d64 files contains sector chksums at the end for all
 * sectors (in the same ordering */

/* crt file format
 * file format for little endian machine contains 
 * data in big endian format
 * 0 "C64 CARTRIDGE   " (string filled with spaces)
 * 0x0010 msb 32bit size of section
 * 1 0 0 0  0 1 0 0  0 0 0 0
 * name of cartridge
 * next sections:
 * 0 CHIP
 * 4 0 0 
 * 6 msb 16bit size of section
 * 8 0 0 0
 * b ?
 * 0x000c msb 16bit address of chip
 * 0x000e msb 16bit size of data in chip 
 * 0x0010 chipdata
 * 
 * supergam: chip section offset b: 0 1 2 3, 4x data at 0x8000 size 0x4000
 * robocop2: chip section offset b: 0 .. 0x1f
 * 16x data at 0x8000 size 0x2000
 * 16x data at 0xa000 size 0x2000
 */

#endif
