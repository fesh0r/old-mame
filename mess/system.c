/******************************************************************************

  driver.c

  The list of all available drivers. Drivers have to be included here to be
  recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the DRIVER() macro. The first one
  declares external references to the drivers; the second one builds an array
  storing all the drivers.

******************************************************************************/

#include "driver.h"


#ifndef DRIVER_RECURSIVE

/* The "root" driver, defined so we can have &driver_##NAME in macros. */
struct GameDriver driver_0 =
{
  __FILE__,
  0,
  "root",
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  NOT_A_DRIVER,
};

#endif

#ifdef TINY_COMPILE
extern const struct GameDriver TINY_NAME;

const struct GameDriver * drivers[] =
{
  &TINY_NAME,
  0             /* end of array */
};

#else

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern const struct GameDriver driver_##NAME;
#define TESTDRIVER(NAME) extern const struct GameDriver driver_##NAME;
#include "system.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &driver_##NAME,
#ifdef MESS_DEBUG
#define TESTDRIVER(NAME) &driver_##NAME,
#else
#define TESTDRIVER(NAME)
#endif
const struct GameDriver *drivers[] =
{
#include "system.c"
  0             /* end of array */
};

#else /* DRIVER_RECURSIVE */

#ifndef NEOMAME

/****************CONSOLES****************************************************/

/*
  pong games
  first done in discret logic
  later pong in a chip by several firms (atari, ge, ...)

  ay-3-8500-7 6(7) games in this chip
   pelota
   squash
   fussball
   tennis
   schiessen-1
   schiessen-2
   undocumented (when all switches are off)
  nothing known about its internal design

  ay-3-8610
   Zielschiessen
   Tennis
   Hockey
   Squash
   Squash-Training
   Schiess-Training
   Gridball
   Fussball
   Basketball
   Basketball-Training

  ay-3-8765
   artisten rennen
   gelaende fahren
   motorrad rennen
   geschicklichkeitsspringen

  universum multispiel 2006: ay-3-8500-7, the six official selectable

  shg black point (type fs 1003)
  cartridge with ay-3-8610 (fs 1201)
  cartridge with ay-3-8765 (fs 1215)
 */

#ifndef MESS_EXCLUDE_ATARI
	/* ATARI */
	DRIVER( a2600 ) 	/* Atari 2600									  */
	DRIVER( a5200 ) 	/* Atari 5200									  */
	DRIVER( a7800 ) 	/* Atari 7800									  */
	DRIVER( lynx )		/* Atari Lynx Handheld							  */
	DRIVER( lynxa )		/* Atari Lynx Handheld alternate rom save		  */
	DRIVER( lynx2 )		/* Atari Lynx II Handheld redesigned, no additions*/
/*	DRIVER( jaguar )*/	/* Atari Jaguar 								  */
#endif

#ifndef MESS_EXCLUDE_NINTENDO
	/* NINTENDO */
	DRIVER( nes )		/* Nintendo Entertainment System				  */
	DRIVER( nespal )	/* Nintendo Entertainment System				  */
	DRIVER( famicom )
	DRIVER( gameboy )	/* Nintendo GameBoy Handheld					  */
	DRIVER (snes)		/* Nintendo Super Nintendo						  */
/*	DRIVER (vboy)	*/	/* Nintendo Virtual Boy 						  */
#endif

#ifndef MESS_EXCLUDE_SEGA
	/* SEGA */
	DRIVER( gamegear )	/* Sega Game Gear Handheld						  */
	DRIVER( sms )		/* Sega Sega Master System						  */
	DRIVER( genesis )	/* Sega Genesis/MegaDrive						  */
TESTDRIVER( saturn )	/* Sega Saturn									  */
#endif

#ifndef MESS_EXCLUDE_MISC_CONSOLES
	/* BALLY */
	DRIVER( astrocde )	/* Bally Astrocade								  */

	/* RCA */
TESTDRIVER( vip )		/* Cosmac VIP									  */
	DRIVER( studio2 )	/* Studio II									  */
	/* hanimex mpt-02 */
/*	DRIVER( cstudio2 )*//* Colour Studio II								  */

	/* FAIRCHILD */
	DRIVER( channelf )  /* Fairchild Channel F VES - 1976                 */
	/* checkers cartridge, additional processor in it */
	/* saba videoplay */
	/* itt telematch */
	/* nordmende teleplay */
	/* channelf system 2, redesigned, no additions */
	/* saba videoplay 2 */

	/* COLECO */
	DRIVER( coleco )	/* ColecoVision (Original BIOS )				  */
	DRIVER( colecoa )	/* ColecoVision (Thick Characters)				  */
#if 0					/* Please dont include these next 2 in a distribution, they are Hacks	*/
	DRIVER( colecofb )	/* ColecoVision (Fast BIOS load)				  */
	DRIVER( coleconb )	/* ColecoVision (No BIOS load)					  */
#endif

	/* NEC */
	DRIVER( pce )		/* PC/Engine - Turbo Graphics-16  NEC 1989-1993   */
	/* pc6601 series z80 based */
	/* pc8801 series z80 based */
	/* pc9801 series i8086 based (v30, i386, ..) */

/*  DRIVER( arcadia )*/ /* Emerson Arcadia 2001                           */
	/* schmid tvg 2000 */
	/* eduscho tele fever */
	/* hanimex fever 1 (hmg 2650) */

	/* GCE */
	DRIVER( vectrex )	/* General Consumer Electric Vectrex - 1982-1984  */
						/* (aka Milton-Bradley Vectrex) 				  */
	DRIVER( raaspec )	/* RA+A Spectrum - Modified Vectrex 			  */

/*  DRIVER( intv )*/    /* Mattel Intellivision - 1979 AKA INTV           */

	/* ENTEX */
	DRIVER( advision )	/* Adventurevision								  */

	/* CAPCOM */
	DRIVER( sfzch )     /* CPS Changer (Street Fighter ZERO)              */

	/* Magnavox */
/*	DRIVER( odyssey )*/	/* Magnavox Odyssey - analogue (1972)			  */
TESTDRIVER( odyssey2 )	/* Magnavox Odyssey 2 - 1978-1983				  */

	/* Watara */
	DRIVER( svision )	/* Super Vision	Handheld						  */

	/* 1992 epoch barcode battler handheld*/

	/* tiger game.com handheld*/

	/* interton vc4000 */

	/* bandai wonderswan handheld*/

	/* 1979 mb microvision handheld*/
#endif

/****************COMPUTERS****************************************************/
    /* ACORN */
#ifndef MESS_EXCLUDE_ACORN
    DRIVER( atom )      /* 1979 Acorn Atom                                */
    DRIVER( atomeb )    /* 1979 Acorn Atom                                */
    DRIVER( bbca )      /* 1981 BBC Micro Model A                         */
    DRIVER( bbcb )      /* 1981 BBC Micro Model B                         */
    DRIVER( bbcb1770 )  /* 1981 BBC Micro Model B with WD 1770 disc       */
    DRIVER( bbcbp )     /* 198? BBC Micro Model B+ 64K                    */
    DRIVER( bbcbp128 )  /* 198? BBC Micro Model B+ 128K                   */
/*	DRIVER( electron )*//* 198? Acorn Electron							  */

TESTDRIVER( a310 )      /* 1988 Acorn Archimedes 310                      */

TESTDRIVER( z88 )		/*												  */
#endif

#ifndef MESS_EXCLUDE_AMSTRAD
	DRIVER( cpc464 )	/* Amstrad (Schneider in Germany) 1984			  */
	DRIVER( cpc664 )	/* Amstrad (Schneider in Germany) 1985			  */
	DRIVER( cpc6128 )	/* Amstrad (Schneider in Germany) 1985			  */
/*	DRIVER( cpc464p )*/ /* Amstrad CPC464  Plus - 1987					  */
/*	DRIVER( cpc6128p )*//* Amstrad CPC6128 Plus - 1987					  */
	DRIVER( kccomp )	/* VEB KC compact								  */

	DRIVER( pcw8256 )	/* 198? PCW8256 								  */
	DRIVER( pcw8512 )	/* 198? PCW8512 								  */
	DRIVER( pcw9256 )	/* 198? PCW9256 								  */
	DRIVER( pcw9512 )	/* 198? PCW9512 (+) 							  */
	DRIVER( pcw10 ) 	/* 198? PCW10									  */

	DRIVER( pcw16 )     /* 1995 PCW16                                     */

	DRIVER( nc100 ) 	/* 19?? NC100									  */
	DRIVER( nc100a ) 	/* 19?? NC100									  */
	DRIVER( nc200 )     /* 19?? NC200									  */
#endif

#ifndef MESS_EXCLUDE_APPLE
	/* APPLE */
/*
 * CPU Model			 Month				 Year
 * -------------		 -----				 ----
 *
 * Apple I				 July				 1976
 * Apple II 			 April				 1977
 * Apple II Plus		 June				 1979
 * Apple III			 May				 1980
 * Apple IIe			 January			 1983
 * Apple III Plus		 December			 1983
 * Apple IIe Enhanced	 March				 1985
 * Apple IIc			 April				 1984
 * Apple IIc ROM 0		 ?					 1985
 * Apple IIc ROM 3		 September			 1986
 * Apple IIgs			 September			 1986
 * Apple IIe Platinum	 January			 1987
 * Apple IIgs ROM 01	 September			 1987
 * Apple IIc ROM 4		 ?					 198?
 * Apple IIc Plus		 September			 1988
 * Apple IIgs ROM 3 	 August 			 1989
 */
	DRIVER( apple1 )	/* 1976 Apple 1 								  */
	DRIVER( apple2c )	/* 1984 Apple //c								  */
	DRIVER( apple2c0 )	/* 1986 Apple //c (3.5 ROM) 					  */
	DRIVER( apple2cp )	/* 1988 Apple //c+								  */
	DRIVER( apple2e )	/* 1983 Apple //e								  */
	DRIVER( apple2ee )	/* 1985 Apple //e Enhanced						  */
	DRIVER( apple2ep )	/* 1987 Apple //e Platinum						  */
/*
 * Lisa 				 January			 1983
 * Lisa 2 				 January			 1984
 * Macintosh XL 		 January			 1985
 */
	DRIVER( lisa2 ) 	/* 1984 Apple Lisa 2							  */
	DRIVER( lisa210 ) 	/* 1984 Apple Lisa 2/10							  */
	DRIVER( macxl ) 	/* 1984 Apple Macintosh XL						  */
/*
 * Macintosh 			 January			 1984
 * Macintosh 512k		 July?				 1984
 * Macintosh 512ke		 ?					 1986
 * Macintosh Plus 		 ?					 1986
 * Macintosh SE			 ?					 1987
 * Macintosh II 		 ?					 1987
 */
/*	DRIVER( mac512k )*/	/* 1984 Apple Macintosh 512k					  */
	DRIVER( mac512ke )  /* 1986 Apple Macintosh 512ke                     */
	DRIVER( macplus )	/* 1986 Apple Macintosh Plus					  */
/*	DRIVER( mac2 )*/	/* 1987 Apple Macintosh II						  */
#endif

#ifndef MESS_EXCLUDE_ATARI
	/* ATARI */
/*
400/800 10kB OS roms
A    NTSC  (?)         (?)         (?)
A    PAL   (?)         0x72b3fed4  CO15199, CO15299, CO12399B
B    NTSC  (?)         0x0e86d61d  CO12499B, CO14599B, 12399B
B    PAL   (?)         (?)         (?)

XL/XE 16kB OS roms
10   1200XL  10/26/1982  0xc5c11546  CO60616A, CO60617A
11   1200XL  12/23/1982  (?)         CO60616B, CO60617B
1    600XL   03/11/1983  0x643bcc98  CO62024
2    XL/XE   05/10/1983  0x1f9cd270  CO61598B
3    800XE   03/01/1985  0x29f133f7  C300717
4    XEGS    05/07/1987  0x1eaf4002  C101687
*/

	DRIVER( a400 )		/* 1979 Atari 400								  */
	DRIVER( a400pal )	/* 1979 Atari 400 PAL							  */
	DRIVER( a800 )		/* 1979 Atari 800								  */
	DRIVER( a800pal )	/* 1979 Atari 800 PAL							  */
	DRIVER( a800xl )	/* 1983 Atari 800 XL							  */

//!!TESTDRIVER( atarist )	/* Atari ST 									  */
#endif

#ifndef MESS_EXCLUDE_CBM
	/* COMMODORE */
	DRIVER( kim1 )		/* Commodore (MOS) KIM-1 1975					  */
TESTDRIVER( sym1 )		/* Synertek SYM1								  */
TESTDRIVER( aim65 )		/* Rockwell AIM65								  */

	DRIVER( pet )		/* PET2001/CBM20xx Series (Basic 1) 			  */
	DRIVER( cbm30 ) 	/* Commodore 30xx (Basic 2) 					  */
	DRIVER( cbm30b )	/* Commodore 30xx (Basic 2) (business keyboard)   */
	DRIVER( cbm40 ) 	/* Commodore 40xx FAT (CRTC) 60Hz				  */
	DRIVER( cbm40pal )	/* Commodore 40xx FAT (CRTC) 50Hz				  */
	DRIVER( cbm40b )	/* Commodore 40xx THIN (business keyboard)		  */
	DRIVER( cbm80 ) 	/* Commodore 80xx 60Hz							  */
	DRIVER( cbm80pal )	/* Commodore 80xx 50Hz							  */
	DRIVER( cbm80ger )	/* Commodore 80xx German (50Hz) 				  */
	DRIVER( cbm80swe )	/* Commodore 80xx Swedish (50Hz)				  */
	DRIVER( superpet )	/* Commodore SP9000/MMF9000 (50Hz)				  */
TESTDRIVER( mmf9000 )	/* Commodore MMF9000 Swedish					  */

	DRIVER( vic20 ) 	/* Commodore Vic-20 NTSC						  */
/*	DRIVER( vic1001 )*/	/* Commodore VIC-1001 (VIC20 Japan)				  */
	DRIVER( vc20 )		/* Commodore Vic-20 PAL 						  */
	DRIVER( vic20swe )	/* Commodore Vic-20 Sweden						  */
TESTDRIVER( vic20v ) 	/* Commodore Vic-20 NTSC, VC1540				  */
TESTDRIVER( vc20v ) 	/* Commodore Vic-20 PAL, VC1541					  */
	DRIVER( vic20i )	/* Commodore Vic-20 IEEE488 Interface			  */

	DRIVER( max )		/* Max (Japan)/Ultimax (US)/VC10 (German)		  */
	DRIVER( c64 )		/* Commodore 64 - NTSC							  */
/*	DRIVER( j64 )*/		/* Commodore 64 - NTSC (Japan)					  */
	DRIVER( c64pal )	/* Commodore 64 - PAL							  */
	DRIVER( vic64s )	/* Commodore VIC64S (Swedish)					  */
	DRIVER( cbm4064 )	/* Commodore CBM4064							  */
TESTDRIVER( sx64 )		/* Commodore SX 64 - PAL						  */
TESTDRIVER( vip64 )		/* Commodore VIP64 (SX64, PAL, Swedish)			  */
	DRIVER( c64gs ) 	/* Commodore 64 Games System					  */

	DRIVER( cbm500 )	/* Commodore 500/P128-40						  */
	DRIVER( cbm610 )	/* Commodore 610/B128LP 						  */
	DRIVER( cbm620 )	/* Commodore 620/B256LP 						  */
	DRIVER( cbm620hu )	/* Commodore 620/B256LP Hungarian				  */
/*	DRIVER( cbm630 )*/	/* Commodore 630								  */
	DRIVER( cbm710 )	/* Commodore 710/B128HP 						  */
	DRIVER( cbm720 )	/* Commodore 720/B256HP 						  */
	DRIVER( cbm720se )	/* Commodore 720/B256HP Swedish/Finnish			  */
/*	DRIVER( cbm730 )*/	/* Commodore 730								  */

	DRIVER( c16 )		/* Commodore 16 								  */
	DRIVER( c16hun )	/* Commodore 16 Hungarian Character Set Hack	  */
	DRIVER( c16c )		/* Commodore 16  c1551							  */
TESTDRIVER( c16v )		/* Commodore 16  vc1541 						  */
	DRIVER( plus4 ) 	/* Commodore +4  c1551							  */
	DRIVER( plus4c )	/* Commodore +4  vc1541 						  */
TESTDRIVER( plus4v )	/* Commodore +4 								  */
	DRIVER( c364 )		/* Commodore 364 - Prototype					  */

	DRIVER( c128 )		/* Commodore 128 - NTSC 						  */
	DRIVER( c128ger )	/* Commodore 128 - PAL (german) 				  */
	DRIVER( c128fra )	/* Commodore 128 - PAL (french) 				  */
	DRIVER( c128ita )	/* Commodore 128 - PAL (italian)				  */
	DRIVER( c128swe )	/* Commodore 128 - PAL (swedish)				  */
TESTDRIVER( c128nor )	/* Commodore 128 - PAL (norwegian)				  */
TESTDRIVER( c128d )		/* Commodore 128D - NTSC 						  */

/*	DRIVER( cbm900 )*/	/* Commodore 900 Prototype (z8000)				  */

//!!	DRIVER( amiga ) 	/* Commodore Amiga								  */

	DRIVER( c65 )		/* C65 / C64DX (Prototype, NTSC, 911001)		  */
	DRIVER( c65e )		/* C65 / C64DX (Prototype, NTSC, 910828)		  */
	DRIVER( c65d )		/* C65 / C64DX (Prototype, NTSC, 910626)		  */
	DRIVER( c65c )		/* C65 / C64DX (Prototype, NTSC, 910523)		  */
	DRIVER( c65ger )	/* C65 / C64DX (Prototype, German PAL, 910429)	  */
	DRIVER( c65a )		/* C65 / C64DX (Prototype, NTSC, 910111)		  */
#endif

#ifndef MESS_EXCLUDE_IBMPC
	/* IBM PC & Clones */
	DRIVER( ibmpc )		/* 1982	IBM PC									  */
	DRIVER( ibmpca )	/* 1982 IBM PC									  */
	DRIVER( pcmda ) 	/* 1987 PC with MDA (MGA aka Hercules)			  */
	DRIVER( pc )		/* 1987 PC with CGA								  */
TESTDRIVER( bondwell )	/* 1985	Bondwell (CGA)                         	  */
	DRIVER( europc )	/* 1988	Schneider Euro PC (CGA or Hercules)		  */

	/* pc junior */
TESTDRIVER( ibmpcjr )	/*      IBM PC Jr								  */
	DRIVER( t1000hx )	/* 1987 Tandy 1000HX (similiar to PCJr) 		  */

	/* xt */
	DRIVER( ibmxt )		/* 1986	IBM XT									  */
	DRIVER( pc200 )     /* 1988 Sinclair PC200                            */
	DRIVER( pc20 )      /* 1988 Amstrad PC20                              */   	
	DRIVER( pc1512 )	/* 1986 Amstrad PC1512 (CGA compatible)			  */
	DRIVER( pc1640 )	/* 1987 Amstrad PC1640 (EGA compatible)			  */

	DRIVER( xtvga ) 	/* 198? PC-XT (VGA, MF2 Keyboard)				  */

	/* at */
TESTDRIVER( ibmat )		/* 1985	IBM AT									  */
	DRIVER( at )		/* 1987 AMI Bios and Diagnostics				  */
TESTDRIVER( atvga ) 	/*												  */
TESTDRIVER( neat )		/* 1989	New Enhanced AT chipset, AMI BIOS		  */

/*	DRIVER( at386)*/	/*												  */
#endif

#ifndef MESS_EXCLUDE_SINCLAIR
	/* SINCLAIR */
	DRIVER( zx80 )		/* Sinclair ZX-80								  */
	DRIVER( zx81 )		/* Sinclair ZX-81								  */
	DRIVER( ts1000 )	/* Timex Sinclair 1000							  */
	DRIVER( aszmic )	/* ASZMIC ZX-81 ROM swap						  */
	DRIVER( pc8300 )	/* Your Computer - PC8300						  */
	DRIVER( pow3000 )	/* Creon Enterprises - Power 3000				  */

	DRIVER( spectrum )	/* 1982 ZX Spectrum 							  */
	DRIVER( specpls4 )	/* 2000 ZX Spectrum +4							  */
	DRIVER( specbusy )	/* 1994 ZX Spectrum (BusySoft Upgrade)			  */
	DRIVER( specgrot )	/* ???? ZX Spectrum (De Groot's Upgrade)          */
	DRIVER( specimc )	/* 1985 ZX Spectrum (Collier's Upgrade)           */
	DRIVER( speclec )	/* 1987 ZX Spectrum (LEC Upgrade)				  */
	DRIVER( inves ) 	/* 1986 Inves Spectrum 48K+ 					  */
	DRIVER( tk90x ) 	/* 1985 TK90x Color Computer					  */
	DRIVER( tk95 )		/* 1986 TK95 Color Computer 					  */
	DRIVER( tc2048 )	/* 198? TC2048									  */
	DRIVER( ts2068 )	/* 1983 TS2068									  */

	DRIVER( spec128 )	/* 1986 ZX Spectrum 128"                          */
	DRIVER( spec128s )	/* 1985 ZX Spectrum 128 (Spain) 				  */
	DRIVER( specpls2 )	/* 1986 ZX Spectrum +2							  */
	DRIVER( specpl2a )	/* 1987 ZX Spectrum +2a 						  */
	DRIVER( specpls3 )	/* 1987 ZX Spectrum +3							  */

	DRIVER( specp2fr )	/* 1986 ZX Spectrum +2 (France) 				  */
	DRIVER( specp2sp )	/* 1986 ZX Spectrum +2 (Spain)					  */
	DRIVER( specp3sp )	/* 1987 ZX Spectrum +3 (Spain)					  */
	DRIVER( specpl3e )	/* 2000 ZX Spectrum +3e 						  */
#endif

#ifndef MESS_EXCLUDE_SHARP
	/* SHARP */
	DRIVER( pc1251 )	/* Pocket Computer 1251 						  */
	DRIVER( pc1401 )	/* Pocket Computer 1401 						  */
	DRIVER( pc1402 )	/* Pocket Computer 1402 						  */
	DRIVER( pc1350 )	/* Pocket Computer 1350 						  */

	DRIVER( mz700 ) 	/* 1982 Sharp MZ700 							  */
	DRIVER( mz700j )	/* 1982 Sharp MZ700 Japan						  */
TESTDRIVER( mz800  )	/* 1982 Sharp MZ800 							  */

/*	DRIVER( x68000 )*/	/* X68000										  */
#endif

#ifndef MESS_EXCLUDE_TI
	/* TEXAS INSTRUMENTS */
TESTDRIVER( ti990_4 )	/* 197? TI 990/4								  */
TESTDRIVER( ti99_224 )	/* 1983 TI 99/2 								  */
TESTDRIVER( ti99_232 )	/* 1983 TI 99/2 								  */
	DRIVER( ti99_4 )	/* 1978 TI 99/4 								  */
	DRIVER( ti99_4e )	/* 1980 TI 99/4E								  */
	DRIVER( ti99_4a )	/* 1981 TI 99/4A								  */
	DRIVER( ti99_4ae )	/* 1981 TI 99/4AE								  */

    DRIVER( avigo )     /*                                                */
#endif

#ifndef MESS_EXCLUDE_MISC_COMPUTERS
	/* CANTAB */
	DRIVER( jupiter )	/* Jupiter Ace									  */

	/* INTELLIGENT SOFTWARE */
	DRIVER( ep128 ) 	/* Enterprise 128 k 							  */
	DRIVER( ep128a )	/* Enterprise 128 k 							  */

	/* NON LINEAR SYSTEMS */
	DRIVER( kaypro )	/* Kaypro 2X									  */

	/* VEB MIKROELEKTRONIK */
	/* KC compact is partial CPC compatible */
	DRIVER( kc85_4 )	/* VEB KC 85/4									  */
    DRIVER( kc85_3 )    /* VEB KC 85/3                                    */
TESTDRIVER( kc85_4d )   /* VEB KC 85/4 with disk interface                */
    /* pc1715 z80/u880 based */
	/* pc1715w z80/u880 based */
	/* a5105 z80/u880 based */
	/* a5120 z80/u880 based */
	/* a7100 i8086 based */

	/* MICROBEE SYSTEMS */
	DRIVER( mbee )		/* Microbee 									  */
	DRIVER( mbeepc )	/* Microbee (Personal Communicator)				  */
	DRIVER( mbee56k )	/* Microbee 56K (CP/M)							  */

	/* TANDY RADIO SHACK */
	DRIVER( trs80 )	    /* TRS-80 Model I	- Radio Shack Level I BASIC   */
	DRIVER( trs80l2 ) 	/* TRS-80 Model I	- Radio Shack Level II BASIC  */
	DRIVER( trs80l2a )	/* TRS-80 Model I	- R/S L2 BASIC				  */
	DRIVER( sys80 ) 	/* EACA System 80								  */
/*	DRIVER( trs80m2 )*/	/* TRS-80 Model II -							  */
TESTDRIVER( trs80m3 )	/* TRS-80 Model III - Radio Shack/Tandy 		  */

	DRIVER( coco )		/* Color Computer								  */
	DRIVER( cocoe )		/* Color Computer (Extended BASIC 1.0)			  */
	DRIVER( coco2 ) 	/* Color Computer 2 							  */
	DRIVER( coco2b ) 	/* Color Computer 2B (uses M6847T1 video chip)    */
	DRIVER( coco3 ) 	/* Color Computer 3 							  */
	DRIVER( coco3h )	/* Hacked Color Computer 3 (6309)				  */
	DRIVER( dragon32 )	/* Dragon32 									  */
	DRIVER( cp400 ) 	/* Prologica CP400								  */
	DRIVER( mc10 )		/* MC-10										  */

	/* dragon 32 coco compatible */
/*	DRIVER( dragon64 */	/* Dragon 64									  */

	/* EACA */
	DRIVER( cgenie )	/* Colour Genie EG2000							  */
	/* system 80 trs80 compatible */

	/* VIDEO TECHNOLOGY */
	DRIVER( laser110 )	/* 1983 Laser 110								  */
	DRIVER( laser200 )	/* 1983 Laser 200								  */
	DRIVER( laser210 )	/* 1983 Laser 210 (indentical to Laser 200 ?)	  */
	DRIVER( laser310 )	/* 1983 Laser 310 (210 with diff. keyboard and RAM) */
	DRIVER( vz200 ) 	/* 1983 Dick Smith Electronics / Sanyo VZ200	  */
	DRIVER( vz300 ) 	/* 1983 Dick Smith Electronics / Sanyo VZ300	  */
	DRIVER( fellow )	/* 1983 Salora Fellow (Finland) 				  */
	DRIVER( tx8000 )	/* 1983 Texet TX-8000 (U.K.)					  */
	DRIVER( laser350 )	/* 1984? Laser 350								  */
	DRIVER( laser500 )	/* 1984? Laser 500								  */
	DRIVER( laser700 )	/* 1984? Laser 700								  */
	/* Creativision console */

	/* TANGERINE */
	DRIVER( microtan )	/* 1979 Microtan 65 							  */

	DRIVER( oric1 ) 	/* 1983 Oric 1									  */
	DRIVER( orica ) 	/* 1984 Oric Atmos								  */
	DRIVER( prav8d )    /* 1985 Pravetz 8D                                  */
	DRIVER( prav8dd )   /* 1989 Pravetz 8D (Disk ROM)                       */
	DRIVER( prav8dda )  /* 1989 Pravetz 8D (Disk ROM, alternate)            */
	DRIVER( telstrat )	/* ??? Oric Telestrat/Stratos						*/

	/* PHILIPS */
	DRIVER( p2000t )	/* 1980 P2000T									  */
	DRIVER( p2000m )	/* 1980 P2000M									  */
	/* philips g7000 odyssey2 compatible */

	/* COMPUKIT */
	DRIVER( uk101 ) 	/* 1979 UK101									  */

	/* OHIO SCIENTIFIC */
	DRIVER( superbrd )	/* 1979 Superboard II							  */

	/* ASCII & MICROSOFT */
	DRIVER( msx )		/* 1983 MSX 									  */
	DRIVER( msxj )		/* 1983 MSX Jap 								  */
	DRIVER( msxkr ) 	/* 1983 MSX Korean								  */
	DRIVER( msxuk ) 	/* 1983 MSX UK									  */
	DRIVER( hotbit11 )	/* 198? ???									      */
	DRIVER( hotbit12 )	/* 198? ???									      */
	DRIVER( expert10 )	/* 198? ???									      */
	DRIVER( expert11 )	/* 198? ???									      */
	DRIVER( msx2 ) 		/* 1985 MSX2									  */
	DRIVER( msx2a )		/* 1985 MSX2									  */
	DRIVER( msx2j ) 	/* 1983 MSX2 Jap								  */

	/* NASCOM MICROCOMPUTERS */
	DRIVER( nascom1 )	/* 1978 Nascom 1								  */
	DRIVER( nascom2 )	/* 1979 Nascom 2								  */

	/* MILES GORDON TECHNOLOGY */
	DRIVER( coupe ) 	/* 1989 Sam Coupe 256K RAM						  */
	DRIVER( coupe512 )	/* 1989 Sam Coupe 512K RAM						  */

	/* MOTOROLA */
TESTDRIVER( mekd2 )     /* 1977 Motorola Evaluation Kit                   */

	/* DEC */
	DRIVER( pdp1 )      /* 1962 DEC PDP1 for SPACEWAR! - 1962             */

	/* MEMOTECH */
	DRIVER( mtx512 )    /* 1983 Memotech MTX512                           */

	/* MATTEL */
//TESTDRIVER( intvkbd )	/* 1981 - Mattel Intellivision Keyboard Component */
						/* (Test marketed, later recalled )				  */
	DRIVER( aquarius )	/* 1983 Aquarius								  */
	
	/*EXIDY INC */
	DRIVER( exidy )  /* Sorcerer                                       */
	
	/* GALAKSIJA */
	DRIVER( galaxy )
	
	/* Team Concepts */
	/* CPU not known, else should be easy, look into systems/comquest.c */
TESTDRIVER( comquest )	/* Comquest Plus German							*/

	/* Hewlett Packard */
/*DRIVER( hp48s )*/ 	/* HP48 S/SX 							  		  */
/*DRIVER( hp48g )*/ 	/* HP48 G/GX 							  		  */

	/* SpectraVideo */
	DRIVER( svi318 ) 	/* SVI-318										  */
	DRIVER( svi328 ) 	/* SVI-328										  */
	DRIVER( svi328a ) 	/* SVI-328	(BASIC 1.11)						  */
#endif

/****************Games*******************************************************/

#ifndef MESS_EXCLUDE_MISC_GAMES
	/* The Ideal Game Corp. */
	/* distributed by ARXON in Germany/Austria */
	/* PIC1655A (NMOS, not CMOS 16C55) dumping problems */
/*	DRIVER( maniac )*/	/* Maniac										  */

	/* Computer Electronic */
TESTDRIVER( mk1 )		/* Chess Champion MK I							  */
	/* Quelle International */
	DRIVER( mk2 )		/* Chess Champion MK II							  */
	/* NOVAG Industries Ltd. */
TESTDRIVER( ssystem3 )	/* Chess Champion Super System III / MK III		  */

	/* tchibo */
	/* single chip with ram, rom, io without label, how to dump? */
/*	DRIVER( partner3)*/	/* Chess Partner 3 - Kasparov					  */
#endif

#endif /* NEOMAME */

#endif /* DRIVER_RECURSIVE */

#endif /* TINY_COMPILE */

