/******************************************************************************
	BBC Model A,B

	BBC Model B Plus

	MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/bbc.h"
#include "machine/6522via.h"
#include "includes/upd7002.h"
#include "includes/basicdsk.h"


/******************************************************************************
&FC00-&FCFF FRED															1Mhz
&FD00-&FDFF JIM																1Mhz
&FE00-&FEFF SHEILA
&00-&07 6845 CRTC		Video controller				8  ( 2 bytes x	4 ) 1Mhz
&08-&0F 6850 ACIA		Serial controller				8  ( 2 bytes x	4 ) 1Mhz
&10-&17 Serial ULA		Serial system chip				8  ( 1 byte  x  8 ) 1Mhz
&18-&1f INTOFF/STATID   ECONET Interrupt Off / ID No.   8  ( 1 byte  x  8 ) 1Mhz
write:
&20-&2F Video ULA		Video system chip				16 ( 2 bytes x	8 ) 2Mhz
&30-&3F 74LS161 		Paged ROM selector				16 ( 1 byte  x 16 ) 2Mhz
read:
&20-&2F INTON   		ECONET Interrupt On				16 ( 1 bytes x 16 ) 2Mhz
&30-&3F Not Connected   Not Connected										2Mhz

&40-&5F 6522 VIA		SYSTEM VIA						32 (16 bytes x	2 ) 1Mhz
&60-&7F 6522 VIA		USER VIA						32 (16 bytes x	2 ) 1Mhz
&80-&9F 8271 FDC		FDC Floppy disc controller		32 ( 8 bytes x	4 ) 2Mhz
&A0-&BF 68B54 ADLC		ECONET controller				32 ( 4 bytes x	8 ) 2Mhz
&C0-&DF uPD7002 		Analogue to digital converter	32 ( 4 bytes x	8 ) 1Mhz
&E0-&FF Tube ULA		Tube system interface			32 (32 bytes x  1 ) 2Mhz
******************************************************************************/


static MEMORY_READ_START(readmem_bbca)
	{ 0x0000, 0x3fff, MRA_RAM		   },
	{ 0x4000, 0x7fff, MRA_BANK1 	   },  /* bank 1 is a repeat of the memory at 0x0000 to 0x3fff	 */
	{ 0x8000, 0xbfff, MRA_BANK3 	   },  /* Paged ROM                                              */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                     */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                       */
										   /* Shiela Address Page &fe00 - &feff 					 */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			     */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			     */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		     */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID 1 ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         1 ECONET Interrupt On		     */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		     */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 					 */
	{ 0xfe60, 0xfe7f, BBC_NOP_00_r	   },  /* &60-&7f  6522 VIA 	 1 USER VIA 					 */
	{ 0xfe80, 0xfe9f, BBC_NOP_00_r	   },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller		 */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 1 ECONET controller			 */
	{ 0xfec0, 0xfedf, BBC_NOP_00_r	   },  /* &c0-&df  uPD7002		 1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 1 Tube system interface		 */
	{ 0xff00, 0xffff, MRA_BANK2		   },  /* Hardware marked with a 1 is not present in a Model A	 */
MEMORY_END

static MEMORY_WRITE_START(writemem_bbca)
	{ 0x0000, 0x3fff, memory_w		   },
	{ 0x4000, 0x7fff, memory_w		   },  /* this is a repeat of the memory at 0x0000 to 0x3fff	 */
	{ 0x8000, 0xdfff, MWA_ROM		   },  /* Paged ROM                                              */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                     */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                       */
										   /* Shiela Address Page &fe00 - &feff 					 */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			     */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			     */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		     */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No.   */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip				 */
	{ 0xfe30, 0xfe3f, page_selecta_w   },  /* &30-&3f  84LS161		 Paged ROM selector 			 */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 					 */
	{ 0xfe60, 0xfe7f, MWA_NOP		   },  /* &60-&7f  6522 VIA 	 1 USER VIA 					 */
	{ 0xfe80, 0xfe9f, MWA_NOP		   },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller		 */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 1 ECONET controller			 */
	{ 0xfec0, 0xfedf, MWA_NOP		   },  /* &c0-&df  uPD7002		 1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 1 Tube system interface		 */
	{ 0xff00, 0xffff, MWA_ROM		   },  /* Hardware marked with a 1 is not present in a Model A	 */
MEMORY_END


static MEMORY_READ_START(readmem_bbcb)
	{ 0x0000, 0x7fff, MRA_RAM		   },
	{ 0x8000, 0xbfff, MRA_BANK3 	   },
	{ 0xc000, 0xfbff, MRA_BANK2		   },
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         ECONET Interrupt On		   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_i8271_read   },  /* &80-&9f  8271 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r	       },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcb)
	{ 0x0000, 0x7fff, memory_w		   },
	{ 0x8000, 0xdfff, MWA_ROM		   },
	{ 0xc000, 0xfdff, MWA_ROM		   },
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_i8271_write  },  /* &80-&9f  8271 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w        },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


static MEMORY_READ_START(readmem_bbcb1770)
	{ 0x0000, 0x7fff, MRA_RAM		   },
	{ 0x8000, 0xbfff, MRA_BANK3 	   },
	{ 0xc000, 0xfbff, MRA_BANK2		   },
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         ECONET Interrupt On		   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r  	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcb1770)
	{ 0x0000, 0x7fff, memory_w		   },
	{ 0x8000, 0xdfff, MWA_ROM		   },
	{ 0xc000, 0xfdff, MWA_ROM		   },
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


static MEMORY_READ_START(readmem_bbcbp)
	{ 0x0000, 0x2fff, MRA_RAM		   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, MRA_BANK1        },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, MRA_BANK3 	   },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xbfff, MRA_BANK4        },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         ECONET Interrupt On		   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r 	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcbp)
	{ 0x0000, 0x2fff, memorybp0_w	   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, memorybp1_w      },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, memorybp3_w      },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xdfff, MWA_ROM          },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectbp_w  },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END



static MEMORY_READ_START(readmem_bbcbp128)
	{ 0x0000, 0x2fff, MRA_RAM		   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, MRA_BANK1        },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, MRA_BANK3 	   },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xbfff, MRA_BANK4        },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         ECONET Interrupt On		   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r 	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcbp128)
	{ 0x0000, 0x2fff, memorybp0_w	   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, memorybp1_w      },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, memorybp3_128_w  },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xdfff, memorybp4_128_w  },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectbp_w  },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


/***************/
/* bbc TUBE    */
/***************/
// This code can live here right now but will get turned into a fully generic TUBE driver (in its own file) when it is working
// This tube code is very much WIP right now.


UINT8  r1ph[24];
UINT8  r1ph_top=0;
UINT8  r1ph_bottom=0;
UINT8  r1ph_da=0;
UINT8  r1ph_nf=1;

UINT8  r1hp[1];
UINT8  r1hp_top=0;
UINT8  r1hp_bottom=0;
UINT8  r1hp_da=0;
UINT8  r1hp_nf=1;
UINT8  r1hp_p=0;
UINT8  r1hp_v=0;
UINT8  r1hp_m=0;
UINT8  r1hp_j=0;
UINT8  r1hp_i=0;
UINT8  r1hp_q=0;


UINT8  r2ph[1];
UINT8  r2ph_top=0;
UINT8  r2ph_bottom=0;
UINT8  r2ph_da=0;
UINT8  r2ph_nf=1;

UINT8  r2hp[1];
UINT8  r2hp_top=0;
UINT8  r2hp_bottom=0;
UINT8  r2hp_da=0;
UINT8  r2hp_nf=1;

UINT8  r3ph[2];
UINT8  r3ph_top=0;
UINT8  r3ph_bottom=0;
UINT8  r3ph_da=0;
UINT8  r3ph_nf=1;

UINT8  r3hp[2];
UINT8  r3hp_top=0;
UINT8  r3hp_bottom=0;
UINT8  r3hp_da=0;
UINT8  r3hp_nf=1;

UINT8  r4ph[1];
UINT8  r4ph_top=0;
UINT8  r4ph_bottom=0;
UINT8  r4ph_da=0;
UINT8  r4ph_nf=1;

UINT8  r4hp[1];
UINT8  r4hp_top=0;
UINT8  r4hp_bottom=0;
UINT8  r4hp_da=0;
UINT8  r4hp_nf=1;

int    r1irq=0;
int    r3nmi=0;
int    r4irq=0;

static WRITE_HANDLER ( tube_port_a_w )
{
	switch (offset&0x7)
	{
	case 0:
		logerror("Port A write to R1STAT %02X\n",data);
		//r1hp_da=(data>>7)|1;
		//r1hp_nf=(data>>6)|1;
		r1hp_p =(data>>5)|1;
		r1hp_v =(data>>4)|1;
		r1hp_m =(data>>3)|1;
		r1hp_j =(data>>2)|1;
		r1hp_i =(data>>1)|1;
		r1hp_q =(data>>0)|1;
		break;
	case 1:
		logerror("Port A write to R1DATA %02X\n",data);
		exit(1);
		break;
	case 2:
		logerror("Port A write to R2STAT %02X\n",data);
		exit(1);
		break;
	case 3:
		logerror("Port A write to R2DATA %02X\n",data);
		exit(1);
		break;
	case 4:
		logerror("Port A write to R3STAT %02X\n",data);
		exit(1);
		break;
	case 5:
		if (r3hp_nf)
		{
			logerror("Port A write to R3DATA %02X\n",data);
			r3hp[r3hp_top]=data;
			r3hp_top=(r3hp_top+1)%1;
			r3hp_da=1;
			if (r3hp_top==r3hp_bottom) r3hp_nf=0;

			if (r1hp_m && (r3nmi==0)) {
				r3nmi=1;
				cpu_set_irq_line(1, M6502_INT_NMI, r3nmi);
			}
		} else {
			logerror("Port A write to R3DATA on full buffer %02X\n",data);
			r3hp[r3hp_top]=data;
			r3hp_top=(r3hp_top+1)%1;
			r3hp_da=1;
			if (r3hp_top==r3hp_bottom) r3hp_nf=0;

			if (r1hp_m && (r3nmi==0)) {
				r3nmi=1;
				cpu_set_irq_line(1, M6502_INT_NMI, r3nmi);
			}
		}
		break;
	case 6:
		logerror("Port A write to R4STAT %02X\n",data);
		exit(1);
		break;
	case 7:
		logerror("Port A write to R4DATA %02X\n",data);
		if (r4hp_nf)
		{
			r4hp[r4hp_top]=data;
			r4hp_top=(r4hp_top+1)%1;
			r4hp_da=1;
			if (r4hp_top==r4hp_bottom) r4hp_nf=0;

			if (r1hp_j) {
				r4irq=1;
				cpu_set_irq_line(1, M6502_IRQ_LINE, r1irq|r4irq);
			}
		} else {
			logerror("Port A write to R4DATA on full buffer\n");
			exit(1);
		}
		break;
	};
}

static READ_HANDLER ( tube_port_a_r )
{
	UINT8 retval=0x00;

	switch (offset&0x7)
	{
	case 0:
		logerror("Port A read from R1STAT\n");
		retval=(r1ph_da<<7) | (r1hp_nf<<6) | 0x3f;
		break;
	case 1:
		logerror("Port A read from R1DATA\n");
		if (r1ph_da)
		{
			retval=r1ph[r1ph_bottom];
			r1ph_bottom=(r1ph_bottom+1)%24;
			r1ph_nf=1;
			if (r1ph_top==r1ph_bottom) r1ph_da=0;
		} else {
			logerror("Port A read from R1DATA on empty buffer\n");
			exit(1);
		}

		break;
	case 2:
		logerror("Port A read from R2STAT\n");
		exit(1);
		break;
	case 3:
		logerror("Port A read from R2DATA\n");
		exit(1);
		break;
	case 4:
		logerror("Port A read from R3STAT\n");
		exit(1);
		break;
	case 5:
		logerror("Port A read from R3DATA\n");
		exit(1);
		break;
	case 6:
		logerror("Port A read from R4STAT\n");
		retval=(r4ph_da<<7) | (r4hp_nf<<6) | 0x3f;
		break;
	case 7:
		logerror("Port A read from R4DATA\n");
		exit(1);
		break;
	};
	logerror("returning %02X\n",retval);
	return retval;
}


static WRITE_HANDLER ( tube_port_b_w )
{
	switch (offset&0x7)
	{
	case 0:
		logerror("Port B write to R1STAT %02X\n",data);
		exit(1);
		break;
	case 1:
		logerror("Port B write to R1DATA %02X\n",data);
		if (r1ph_nf)
		{
			r1ph[r1ph_top]=data;
			r1ph_top=(r1ph_top+1)%24;
			r1ph_da=1;
			if (r1ph_top==r1ph_bottom) r1ph_nf=0;
		} else {
			logerror("Port B write to R1DATA on full buffer\n");
			exit(1);
		}
		break;
	case 2:
		logerror("Port B write to R2STAT %02X\n",data);
		exit(1);
		break;
	case 3:
		logerror("Port B write to R2DATA %02X\n",data);
		exit(1);
		break;
	case 4:
		logerror("Port B write to R3STAT %02X\n",data);
		exit(1);
		break;
	case 5:
		logerror("Port B write to R3DATA %02X\n",data);
		exit(1);
		break;
	case 6:
		logerror("Port B write to R4STAT %02X\n",data);
		exit(1);
		break;
	case 7:
		logerror("Port B write to R4DATA %02X\n",data);
		exit(1);
		break;
	};
}


static READ_HANDLER ( tube_port_b_r )
{
	UINT8 retval=0x00;

	switch (offset&0x7)
	{
	case 0:
		logerror("Port B read from R1STAT\n");
		retval=(r1hp_da<<7) | (r1ph_nf<<6) | 0x3f;
		break;
	case 1:
		logerror("Port B read from R1DATA\n");
		exit(1);
		break;
	case 2:
		logerror("Port B read from R2STAT\n");
		retval=(r2hp_da<<7) | (r2ph_nf<<6) | 0x3f;
		break;
	case 3:
		logerror("Port B read from R2DATA\n");
		exit(1);
		break;
	case 4:
		logerror("Port B read from R3STAT\n");
		retval=(r3hp_da<<7) | (r3ph_nf<<6) | 0x3f;
		break;
	case 5:
		if (r3hp_da)
		{
			logerror("Port B read from R3DATA\n");

			retval=r3hp[r3hp_bottom];
			r3hp_bottom=(r3hp_bottom+1)%1;
			r3hp_nf=1;
			if (r3hp_top==r3hp_bottom) r3hp_da=0;
			if (r3nmi==1)
			{
				r3nmi=0;
				cpu_set_irq_line(1, M6502_INT_NMI, r3nmi);
			}
		} else {
			logerror("Port B read from R3DATA on empty buffer\n");
			retval=r3hp[r3hp_bottom];
			if (r3nmi==1)
			{
				r3nmi=0;
				cpu_set_irq_line(1, M6502_INT_NMI, r3nmi);
			}
		}
		break;
	case 6:
		logerror("Port B read from R4STAT\n");
		retval=(r4hp_da<<7) | (r4ph_nf<<6) | 0x3f;
		break;
	case 7:
		if (r4hp_da)
		{
			logerror("Port B read from R4DATA\n");

			retval=r4hp[r4hp_bottom];
			r4hp_bottom=(r4hp_bottom+1)%1;
			r4hp_nf=1;
			if (r4hp_top==r4hp_bottom) r4hp_da=0;
			if (r4irq==1)
			{
				r4irq=0;
				cpu_set_irq_line(1, M6502_IRQ_LINE, r1irq|r4irq);
			}
		} else {
			logerror("Port B read from R4DATA on empty buffer\n");
			exit(1);
		}
		break;
	};

	logerror("returning %02X\n",retval);
	return retval;
}


/******************************************************/
/* Starting to look at adding a 6502 second processor */
/******************************************************/

//the 6502 second processor emulation is more or less complete with this,
//I just need to write the TUBE driver now

//initially the rom is mapped in for read cycles when A15 is high
//the first read or write to the tube maps the rom out leaving only the ram accessable
//the first access to the tube is a read, so the map out code is only implemented in the read function




static READ_HANDLER ( bbcs_tube_r )
{

	if (startbank)
	{
		cpu_setbank(5,memory_region(REGION_CPU2)+0xf000);
		startbank=0;
	}
	return tube_port_b_r( offset );
}



/* now add a bbc B with the Tube Connected */

static MEMORY_READ_START(readmem_bbcbtube)
	{ 0x0000, 0x7fff, MRA_RAM		   },
	{ 0x8000, 0xbfff, MRA_BANK3 	   },
	{ 0xc000, 0xfbff, MRA_BANK2		   },
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, BBC_NOP_00_r	   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, BBC_NOP_00_r	   },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, BBC_NOP_00_r	   },  /* &20-&2f  INTON         ECONET Interrupt On		   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC    		 Not Connected		 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r  	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, tube_port_a_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcbtube)
	{ 0x0000, 0x7fff, memory_w		   },
	{ 0x8000, 0xdfff, MWA_ROM		   },
	{ 0xc000, 0xfdff, MWA_ROM		   },
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe17, MWA_NOP		   },  /* &10-&17  Serial ULA	 Serial system chip 		   */
	{ 0xfe18, 0xfe1f, MWA_NOP          },  /* &18-&1f  INTOFF/STATID ECONET Interrupt Off / ID No. */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, tube_port_a_w	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END

/* Tube memory */

static MEMORY_READ_START(readmem_bbc6502)
	{ 0x0000, 0xefff, MRA_RAM       },
	{ 0xf000, 0xfeF7, MRA_BANK5     },
	{ 0xfef8, 0xfeff, bbcs_tube_r   },
	{ 0xff00, 0xffff, MRA_BANK5     },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbc6502)
	{ 0x0000, 0xfeF7, MWA_RAM       },
	{ 0xfef8, 0xfeff, tube_port_b_w },
	{ 0xff00, 0xffff, MWA_RAM       },
MEMORY_END



// end of tube connection WIP
/********************/



unsigned short bbc_colour_table[8]=
{
	0,1,2,3,4,5,6,7
};

unsigned char	bbc_palette[8*3]=
{

	0x0ff,0x0ff,0x0ff,
	0x000,0x0ff,0x0ff,
	0x0ff,0x000,0x0ff,
	0x000,0x000,0x0ff,
	0x0ff,0x0ff,0x000,
	0x000,0x0ff,0x000,
	0x0ff,0x000,0x000,
	0x000,0x000,0x000
};

static void init_palette_bbc(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,bbc_palette,sizeof(bbc_palette));
	memcpy(sys_colortable,bbc_colour_table,sizeof(bbc_colour_table));
}

INPUT_PORTS_START(bbca)

	/* KEYBOARD COLUMN 0 */
	PORT_START
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",      KEYCODE_RSHIFT,   IP_JOY_NONE)
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",      KEYCODE_LSHIFT,   IP_JOY_NONE)
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",          KEYCODE_Q,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F0",         KEYCODE_0_PAD,    IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "1",          KEYCODE_1,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK",  KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT LOCK", KEYCODE_LALT,     IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB",        KEYCODE_TAB,      IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "ESCAPE",     KEYCODE_ESC,      IP_JOY_NONE)

	/* KEYBOARD COLUMN 1 */
	PORT_START
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",       KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "3",          KEYCODE_3,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "W",          KEYCODE_W,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "2",          KEYCODE_2,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "A",          KEYCODE_A,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "S",          KEYCODE_S,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",          KEYCODE_Z,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",         KEYCODE_1_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 2 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 8 (Not Used)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "4",          KEYCODE_4,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "E",          KEYCODE_E,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "D",          KEYCODE_D,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "X",          KEYCODE_X,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "C",          KEYCODE_C,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",      KEYCODE_SPACE,    IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",         KEYCODE_2_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 3 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 7 (Not Used)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "5",          KEYCODE_5,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "T",          KEYCODE_T,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "R",          KEYCODE_R,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F",          KEYCODE_F,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "G",          KEYCODE_G,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "V",          KEYCODE_V,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",         KEYCODE_3_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 4 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 6 (Disc Speed 1)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",         KEYCODE_4_PAD,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "7",          KEYCODE_7,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "6",          KEYCODE_6,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",          KEYCODE_Y,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "H",          KEYCODE_H,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "B",          KEYCODE_B,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",         KEYCODE_5_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 5 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 5 (Disc Speed 0)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "8",          KEYCODE_8,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "I",          KEYCODE_I,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "U",          KEYCODE_U,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "J",          KEYCODE_J,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "N",          KEYCODE_N,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "M",          KEYCODE_M,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F6",         KEYCODE_6_PAD,    IP_JOY_NONE)

/* KEYBOARD COLUMN 6 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 4 (Shift Break)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F7",         KEYCODE_7_PAD,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "9",          KEYCODE_9,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "O",          KEYCODE_O,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "K",          KEYCODE_K,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "L",          KEYCODE_L,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, ",",          KEYCODE_COMMA,    IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F8",         KEYCODE_3_PAD,    IP_JOY_NONE)

/* KEYBOARD COLUMN 7 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 3 (Mode bit 2)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "-",          KEYCODE_MINUS,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "0",          KEYCODE_0,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "P",          KEYCODE_P,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "@",          KEYCODE_BACKSLASH,IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, ";",          KEYCODE_COLON,    IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, ".",          KEYCODE_STOP,     IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F9",         KEYCODE_9_PAD,    IP_JOY_NONE)


	/* KEYBOARD COLUMN 8 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 2 (Mode bit 1)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "^",          KEYCODE_EQUALS,     IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "_",          KEYCODE_TILDE,      IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "[",          KEYCODE_OPENBRACE,  IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, ":",          KEYCODE_QUOTE,      IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "]",          KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "/",          KEYCODE_SLASH,      IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "\\",         KEYCODE_BACKSLASH2, IP_JOY_NONE)

	/* KEYBOARD COLUMN 9 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 1 (Mode bit 0)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR LEFT", KEYCODE_LEFT,      IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR DOWN", KEYCODE_DOWN,      IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR UP",   KEYCODE_UP,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN",      KEYCODE_ENTER,     IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "DELETE",      KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "COPY",        KEYCODE_END,       IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR RIGHT",KEYCODE_RIGHT,     IP_JOY_NONE)


	PORT_START  // KEYBOARD COLUMN 10 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 11 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 12 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 13 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 14 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 15 RESERVED FOR BBC MASTER


	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff )

INPUT_PORTS_END


/* the BBC came with 4 rom sockets on the mother board as shown in the model A driver */
/* you could get a number of rom upgrade boards that took this up to 16 roms as in the */
/* model B driver */

ROM_START(bbca)
	ROM_REGION(0x04000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x14000,REGION_USER1,0) /* ROM */
	ROM_LOAD("os12.rom",    0x10000,  0x4000, 0x3c14fc70)

														  /* rom page 0  00000 */
														  /* rom page 1  04000 */
														  /* rom page 2  08000 */
	ROM_LOAD("basic2.rom",  0x0c000, 0x4000, 0x79434781 ) /* rom page 3  0c000 */
ROM_END


/*	0000- 7fff	ram */
/*	8000- bfff	not used, this area is mapped over with one of the roms at 10000 and above */
/*	c000- ffff	OS rom and memory mapped hardware at fc00-feff */
/* 10000-4ffff	16 paged rom banks mapped back into 8000-bfff by the page rom select */


ROM_START(bbcb)
	ROM_REGION(0x08000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x44000,REGION_USER1,0) /* ROM */
	ROM_LOAD("os12.rom", 0x40000,0x4000, 0x3c14fc70)


	                                                      /* rom page 0  00000 */
	                                                      /* rom page 1  04000 */
	                                                      /* rom page 2  08000 */
	                                                      /* rom page 3  0c000 */
	                                                      /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */


	/* just use one of the following DFS roms */

	/* dnfs is acorns disc and network filing system rom it replaced dfs 0.9  */
//	ROM_LOAD("dnfs.rom",    0x38000, 0x4000, 0x8ccd2157 ) /* rom page 14 38000 */

	/* dfs 0.9 was the standard acorn dfs rom before it was replaced with the dnfs rom */
//	ROM_LOAD("dfs09.rom",   0x38000, 0x2000, 0x3ce609cf ) /* rom page 14 38000 */
//	ROM_RELOAD(             0x3a000, 0x2000             )

	/* dfs 1.44 from watford electronics, this is the best of the non-acorn dfs roms */
	ROM_LOAD("dfs144.rom",  0x38000, 0x4000, 0x9fb8d13f ) /* rom page 14 38000 */

	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 15 3c000 */


ROM_END



ROM_START(bbcb1770)
	ROM_REGION(0x08000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x44000,REGION_USER1,0) /* ROM */
	ROM_LOAD("os12.rom", 0x40000,0x4000, 0x3c14fc70)

	                                                      /* rom page 0  00000 */
	                                                      /* rom page 1  04000 */
	                                                      /* rom page 2  08000 */
	                                                      /* rom page 3  0c000 */
	                                                      /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */

/* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 15 3c000 */

ROM_END




ROM_START(bbcbp)
	ROM_REGION(0x10000,REGION_CPU1,0) /* ROM MEMORY */

	ROM_REGION(0x44000,REGION_USER1,0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, 0x9f356396 )  /* basic rom */
	ROM_CONTINUE(           0x40000, 0x4000)  /* OS */

	                                                      /* rom page 0  00000 */
	                                                      /* rom page 1  04000 */
	                                                      /* rom page 2  08000 */
	                                                      /* rom page 3  0c000 */
	                                                      /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */

    /* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

ROM_END


ROM_START(bbcbp128)
	ROM_REGION(0x10000,REGION_CPU1,0) /* ROM MEMORY */

	ROM_REGION(0x44000,REGION_USER1,0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, 0x9f356396 )  /* basic rom */
	ROM_CONTINUE(           0x40000, 0x4000)  /* OS */

	                                                      /* rom page 0  00000 */
	                                                      /* rom page 1  04000 */
	                                                      /* rom page 2  08000 */
	                                                      /* rom page 3  0c000 */
	                                                      /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */

    /* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

ROM_END



ROM_START(bbcb6502)
	ROM_REGION(0x08000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x44000,REGION_USER1,0) /* ROM */
	ROM_LOAD("os12.rom", 0x40000,0x4000, 0x3c14fc70)

	                                                      /* rom page 0  00000 */
	                                                      /* rom page 1  04000 */
	                                                      /* rom page 2  08000 */
	                                                      /* rom page 3  0c000 */
	                                                      /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */

/* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 15 3c000 */

	ROM_REGION(0x11000,REGION_CPU2,0)
	ROM_LOAD("6502tube.rom" , 0x10000,0x1000,0x98b5fe42)

ROM_END



static int bbcb_vsync(void)
{
	via_0_ca1_w(0,1);
	via_0_ca1_w(0,0);
	bbc_frameclock();
	return 0;
}


static struct SN76496interface sn76496_interface =
{
	1,		/* 1 chip */
	{ 4000000 },	/* 4Mhz */
	{ 100 }
};

static struct MachineDriver machine_driver_bbca =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbca,
			writemem_bbca,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, 128,
	1,
	init_machine_bbca, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_starta,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcb =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcb,
			writemem_bbcb,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, 128,
	1,
	init_machine_bbcb, /* init_machine */
	stop_machine_bbcb, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startb,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcb1770 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcb1770,
			writemem_bbcb1770,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, 128,
	1,
	init_machine_bbcb1770, /* init_machine */
	stop_machine_bbcb1770, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startb,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcbp =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcbp,
			writemem_bbcbp,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, 128,
	1,
	init_machine_bbcbp, /* init_machine */
	stop_machine_bbcbp, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startbp,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcbp128 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcbp128,
			writemem_bbcbp128,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, 128,
	1,
	init_machine_bbcbp, /* init_machine */
	stop_machine_bbcbp, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startbp,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



static struct MachineDriver machine_driver_bbcb6502 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcbtube,
			writemem_bbcbtube,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		},
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbc6502,
			writemem_bbc6502,
			0,
			0,
		}

	},
	50, 128,
	1,
	init_machine_bbcb6502, /* init_machine */
	stop_machine_bbcb6502, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0, 	/* screen refresh interrupts */
	bbc_vh_startb,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static const struct IODevice io_bbca[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

static const struct IODevice io_bbcb[] = {
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},    {
        IO_CARTSLOT,            /* type */
        4,                      /* count */
        "rom\0",                /* file extensions */
        IO_RESET_NONE,          /* reset if file changed */
        0,
        bbcb_load_rom,          /* init */
        NULL,                   /* exit */
        NULL,                   /* info */
        NULL,                   /* open */
        NULL,                   /* close */
        NULL,                   /* status */
        NULL,                   /* seek */
        NULL,                   /* tell */
        NULL,                   /* input */
        NULL,                   /* output */
        NULL,                   /* input_chunk */
        NULL                    /* output_chunk */
    },
	{ IO_END }
};


static const struct IODevice io_bbcb1770[] = {
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},    {
        IO_CARTSLOT,            /* type */
        4,                      /* count */
        "rom\0",                /* file extensions */
        IO_RESET_NONE,          /* reset if file changed */
        0,
        bbcb_load_rom,          /* init */
        NULL,                   /* exit */
        NULL,                   /* info */
        NULL,                   /* open */
        NULL,                   /* close */
        NULL,                   /* status */
        NULL,                   /* seek */
        NULL,                   /* tell */
        NULL,                   /* input */
        NULL,                   /* output */
        NULL,                   /* input_chunk */
        NULL                    /* output_chunk */
    },
	{ IO_END }
};


static const struct IODevice io_bbcbp[] = {
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},    {
        IO_CARTSLOT,            /* type */
        4,                      /* count */
        "rom\0",                /* file extensions */
        IO_RESET_NONE,          /* reset if file changed */
        0,
        bbcb_load_rom,          /* init */
        NULL,                   /* exit */
        NULL,                   /* info */
        NULL,                   /* open */
        NULL,                   /* close */
        NULL,                   /* status */
        NULL,                   /* seek */
        NULL,                   /* tell */
        NULL,                   /* input */
        NULL,                   /* output */
        NULL,                   /* input_chunk */
        NULL                    /* output_chunk */
    },
	{ IO_END }
};

static const struct IODevice io_bbcbp128[] = {
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},    {
        IO_CARTSLOT,            /* type */
        4,                      /* count */
        "rom\0",                /* file extensions */
        IO_RESET_NONE,          /* reset if file changed */
        0,
        bbcb_load_rom,          /* init */
        NULL,                   /* exit */
        NULL,                   /* info */
        NULL,                   /* open */
        NULL,                   /* close */
        NULL,                   /* status */
        NULL,                   /* seek */
        NULL,                   /* tell */
        NULL,                   /* input */
        NULL,                   /* output */
        NULL,                   /* input_chunk */
        NULL                    /* output_chunk */
    },
	{ IO_END }
};



static const struct IODevice io_bbcb6502[] = {
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};



/*	   year name	parent	machine  input	init	company */
COMP (1981,bbca,	0,		bbca,	 bbca,	0,	"Acorn","BBC Micro Model A" )
COMP (1981,bbcb,	bbca,	bbcb,	 bbca,	0,	"Acorn","BBC Micro Model B" )
COMP (1981,bbcb1770,bbca,	bbcb1770,bbca,	0,	"Acorn","BBC Micro Model B with WD1770 disc controller" )
COMP (1985,bbcbp,   bbca,	bbcbp   ,bbca,  0,  "Acorn","BBC Micro Model B+ 64K" )
COMP (1985,bbcbp128,bbca,   bbcbp128,bbca,  0,  "Acorn","BBC Micro Model B+ 128k" )
/*
COMP (198?,bbcm,    0,      bbcm    ,bbcm,  0,  "Acorn","BBC Master" )
*/



// This bbc6502 is the BBC second processor upgrade
// I have just added the second processor on its own right now until I work out how it works.
// then it will get connected up to another BBC driver
// The code for this second processor upgrade is more or less complete here now.
// It just needs a TUBE driver made to connect it to the BBC

COMP (198?,bbcb6502,bbca,      bbcb6502,bbca,  0,  "Acorn","BBC Micro Model B with a 6502 Second Processor")

