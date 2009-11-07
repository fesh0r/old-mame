/******************************************************************************
    BBC Model A,B

    BBC Model B Plus

    MESS Driver By:

    Gordon Jefferyes
    mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/mc146818.h"
#include "includes/bbc.h"
#include "machine/upd7002.h"
#include "machine/ctronics.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/uef_cas.h"
#include "formats/csw_cas.h"
#include "sound/sn76496.h"


/******************************************************************************
A= BBC Model A

                    A   B   B+  M
+   &0000           +   +   +   +
|                   |   |   |   |
|   &1000           |   |   |   |
|1                  |   |   |1  |1
|   &2000           |1  |   |   |
|                   |   |   |   |
+   &3000           |   |   +   +
|2                  |   |   |   |
+   &4000           +   |1  |   |
|                   |   |   |   |
|   &5000           |   |   |   |
|                   |   |   |2  |2
|3  &6000           |3  |   |   |
|                   |   |   |   |
|   &7000           |   |   |   |
|                   |   |   |   |
+   &8000           +   +   +   +
|4                  |   |   |   |4
+   &9000           |   |   |   +
|                   |   |   |4  |
|5  &A000           |4  |4  |   |5
|                   |   |   |   |
+   &B000           |   |   +   |
|6                  |   |   |6  |
+   &C000           +   +   +   +
|                   |   |   |   |7
|7  &D000           |   |   |   |
|                   |   |   |   |
+   &E000           |7  |7  |7  +
|                   |   |   |   |
|8  &F000           |   |   |   |8
|                   |   |   |   |
+   &FC00 FRED      +   +   +   +
    &FD00 JIM       +   +   +   +
    &FE00 SHEILA    +   +   +   +
+   &FF00           +   +   +   +
|9                  |9  |9  |9  |9
+   &FFFF           +   +   +   +



&00-&07 6845 CRTC       Video controller                8  ( 2 bytes x  4 ) 1MHz
&08-&0F 6850 ACIA       Serial controller               8  ( 2 bytes x  4 ) 1MHz
&10-&17 Serial ULA      Serial system chip              8  ( 1 byte  x  8 ) 1MHz
&18-&1f INTOFF/STATID   ECONET Interrupt Off / ID No.   8  ( 1 byte  x  8 ) 1MHz
write:
&20-&2F Video ULA       Video system chip               16 ( 2 bytes x  8 ) 2MHz
&30-&3F 74LS161         Paged ROM selector              16 ( 1 byte  x 16 ) 2MHz
read:
&20-&2F INTON           ECONET Interrupt On             16 ( 1 bytes x 16 ) 2MHz
&30-&3F Not Connected   Not Connected                                       2MHz

&40-&5F 6522 VIA        SYSTEM VIA                      32 (16 bytes x  2 ) 1MHz
&60-&7F 6522 VIA        USER VIA                        32 (16 bytes x  2 ) 1MHz
&80-&9F 8271 FDC        FDC Floppy disc controller      32 ( 8 bytes x  4 ) 2MHz
&A0-&BF 68B54 ADLC      ECONET controller               32 ( 4 bytes x  8 ) 2MHz
&C0-&DF uPD7002         Analogue to digital converter   32 ( 4 bytes x  8 ) 1MHz
&E0-&FF Tube ULA        Tube system interface           32 (32 bytes x  1 ) 2MHz
******************************************************************************/

static READ8_HANDLER( bbc_fe_r )
{
	return 0xfe;
}

static ADDRESS_MAP_START( bbca_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH											/*  Hardware marked with a 1 is not present in a Model A        */

	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(SMH_BANK(1)		, bbc_memorya1_w      	)	/*    0000-3fff                 Regular Ram                     */
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(SMH_BANK(3)		, bbc_memorya1_w      	)	/*    4000-7fff                 Repeat of the Regular Ram       */

	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(SMH_BANK(4)		, SMH_ROM      	)	/*    8000-bfff                 Paged ROM                       */

	AM_RANGE(0xc000, 0xfbff) AM_READWRITE(SMH_BANK(7)    	, SMH_ROM      	)	/*    c000-fbff                 OS ROM                          */

	AM_RANGE(0xfc00, 0xfdff) AM_NOP	/*    fc00-fdff                 FRED & JIM Pages                */

																					/*    fe00-feff                 Shiela Address Page             */
	AM_RANGE(0xfe00, 0xfe07) AM_READWRITE(BBC_6845_r    	, BBC_6845_w		)	/*    fe00-fe07  6845 CRTA      Video controller                */
	AM_RANGE(0xfe08, 0xfe08) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xfe09, 0xfe09) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xfe10, 0xfe17) AM_READWRITE(bbc_fe_r		, BBC_SerialULA_w   )	/*    fe10-fe17  Serial ULA     Serial system chip              */
	AM_RANGE(0xfe18, 0xfe1f) AM_NOP													/*    fe18-fe1f  INTOFF/STATID  1 ECONET Interrupt Off / ID No. */
	AM_RANGE(0xfe20, 0xfe2f) AM_WRITE	 (              	  bbc_videoULA_w    )	/* R: fe20-fe2f  INTON          1 ECONET Interrupt On           */
																					/* W: fe20-fe2f  Video ULA      Video system chip               */
	AM_RANGE(0xfe30, 0xfe3f) AM_READWRITE(bbc_fe_r    	, bbc_page_selecta_w	)	/* R: fe30-fe3f  NC             Not Connected                   */
																					/* W: fe30-fe3f  84LS161        Paged ROM selector              */
	AM_RANGE(0xfe40, 0xfe5f) AM_DEVREADWRITE("via6522_0", via_r, via_w)	/*    fe40-fe5f  6522 VIA       SYSTEM VIA                      */
	AM_RANGE(0xfe60, 0xfe7f) AM_NOP													/*    fe60-fe7f  6522 VIA       1 USER VIA                      */
	AM_RANGE(0xfe80, 0xfe9f) AM_NOP													/*    fe80-fe9f  8271/1770 FDC  1 Floppy disc controller        */
	AM_RANGE(0xfea0, 0xfebf) AM_READ     (bbc_fe_r    	                	)	/*    fea0-febf  68B54 ADLC     1 ECONET controller             */
	AM_RANGE(0xfec0, 0xfedf) AM_NOP													/*    fec0-fedf  uPD7002        1 Analogue to digital converter */
	AM_RANGE(0xfee0, 0xfeff) AM_READ     (bbc_fe_r    	                	)	/*    fee0-feff  Tube ULA       1 Tube system interface         */

	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION("user1", 0x13f00)				/*    ff00-ffff                 OS Rom (continued)              */
ADDRESS_MAP_END


static ADDRESS_MAP_START( bbcb_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(SMH_BANK(1)		, bbc_memorya1_w      	)	/*    0000-3fff                 Regular Ram                     */
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(SMH_BANK(3)		, bbc_memoryb3_w      	)	/*    4000-7fff                 Repeat of the Regular Ram       */


	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(SMH_BANK(4)    	, bbc_memoryb4_w      	)	/*    8000-bfff                 Paged ROM                       */

	AM_RANGE(0xc000, 0xfbff) AM_READWRITE(SMH_BANK(7)    	, SMH_ROM       	)	/*    c000-fbff                 OS ROM                          */

	AM_RANGE(0xfc00, 0xfdff) AM_READWRITE(bbc_opus_read     , bbc_opus_write	)	/*    fc00-fdff                 OPUS Disc Controller            */

																					/*    fe00-feff                 Shiela Address Page             */
	AM_RANGE(0xfe00, 0xfe07) AM_READWRITE(BBC_6845_r    	, BBC_6845_w     	)	/*    fe00-fe07  6845 CRTC      Video controller                */
	AM_RANGE(0xfe08, 0xfe08) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xfe09, 0xfe09) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xfe10, 0xfe17) AM_READWRITE(bbc_fe_r		, BBC_SerialULA_w   )	/*    fe10-fe17  Serial ULA     Serial system chip              */
	AM_RANGE(0xfe18, 0xfe1f) AM_NOP													/*    fe18-fe1f  INTOFF/STATID  ECONET Interrupt Off / ID No.   */
	AM_RANGE(0xfe20, 0xfe2f) AM_WRITE    (              	  bbc_videoULA_w    )	/* R: fe20-fe2f  INTON          ECONET Interrupt On             */
																					/* W: fe20-fe2f  Video ULA      Video system chip               */
	AM_RANGE(0xfe30, 0xfe3f) AM_READWRITE(bbc_fe_r    	, bbc_page_selectb_w 	)	/* R: fe30-fe3f  NC             Not Connected                   */
																					/* W: fe30-fe3f  84LS161        Paged ROM selector              */
	AM_RANGE(0xfe40, 0xfe5f) AM_DEVREADWRITE("via6522_0", via_r, via_w)	/*    fe40-fe5f  6522 VIA       SYSTEM VIA                      */
	AM_RANGE(0xfe60, 0xfe7f) AM_DEVREADWRITE("via6522_1", via_r, via_w)	/*    fe60-fe7f  6522 VIA       USER VIA                        */
	AM_RANGE(0xfe80, 0xfe9f) AM_READWRITE(bbc_disc_r		, bbc_disc_w		)	/*    fe80-fe9f  8271 FDC       Floppy disc controller          */
	AM_RANGE(0xfea0, 0xfebf) AM_READ	 (bbc_fe_r    	                 	)	/*    fea0-febf  68B54 ADLC     ECONET controller               */
	AM_RANGE(0xfec0, 0xfedf) AM_DEVREADWRITE("upd7002",uPD7002_r,uPD7002_w	)	/*    fec0-fedf  uPD7002        Analogue to digital converter   */
	AM_RANGE(0xfee0, 0xfeff) AM_READ	 (bbc_fe_r						 	)	/*    fee0-feff  Tube ULA       Tube system interface           */

	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION("user1", 0x43f00)				/*    ff00-ffff                 OS Rom (continued)              */
ADDRESS_MAP_END


static ADDRESS_MAP_START( bbcbp_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x0000, 0x2fff) AM_READWRITE(SMH_BANK(1)		, bbc_memorybp1_w		)	/*    0000-2fff                 Regular Ram                     */

	AM_RANGE(0x3000, 0x7fff) AM_READWRITE(SMH_BANK(2)		, bbc_memorybp2_w		)	/*    3000-7fff                 Video/Shadow Ram                */

	AM_RANGE(0x8000, 0xafff) AM_READWRITE(SMH_BANK(4)		, bbc_memorybp4_w		)	/*    8000-afff                 Paged ROM or 12K of RAM         */
	AM_RANGE(0xb000, 0xbfff) AM_READWRITE(SMH_BANK(6)		, SMH_ROM			)	/*    b000-bfff                 Rest of paged ROM area          */

	AM_RANGE(0xc000, 0xfbff) AM_READWRITE(SMH_BANK(7)		, SMH_ROM			)	/*    c000-fbff                 OS ROM                          */

	AM_RANGE(0xfc00, 0xfdff) AM_NOP	/*    fc00-fdff                 FRED & JIM Pages                */

																					/*    fe00-feff                 Shiela Address Page             */
	AM_RANGE(0xfe00, 0xfe07) AM_READWRITE(BBC_6845_r		, BBC_6845_w		)	/*    fe00-fe07  6845 CRTC      Video controller                */
	AM_RANGE(0xfe08, 0xfe08) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xfe09, 0xfe09) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xfe10, 0xfe17) AM_READWRITE(bbc_fe_r		, BBC_SerialULA_w   )	/*    fe10-fe17  Serial ULA     Serial system chip              */
	AM_RANGE(0xfe18, 0xfe1f) AM_NOP													/*    fe18-fe1f  INTOFF/STATID  ECONET Interrupt Off / ID No.   */
	AM_RANGE(0xfe20, 0xfe2f) AM_WRITE	 (bbc_videoULA_w						)	/* R: fe20-fe2f  INTON          ECONET Interrupt On             */
																					/* W: fe20-fe2f  Video ULA      Video system chip               */
	AM_RANGE(0xfe30, 0xfe3f) AM_READWRITE(bbc_fe_r		, bbc_page_selectbp_w	)	/* R: fe30-fe3f  NC             Not Connected                   */
																					/* W: fe30-fe3f  84LS161        Paged ROM selector              */
	AM_RANGE(0xfe40, 0xfe5f) AM_DEVREADWRITE("via6522_0", via_r, via_w)	/*    fe40-fe5f  6522 VIA       SYSTEM VIA                      */
	AM_RANGE(0xfe60, 0xfe7f) AM_DEVREADWRITE("via6522_1", via_r, via_w)	/*    fe60-fe7f  6522 VIA       USER VIA                        */
	AM_RANGE(0xfe80, 0xfe9f) AM_READWRITE(bbc_wd1770_read	, bbc_wd1770_write	)	/*    fe80-fe9f  1770 FDC       Floppy disc controller          */
	AM_RANGE(0xfea0, 0xfebf) AM_READ     (bbc_fe_r							)	/*    fea0-febf  68B54 ADLC     ECONET controller               */
	AM_RANGE(0xfec0, 0xfedf) AM_DEVREADWRITE("upd7002", uPD7002_r, uPD7002_w)	/*    fec0-fedf  uPD7002        Analogue to digital converter   */
	AM_RANGE(0xfee0, 0xfeff) AM_READ	 (bbc_fe_r							)	/*    fee0-feff  Tube ULA       Tube system interface           */

	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION("user1", 0x43f00)				/*    ff00-ffff                 OS Rom (continued)              */
ADDRESS_MAP_END



static ADDRESS_MAP_START( bbcbp128_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x0000, 0x2fff) AM_READWRITE(SMH_BANK(1)		, bbc_memorybp1_w		)	/*    0000-2fff                 Regular Ram                     */

	AM_RANGE(0x3000, 0x7fff) AM_READWRITE(SMH_BANK(2)		, bbc_memorybp2_w		)	/*    3000-7fff                 Video/Shadow Ram                */

	AM_RANGE(0x8000, 0xafff) AM_READWRITE(SMH_BANK(4)		, bbc_memorybp4_128_w	)	/*    8000-afff                 Paged ROM or 12K of RAM         */
	AM_RANGE(0xb000, 0xbfff) AM_READWRITE(SMH_BANK(6)		, bbc_memorybp6_128_w	)	/*    b000-bfff                 Rest of paged ROM area          */

	AM_RANGE(0xc000, 0xfbff) AM_READWRITE(SMH_BANK(7)		, SMH_ROM			)	/*    c000-fbff                 OS ROM                          */

	AM_RANGE(0xfc00, 0xfdff) AM_NOP	/*    fc00-fdff                 FRED & JIM Pages                */

																					/*    fe00-feff                 Shiela Address Page             */
	AM_RANGE(0xfe00, 0xfe07) AM_READWRITE(BBC_6845_r		, BBC_6845_w		)	/*    fe00-fe07  6845 CRTC      Video controller                */
	AM_RANGE(0xfe08, 0xfe08) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xfe09, 0xfe09) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xfe10, 0xfe17) AM_READWRITE(bbc_fe_r		, BBC_SerialULA_w   )	/*    fe10-fe17  Serial ULA     Serial system chip              */
	AM_RANGE(0xfe10, 0xfe17) AM_NOP													/*    fe10-fe17  Serial ULA     Serial system chip              */
	AM_RANGE(0xfe18, 0xfe1f) AM_NOP													/*    fe18-fe1f  INTOFF/STATID  ECONET Interrupt Off / ID No.   */
	AM_RANGE(0xfe20, 0xfe2f) AM_WRITE	 (bbc_videoULA_w						)	/* R: fe20-fe2f  INTON          ECONET Interrupt On             */
																					/* W: fe20-fe2f  Video ULA      Video system chip               */
	AM_RANGE(0xfe30, 0xfe3f) AM_READWRITE(bbc_fe_r		, bbc_page_selectbp_w	)	/* R: fe30-fe3f  NC             Not Connected                   */
																					/* W: fe30-fe3f  84LS161        Paged ROM selector              */
	AM_RANGE(0xfe40, 0xfe5f) AM_DEVREADWRITE("via6522_0", via_r, via_w)	/*    fe40-fe5f  6522 VIA       SYSTEM VIA                      */
	AM_RANGE(0xfe60, 0xfe7f) AM_DEVREADWRITE("via6522_1", via_r, via_w)	/*    fe60-fe7f  6522 VIA       USER VIA                        */
	AM_RANGE(0xfe80, 0xfe9f) AM_READWRITE(bbc_wd1770_read	, bbc_wd1770_write	)	/*    fe80-fe9f  1770 FDC       Floppy disc controller          */
	AM_RANGE(0xfea0, 0xfebf) AM_READ     (bbc_fe_r							)	/*    fea0-febf  68B54 ADLC     ECONET controller               */
	AM_RANGE(0xfec0, 0xfedf) AM_DEVREADWRITE("upd7002",uPD7002_r, uPD7002_w	)	/*    fec0-fedf  uPD7002        Analogue to digital converter   */
	AM_RANGE(0xfee0, 0xfeff) AM_READ	 (bbc_fe_r							)	/*    fee0-feff  Tube ULA       Tube system interface           */

	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION("user1", 0x43f00)				/*    ff00-ffff                 OS Rom (continued)              */
ADDRESS_MAP_END



/******************************************************************************
&FC00-&FCFF FRED
&FD00-&FDFF JIM
&FE00-&FEFF SHEILA      Read                    Write
&00-&07 6845 CRTC       Video controller        Video Controller         8 ( 2 bytes x  4 )
&08-&0F 6850 ACIA       Serial controller       Serial Controller        8 ( 2 bytes x  4 )
&10-&17 Serial ULA      -                       Serial system chip       8 ( 1 byte  x  8 )
&18-&1F uPD7002         A to D converter        A to D converter         8 ( 4 bytes x  2 )

&20-&23 Video ULA       -                       Video system chip        4 ( 2 bytes x  2 )
&24-&27 FDC Latch       1770 Control latch      1770 Control latch       4 ( 1 byte  x  4 )
&28-&2F 1770 registers  1770 Disc Controller    1170 Disc Controller     8 ( 4 bytes x  2 )
&30-&33 ROMSEL          -                       ROM Select               4 ( 1 byte  x  4 )
&34-&37 ACCCON          ACCCON select reg.      ACCCON select reg        4 ( 1 byte  x  4 )
&38-&3F NC              -                       -
&40-&5F 6522 VIA        SYSTEM VIA              SYSTEM VIA              32 (16 bytes x  2 ) 1MHz
&60-&7F 6522 VIA        USER VIA                USER VIA                32 (16 bytes x  2 ) 1MHz
&80-&9F Int. Modem      Int. Modem              Int Modem
&A0-&BF 68B54 ADLC      ECONET controller       ECONET controller       32 ( 4 bytes x  8 ) 2MHz
&C0-&DF NC              -                       -
&E0-&FF Tube ULA        Tube system interface   Tube system interface   32 (32 bytes x  1 ) 2MHz
******************************************************************************/


static ADDRESS_MAP_START(bbcm_mem, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x2fff) AM_READWRITE(SMH_BANK(1)		, bbc_memorybm1_w		)	/*    0000-2fff                 Regular Ram                     */

	AM_RANGE(0x3000, 0x7fff) AM_READWRITE(SMH_BANK(2)		, bbc_memorybm2_w		)	/*    3000-7fff                 Video/Shadow Ram                */

	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(SMH_BANK(4)		, bbc_memorybm4_w		)	/*    8000-8fff                 Paged ROM/RAM or 4K of RAM ANDY */
	AM_RANGE(0x9000, 0xbfff) AM_READWRITE(SMH_BANK(5)		, bbc_memorybm5_w		)	/*    9000-bfff                 Rest of paged ROM/RAM area      */

	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(SMH_BANK(7)		, bbc_memorybm7_w		)	/*    c000-dfff                 OS ROM or 8K of RAM       HAZEL */
	AM_RANGE(0xe000, 0xfbff) AM_ROM AM_REGION("user1", 0x42000)				/*    e000-fbff                 OS ROM                          */

	AM_RANGE(0xfc00, 0xfeff) AM_READWRITE(SMH_BANK(8)			, bbcm_w			)   /*    this is now processed directly because it can be ROM or hardware */

	//AM_RANGE(0xfc00, 0xfeff) AM_READWRITE(bbcm_r          , bbcm_w            )   /*    this is now processed directly because it can be ROM or hardware */
	/*
    AM_RANGE(0xfc00, 0xfdff) AM_READWRITE(SMH_BANK(2)        , SMH_ROM          )       fc00-fdff                   FRED & JIM Pages

                                                                                          fe00-feff                 Shiela Address Page
    AM_RANGE(0xfe00, 0xfe07) AM_READWRITE(BBC_6845_r        , BBC_6845_w        )         fe00-fe07  6845 CRTC      Video controller
    AM_RANGE(0xfe08, 0xfe08) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_stat_r, acia6850_ctrl_w)
    AM_RANGE(0xfe09, 0xfe09) AM_MIRROR(0x06) AM_DEVREADWRITE("acia6850", acia6850_data_r, acia6850_data_w)
    AM_RANGE(0xfe10, 0xfe17) AM_NOP                                                       fe10-fe17  Serial ULA     Serial system chip
    AM_RANGE(0xfe18, 0xfe1f) M_DEVREADWRITE(UPD7002, "uPD7002",uPD7002_r, uPD7002_w )         fec0-fedf  uPD7002        Analogue to digital converter
    AM_RANGE(0xfe20, 0xfe23) AM_READWRITE(return8_FE        , bbc_videoULA_w        )         fe20-fe23  Video ULA      Video system chip
    AM_RANGE(0xfe24, 0xfe27) AM_READWRITE(bbcm_wd1770l_read , bbcm_wd1770l_write)         fe24-fe27  1770 Latch     1770 Control Latch
    AM_RANGE(0xfe28, 0xfe2f) AM_READWRITE(bbcm_wd1770_read  , bbcm_wd1770_write )         fe28-fe2f  1770 FDC       Floppy disc controller
    AM_RANGE(0xfe30, 0xfe33) AM_READWRITE(return8_FE        , bbc_page_selectbp_w   )         fe30-fe33  84LS161        Paged ROM selector
    AM_RANGE(0xfe34, 0xfe37) AM_READWRITE(bbcm_ACCCON_read  , bbcm_ACCCON_write )         fe34-fe37  ACCCON         ACCCON select Latch
    AM_RANGE(0xfe38, 0xfe3f) AM_READ     (return8_FE                            )         fe38-fe3f  NC
    AM_RANGE(0xfe40, 0xfe5f) AM_DEVREADWRITE("via6522_0", via_r, via_w)          fe40-fe5f  6522 VIA       SYSTEM VIA
    AM_RANGE(0xfe60, 0xfe7f) AM_DEVREADWRITE("via6522_1", via_r, via_w)          fe60-fe7f  6522 VIA       USER VIA
    AM_RANGE(0xfe80, 0xfe9f) AM_READ     (return8_FE                            )         fe80-fe9f  NC
    AM_RANGE(0xfea0, 0xfebf) AM_READ     (return8_FE                            )         fea0-febf  68B54 ADLC     ECONET controller
    AM_RANGE(0xfec0, 0xfedf) AM_READ     (return8_FE                            )         fec0-fedf  NC
    AM_RANGE(0xfee0, 0xfeff) AM_READ     (return8_FE                            )         fee0-feff  Tube ULA       Tube system interface
    */

	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION("user1", 0x43f00)				/*    ff00-ffff                 OS Rom (continued)              */
ADDRESS_MAP_END



static const rgb_t bbc_palette[8]=
{
	MAKE_RGB(0x0ff,0x0ff,0x0ff),
	MAKE_RGB(0x000,0x0ff,0x0ff),
	MAKE_RGB(0x0ff,0x000,0x0ff),
	MAKE_RGB(0x000,0x000,0x0ff),
	MAKE_RGB(0x0ff,0x0ff,0x000),
	MAKE_RGB(0x000,0x0ff,0x000),
	MAKE_RGB(0x0ff,0x000,0x000),
	MAKE_RGB(0x000,0x000,0x000)
};

static PALETTE_INIT( bbc )
{
	palette_set_colors(machine, 0, bbc_palette, ARRAY_LENGTH(bbc_palette));
}

/* 2008-05 FP:
Small note about natural keyboard support: currently,
- "Copy" is mapped to 'F11'
- "Shift Lock" is mapped to 'F12'                      */

static INPUT_PORTS_START(bbca)
	PORT_START("COL0")	/* KEYBOARD COLUMN 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Q)									PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0")			PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)									PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK)								PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK")	PORT_CODE(KEYCODE_LALT)			PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)									PORT_CHAR('\t')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESCAPE")		PORT_CODE(KEYCODE_ESC)			PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("COL1")	/* KEYBOARD COLUMN 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")		PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('W')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('A')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('S')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('Z')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("COL2")	/* KEYBOARD COLUMN 2 */
	PORT_DIPNAME(0x01, 0x01, "DIP 8 (Not Used)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('X')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('C')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F2))

	PORT_START("COL3")	/* KEYBOARD COLUMN 3 */
	PORT_DIPNAME(0x01, 0x01, "DIP 7 (Not Used)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('T')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('F')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('G')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("COL4")	/* KEYBOARD COLUMN 4 */
	PORT_DIPNAME(0x01, 0x01, "DIP 6 (Disc Speed 1)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('H')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('B')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F5))

	PORT_START("COL5")	/* KEYBOARD COLUMN 5 */
	PORT_DIPNAME(0x01, 0x01, "DIP 5 (Disc Speed 0)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('J')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('N')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("COL6")	/* KEYBOARD COLUMN 6 */
	PORT_DIPNAME(0x01, 0x01, "DIP 4 (Shift Break)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('O')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)	PORT_CODE(KEYCODE_K)			PORT_CHAR('K')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('L')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("COL7")	/* KEYBOARD COLUMN 7 */
	PORT_DIPNAME(0x01, 0x01, "DIP 3 (Mode bit 2)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('@')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(F9))

	PORT_START("COL8")	/* KEYBOARD COLUMN 8 */
	PORT_DIPNAME(0x01, 0x01, "DIP 2 (Mode bit 1)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('_') PORT_CHAR('\xa3')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START("COL9")	/* KEYBOARD COLUMN 9 */
	PORT_DIPNAME(0x01, 0x01, "DIP 1 (Mode bit 0)")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)			PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)			PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DELETE")		PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("COPY")		PORT_CODE(KEYCODE_END)			PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	/* Keyboard columns 10 -> 15 are reserved for BBC Master */
	PORT_START("COL10")
	PORT_START("COL11")
	PORT_START("COL12")
	PORT_START("COL13")
	PORT_START("COL14")
	PORT_START("COL15")

	PORT_START("IN0")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START("JOY0")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0x0,0xff ) PORT_PLAYER(1)

	PORT_START("JOY1")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0x0,0xff ) PORT_PLAYER(1)

	PORT_START("JOY2")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0x0,0xff ) PORT_PLAYER(2)

	PORT_START("JOY3")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0x0,0xff ) PORT_PLAYER(2)

	PORT_START("BBCCONFIG")
	PORT_DIPNAME( 0x07, 0x00, "DFS SELECT" )
	PORT_DIPSETTING(    0x00, "Acorn DFS 0.90 (read only)"  )
	PORT_DIPSETTING(    0x01, "Acorn DNFS 1.20 (read only)" )
	PORT_DIPSETTING(    0x02, "Watford DFS 1.44 (read only)" )
	PORT_DIPSETTING(    0x03, "Acorn DFS E00 (hack / read only)" )
	PORT_DIPSETTING(    0x04, "Acorn DDFS" )
	PORT_DIPSETTING(    0x05, "Watford DDFS (not working)" )
	PORT_DIPSETTING(    0x06, "Opus Challenger 512K (ram drive only)" )
	PORT_DIPSETTING(    0x07, DEF_STR( None ) )

	PORT_DIPNAME( 0x18, 0x00, "Sideways RAM Type" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x08, "Solidisk 128K (fe62)" )
	PORT_DIPSETTING(    0x10, "Acorn 64K (fe30)" )
	PORT_DIPSETTING(    0x18, "Acorn 128K (fe30)" )

	PORT_DIPNAME( 0x20, 0x20, "Main Ram Size" )
	PORT_DIPSETTING(    0x00, "16K" )
	PORT_DIPSETTING(    0x20, "32K" )
INPUT_PORTS_END


/* the BBC came with 4 rom sockets on the mother board as shown in the model A driver */
/* you could get a number of rom upgrade boards that took this up to 16 roms as in the */
/* model B driver */

ROM_START(bbca)
	ROM_REGION(0x04000,"maincpu",ROMREGION_ERASEFF) /* RAM */

	ROM_REGION(0x14000,"user1",0) /* ROM */
	ROM_LOAD("os12.rom",    0x10000,  0x4000, CRC(3c14fc70) SHA1(0d9bcaf6a393c9ce2359ed700ddb53c232c2c45d))

														  /* rom page 0  00000 */
														  /* rom page 1  04000 */
														  /* rom page 2  08000 */
	ROM_LOAD("basic2.rom",  0x0c000, 0x4000, CRC(79434781) SHA1(4a7393f3a45ea309f744441c16723e2ef447a281)) /* rom page 3  0c000 */
ROM_END


/*  0000- 7fff  ram */
/*  8000- bfff  not used, this area is mapped over with one of the roms at 10000 and above */
/*  c000- ffff  OS rom and memory mapped hardware at fc00-feff */
/* 10000-4ffff  16 paged rom banks mapped back into 8000-bfff by the page rom select */


ROM_START(bbcb)
	ROM_REGION(0x08000,"maincpu",ROMREGION_ERASEFF) /* RAM */

	ROM_REGION(0x44000,"user1",0) /* ROM */

	ROM_LOAD("os12.rom", 0x40000,0x4000, CRC(3c14fc70) SHA1(0d9bcaf6a393c9ce2359ed700ddb53c232c2c45d))

	// usos12.rom is the USA version of the OS. acorn tried to release the BBC B in the USA calling it the
	// "Acorn Proton", it failed to sell in the USA and was withdrawn from the market.
	// the main difference is the screen resolution setting the display to work on American TV's
	//ROM_LOAD("usos12.rom", 0x40000,0x4000, CRC(c8e946a9) )


	ROM_LOAD("basic2.rom",  0x00000, 0x4000, CRC(79434781) SHA1(4a7393f3a45ea309f744441c16723e2ef447a281)) /* rom page 15 3c000 */
	//ROM_LOAD("speech-1.0.rom",  0x08000, 0x2000, CRC(e63f7fb7) )
	//ROM_RELOAD(                 0x0a000, 0x2000                )
	//ROM_LOAD("dfs144.rom",  0x04000, 0x4000, CRC(9fb8d13f) SHA1(387d2468c6e1360f5b531784ce95d5f71a50c2b5)) /* rom page 14 38000 */
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

	ROM_REGION(0x20000,"user2",0) /* DFS ROMS */

	ROM_LOAD("dfs09.rom",    0x00000, 0x2000, CRC(3ce609cf) SHA1(5cc0f14b8f46855c70eaa653cca4ad079b458732))
	ROM_RELOAD(              0x02000, 0x2000                )

	ROM_LOAD("dnfs.rom",     0x04000, 0x4000, CRC(8ccd2157) SHA1(7e3c536baeae84d6498a14e8405319e01ee78232))
	ROM_LOAD("dfs144.rom",   0x08000, 0x4000, CRC(9fb8d13f) SHA1(387d2468c6e1360f5b531784ce95d5f71a50c2b5))
	ROM_LOAD("zdfs-0.90.rom",0x0C000, 0x2000, CRC(ea579d4d) SHA1(59ad2a8994f4bddad6687891f1a2bc29f2fd32b8))
	ROM_LOAD("ddfs223.rom",  0x10000, 0x4000, CRC(7891f9b7) SHA1(0d7ed0b0b3852cb61970ada1993244f2896896aa))
	ROM_LOAD("ddfs-1.53.rom",0x14000, 0x4000, CRC(e1be4ee4) SHA1(6719dc958f2631e6dc8f045429797b289bfe649a))
	ROM_LOAD("ch103.rom",    0x18000, 0x4000, CRC(98367cf4) SHA1(eca3631aa420691f96b72bfdf2e9c2b613e1bf33))
   /*NONE*/

	ROM_REGION(0x80000, "disks", ROMREGION_ERASEFF) /* Opus Ram Disc Space */

ROM_END

#ifdef UNUSED_DEFINITION
ROM_START(bbcbcsw)
	ROM_REGION(0x08000,"maincpu",ROMREGION_ERASEFF) /* RAM */

	ROM_REGION(0x44000,"user1",0) /* ROM */

	ROM_LOAD("os12.rom", 0x40000,0x4000, CRC(3c14fc70) SHA1(0d9bcaf6a393c9ce2359ed700ddb53c232c2c45d))

	// usos12.rom is the USA version of the OS. acorn tried to release the BBC B in the USA calling it the
	// "Acorn Proton", it failed to sell in the USA and was withdrawn from the market.
	// the main difference is the screen resolution setting the display to work on American TV's
	//ROM_LOAD("usos12.rom", 0x40000,0x4000, CRC(c8e946a9) )


	ROM_LOAD("basic2.rom",  0x00000, 0x4000, CRC(79434781) SHA1(4a7393f3a45ea309f744441c16723e2ef447a281)) /* rom page 15 3c000 */
	//ROM_LOAD("speech-1.0.rom",  0x08000, 0x2000, CRC(e63f7fb7) )
	//ROM_RELOAD(                 0x0a000, 0x2000                )
	//ROM_LOAD("dfs144.rom",  0x04000, 0x4000, CRC(9fb8d13f) SHA1(387d2468c6e1360f5b531784ce95d5f71a50c2b5)) /* rom page 14 38000 */
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

	ROM_REGION(0x20000,"user2",0) /* DFS ROMS */

	ROM_LOAD("dfs09.rom",    0x00000, 0x2000, CRC(3ce609cf) SHA1(5cc0f14b8f46855c70eaa653cca4ad079b458732))
	ROM_RELOAD(              0x02000, 0x2000                )

	ROM_LOAD("dnfs.rom",     0x04000, 0x4000, CRC(8ccd2157) SHA1(7e3c536baeae84d6498a14e8405319e01ee78232))
	ROM_LOAD("dfs144.rom",   0x08000, 0x4000, CRC(9fb8d13f) SHA1(387d2468c6e1360f5b531784ce95d5f71a50c2b5))
	ROM_LOAD("zdfs-0.90.rom",0x0C000, 0x2000, CRC(ea579d4d) SHA1(59ad2a8994f4bddad6687891f1a2bc29f2fd32b8))
	ROM_LOAD("ddfs223.rom",  0x10000, 0x4000, CRC(7891f9b7) SHA1(0d7ed0b0b3852cb61970ada1993244f2896896aa))
	ROM_LOAD("ddfs-1.53.rom",0x14000, 0x4000, CRC(e1be4ee4) SHA1(6719dc958f2631e6dc8f045429797b289bfe649a))
	ROM_LOAD("ch103.rom",    0x18000, 0x4000, CRC(98367cf4) SHA1(eca3631aa420691f96b72bfdf2e9c2b613e1bf33))
   /*NONE*/

	ROM_REGION(0x80000, "disks", ROMREGION_ERASEFF) /* Opus Ram Disc Space */

ROM_END
#endif

ROM_START(bbcbp)
	ROM_REGION(0x10000,"maincpu",ROMREGION_ERASEFF) /* ROM MEMORY */

	ROM_REGION(0x44000,"user1",0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, CRC(9f356396) SHA1(ea7d3a7e3ee1ecfaa1483af994048057362b01f2))  /* basic rom */
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
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, CRC(7891f9b7) SHA1(0d7ed0b0b3852cb61970ada1993244f2896896aa)) /* rom page 14 38000 */

ROM_END


ROM_START(bbcbp128)
	ROM_REGION(0x10000,"maincpu",ROMREGION_ERASEFF) /* ROM MEMORY */

	ROM_REGION(0x44000,"user1",0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, CRC(9f356396) SHA1(ea7d3a7e3ee1ecfaa1483af994048057362b01f2))  /* basic rom */
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
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, CRC(7891f9b7) SHA1(0d7ed0b0b3852cb61970ada1993244f2896896aa)) /* rom page 14 38000 */

ROM_END


/* BBC Master Rom Load */
ROM_START(bbcm)
	ROM_REGION(0x10000,"maincpu",ROMREGION_ERASEFF) /* ROM MEMORY */

	ROM_REGION(0x44000,"user1",0) /* ROM */

	ROM_SYSTEM_BIOS( 0, "mos350", "Enhanced MOS 3.50" )
	ROMX_LOAD("mos+3.50.rom",0x20000, 0x20000, CRC(141027b9) SHA1(85211b5bc7c7a269952d2b063b7ec0e1f0196803),ROM_BIOS(1))

	ROM_SYSTEM_BIOS( 1, "mos320", "Original MOS 3.20" )
	ROMX_LOAD("mos3.20.rom",0x20000, 0x20000, CRC(0cfad2ce) SHA1(0275719aa7746dd3b627f95ccc4362b564063a5e),ROM_BIOS(2))

	/* Move loaded roms into place */
	ROM_COPY("user1",0x20000,0x40000,0x4000)
	ROM_FILL(0x20000,0x4000,0xFFFF)

	/* 00000 rom 0   Cartridge */
	/* 04000 rom 1   Cartridge */
	/* 08000 rom 2   Cartridge */
	/* 0c000 rom 3   Cartridge */
	/* 10000 rom 4   RAM */
	/* 14000 rom 5   RAM */
	/* 18000 rom 6   RAM */
	/* 1c000 rom 7   RAM */
	/* 20000 rom 8   SPARE SOCKET */
	/* 24000 rom 9   DFS */
	/* 28000 rom 10  Viewsheet */
	/* 2c000 rom 11  Edit */
	/* 30000 rom 12  Basic */
	/* 34000 rom 13  ADFS */
	/* 38000 rom 14  View */
	/* 3c000 rom 15  Terminal */
ROM_END





static INTERRUPT_GEN( bbcb_vsync )
{
	const device_config *via_0 = devtag_get_device(device->machine, "via6522_0");
	via_ca1_w(via_0, 0,1);
	via_ca1_w(via_0, 0,0);
	bbc_frameclock();
}


//static const struct TMS5220interface tms5220_interface =
//{
//  680000L,
//  50,
//  bbc_TMSint
//};


static const cassette_config bbc_cassette_config =
{
	bbc_cassette_formats,
	NULL,
	CASSETTE_PLAY
};


static WRITE_LINE_DEVICE_HANDLER(bbcb_ack_w)
{
	via_ca1_w(device, 0, !state); /* ack seems to be inverted? */
}

static const centronics_interface bbcb_centronics_config =
{
	FALSE,
	DEVCB_DEVICE_LINE("via6522_1", bbcb_ack_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static ACIA6850_INTERFACE( bbc_acia6850_interface )
{
	0,
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", M6502_IRQ_LINE)
};

static FLOPPY_OPTIONS_START(bbc)
	FLOPPY_OPTION(bbc, "ssd,bbc,img", "BBC disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config bbc_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(bbc),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( bbc_cartslot )
	MDRV_CARTSLOT_ADD("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(bbcb_cart)

	MDRV_CARTSLOT_ADD("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(bbcb_cart)

	MDRV_CARTSLOT_ADD("cart3")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(bbcb_cart)

	MDRV_CARTSLOT_ADD("cart4")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(bbcb_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bbca )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, 2000000)        /* 2.00 MHz */
	MDRV_CPU_PROGRAM_MAP( bbca_mem)
	MDRV_CPU_VBLANK_INT("screen", bbcb_vsync)				/* screen refresh interrupts */
	MDRV_CPU_PERIODIC_INT(bbcb_keyscan, 1000)		/* scan keyboard */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(128))
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( bbca )
	MDRV_MACHINE_RESET( bbca )

	/* video hardware */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(800,300)
	MDRV_SCREEN_VISIBLE_AREA(0,800-1,0,300-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(bbc)

	MDRV_VIDEO_START(bbca)
	MDRV_VIDEO_UPDATE(bbc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489", SN76489, 4000000)	/* 4 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
//  MDRV_SOUND_ADD("tms5220", TMS5220, tms5220_interface)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", bbc_cassette_config )

	/* acia */
	MDRV_ACIA6850_ADD("acia6850", bbc_acia6850_interface)

	/* devices */
	MDRV_UPD7002_ADD("upd7002",BBC_uPD7002)
	MDRV_VIA6522_ADD("via6522_0", 1000000, bbcb_system_via)

	MDRV_I8271_ADD("i8271", bbc_i8271_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bbcb )
	MDRV_IMPORT_FROM( bbca )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( bbcb_mem)
	MDRV_MACHINE_START( bbcb )
	MDRV_MACHINE_RESET( bbcb )
	MDRV_VIDEO_START( bbcb )

	MDRV_VIA6522_ADD("via6522_1", 1000000, bbcb_user_via)
	MDRV_CENTRONICS_ADD("centronics", bbcb_centronics_config)

	MDRV_WD177X_ADD("wd177x", bbc_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(bbc_floppy_config)
	MDRV_IMPORT_FROM(bbc_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bbcbp )
	MDRV_IMPORT_FROM( bbca )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( bbcbp_mem)
	MDRV_MACHINE_START( bbcbp )
	MDRV_MACHINE_RESET( bbcbp )
	MDRV_VIDEO_START( bbcbp )

	MDRV_VIA6522_ADD("via6522_1", 1000000, bbcb_user_via)
	MDRV_CENTRONICS_ADD("centronics", bbcb_centronics_config)
	MDRV_WD177X_ADD("wd177x", bbc_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(bbc_floppy_config)
	MDRV_IMPORT_FROM(bbc_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bbcbp128 )
	MDRV_IMPORT_FROM( bbca )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( bbcbp128_mem)
	MDRV_MACHINE_START( bbcbp )
	MDRV_MACHINE_RESET( bbcbp )
	MDRV_VIDEO_START( bbcbp )

	MDRV_VIA6522_ADD("via6522_1", 1000000, bbcb_user_via)
	MDRV_CENTRONICS_ADD("centronics", bbcb_centronics_config)

	MDRV_WD177X_ADD("wd177x", bbc_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(bbc_floppy_config)
	MDRV_IMPORT_FROM(bbc_cartslot)
MACHINE_DRIVER_END


/****BBC MASTER */
static MACHINE_DRIVER_START( bbcm )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M65SC02, 2000000)        /* 2.00 MHz */
	MDRV_CPU_PROGRAM_MAP( bbcm_mem)
	MDRV_CPU_VBLANK_INT("screen", bbcb_vsync)				/* screen refresh interrupts */
	MDRV_CPU_PERIODIC_INT(bbcm_keyscan, 1000)		/* scan keyboard */
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( bbcm )
	MDRV_MACHINE_RESET( bbcm )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(128))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(800,300)
	MDRV_SCREEN_VISIBLE_AREA(0,800-1,0,300-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(bbc)

	MDRV_VIDEO_START(bbcm)
	MDRV_VIDEO_UPDATE(bbc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489", SN76489, 4000000)	/* 4 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", bbcb_centronics_config)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", bbc_cassette_config )

	/* acia */
	MDRV_ACIA6850_ADD("acia6850", bbc_acia6850_interface)

	/* devices */
	MDRV_UPD7002_ADD("upd7002",BBC_uPD7002)
	MDRV_VIA6522_ADD("via6522_0", 1000000, bbcb_system_via)
	MDRV_VIA6522_ADD("via6522_1", 1000000, bbcb_user_via)

	MDRV_WD177X_ADD("wd177x", bbc_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(bbc_floppy_config)

	MDRV_IMPORT_FROM(bbc_cartslot)
MACHINE_DRIVER_END

/*     YEAR  NAME      PARENT    COMPAT MACHINE   INPUT  INIT      CONFIG   COMPANY  FULLNAME */
COMP ( 1981, bbca,	   0,		 0,		bbca,     bbca,   bbc,     0,		"Acorn","BBC Micro Model A" , 0)
COMP ( 1981, bbcb,     bbca,	 0,		bbcb,     bbca,   bbc,	   0,		"Acorn","BBC Micro Model B" , 0)
COMP ( 1985, bbcbp,    bbca,	 0,		bbcbp,    bbca,   bbc,     0,		"Acorn","BBC Micro Model B+ 64K" , 0)
COMP ( 1985, bbcbp128, bbca,     0,		bbcbp128, bbca,   bbc,     0,		"Acorn","BBC Micro Model B+ 128k" , 0)
COMP ( 198?, bbcm,     bbca,     0,		bbcm,     bbca,   bbcm,    0,		"Acorn","BBC Master" , 0)
