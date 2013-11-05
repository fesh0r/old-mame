###########################################################################
#
#   video.mak
#
#   Rules for building video cores
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


VIDEOSRC = $(EMUSRC)/video
VIDEOOBJ = $(EMUOBJ)/video


#-------------------------------------------------
#
#@src/emu/video/315_5124.h,VIDEOS += SEGA315_5124
#-------------------------------------------------

ifneq ($(filter SEGA315_5124,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/315_5124.o
endif

#-------------------------------------------------
#
#@src/emu/video/bufsprite.h,VIDEOS += BUFSPRITE
#-------------------------------------------------

ifneq ($(filter BUFSPRITE,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/bufsprite.o
endif

#-------------------------------------------------
#
#@src/emu/video/cdp1861.h,VIDEOS += CDP1861
#-------------------------------------------------

ifneq ($(filter CDP1861,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/cdp1861.o
endif

#-------------------------------------------------
#
#@src/emu/video/cdp1862.h,VIDEOS += CDP1862
#-------------------------------------------------

ifneq ($(filter CDP1862,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/cdp1862.o
endif

#-------------------------------------------------
#
#@src/emu/video/crt9007.h,VIDEOS += CRT9007
#-------------------------------------------------

ifneq ($(filter CRT9007,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/crt9007.o
endif

#-------------------------------------------------
#
#@src/emu/video/crt9021.h,VIDEOS += CRT9021
#-------------------------------------------------

ifneq ($(filter CRT9021,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/crt9021.o
endif

#-------------------------------------------------
#
#@src/emu/video/crt9212.h,VIDEOS += CRT9212
#-------------------------------------------------

ifneq ($(filter CRT9212,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/crt9212.o
endif

#-------------------------------------------------
#
#@src/emu/video/dl1416.h,VIDEOS += DL1416
#-------------------------------------------------

ifneq ($(filter DL1416,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/dl1416.o
endif

#-------------------------------------------------
#
#@src/emu/video/dm9368.h,VIDEOS += DM9368
#-------------------------------------------------

ifneq ($(filter DM9368,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/dm9368.o
endif

#-------------------------------------------------
#
#@src/emu/video/ef9340_1.h,VIDEOS += EF9340_1
#-------------------------------------------------

ifneq ($(filter EF9340_1,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/ef9340_1.o
endif

#-------------------------------------------------
#
#@src/emu/video/fixfreq.h,VIDEOS += FIXFREQ
#-------------------------------------------------

ifneq ($(filter FIXFREQ,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/fixfreq.o
endif

#-------------------------------------------------
#
#@src/emu/video/h63484.h,VIDEOS += H63484
#-------------------------------------------------

ifneq ($(filter H63484,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/h63484.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd44102.h,VIDEOS += HD44102
#-------------------------------------------------

ifneq ($(filter HD44102,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd44102.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd44352.h,VIDEOS += HD44352
#-------------------------------------------------

ifneq ($(filter HD44352,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd44352.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd44780.h,VIDEOS += HD44780
#-------------------------------------------------

ifneq ($(filter HD44780,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd44780.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd61830.h,VIDEOS += HD61830
#-------------------------------------------------

ifneq ($(filter HD61830,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd61830.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd63484.h,VIDEOS += HD63484
#-------------------------------------------------

ifneq ($(filter HD63484,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd63484.o
endif

#-------------------------------------------------
#
#@src/emu/video/hd66421.h,VIDEOS += HD66421
#-------------------------------------------------

ifneq ($(filter HD66421,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/hd66421.o
endif

#-------------------------------------------------
#
#@src/emu/video/huc6202.h,VIDEOS += HUC6202
#-------------------------------------------------

ifneq ($(filter HUC6202,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/huc6202.o
endif

#-------------------------------------------------
#
#@src/emu/video/huc6260.h,VIDEOS += HUC6260
#-------------------------------------------------

ifneq ($(filter HUC6260,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/huc6260.o
endif

#-------------------------------------------------
#
#@src/emu/video/huc6261.h,VIDEOS += HUC6261
#-------------------------------------------------

ifneq ($(filter HUC6261,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/huc6261.o
endif

#-------------------------------------------------
#
#@src/emu/video/huc6270.h,VIDEOS += HUC6270
#-------------------------------------------------

ifneq ($(filter HUC6270,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/huc6270.o
endif

#-------------------------------------------------
#
#@src/emu/video/huc6272.h,VIDEOS += HUC6272
#-------------------------------------------------

ifneq ($(filter HUC6272,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/huc6272.o
endif

#-------------------------------------------------
#
#@src/emu/video/i8244.h,VIDEOS += I8244
#-------------------------------------------------

ifneq ($(filter I8244,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/i8244.o
endif

#-------------------------------------------------
#
#@src/emu/video/i8275.h,VIDEOS += I8275
#@src/emu/video/i8275x.h,VIDEOS += I8275
#-------------------------------------------------

ifneq ($(filter I8275,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/i8275.o         \
			$(VIDEOOBJ)/i8275x.o
endif

#-------------------------------------------------
#
#@src/emu/video/k053250.h,VIDEOS += K053250
#-------------------------------------------------

ifneq ($(filter K053250,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/k053250.o
endif

#-------------------------------------------------
#
#@src/emu/video/m50458.h,VIDEOS += M50458
#-------------------------------------------------

ifneq ($(filter M50458,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/m50458.o
endif

#-------------------------------------------------
#
#@src/emu/video/mb90082.h,VIDEOS += MB90082
#-------------------------------------------------

ifneq ($(filter MB90082,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/mb90082.o
endif

#-------------------------------------------------
#
#@src/emu/video/mb_vcu.h,VIDEOS += MB_VCU
#-------------------------------------------------

ifneq ($(filter MB_VCU,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/mb_vcu.o
endif

#-------------------------------------------------
#
#@src/emu/video/mc6845.h,VIDEOS += MC6845
#-------------------------------------------------

ifneq ($(filter MC6845,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/mc6845.o
endif

#-------------------------------------------------
#
#@src/emu/video/mc6847.h,VIDEOS += MC6847
#-------------------------------------------------

ifneq ($(filter MC6847,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/mc6847.o
endif

#-------------------------------------------------
#
#@src/emu/video/msm6255.h,VIDEOS += MSM6255
#-------------------------------------------------

ifneq ($(filter MSM6255,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/msm6255.o
endif

#-------------------------------------------------
#
#@src/emu/video/pc_cga.h,VIDEOS += PC_CGA
#-------------------------------------------------

ifneq ($(filter PC_CGA,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/pc_cga.o        \
			$(VIDEOOBJ)/cgapal.o
endif

#-------------------------------------------------
#
#@src/emu/video/pc_vga.h,VIDEOS += PC_VGA
#-------------------------------------------------

ifneq ($(filter PC_VGA,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/pc_vga.o
endif

#-------------------------------------------------
#
#@src/emu/video/poly.h,VIDEOS += POLY
#-------------------------------------------------

ifneq ($(filter POLY,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/poly.o
endif

#-------------------------------------------------
#
#@src/emu/video/psx.h,VIDEOS += PSX
#-------------------------------------------------

ifneq ($(filter PSX,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/psx.o
endif

#-------------------------------------------------
#
#@src/emu/video/ramdac.h,VIDEOS += RAMDAC
#-------------------------------------------------

ifneq ($(filter RAMDAC,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/ramdac.o
endif

#-------------------------------------------------
#
#@src/emu/video/saa5050.h,VIDEOS += SAA5050
#-------------------------------------------------

ifneq ($(filter SAA5050,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/saa5050.o
endif

#-------------------------------------------------
#
#@src/emu/video/sed1330.h,VIDEOS += SED1330
#-------------------------------------------------
ifneq ($(filter SED1330,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/sed1330.o
endif

#-------------------------------------------------
#
#@src/emu/video/stvvdp1.h,VIDEOS += STVVDP
#@src/emu/video/stvvdp2.h,VIDEOS += STVVDP
#-------------------------------------------------

ifneq ($(filter STVVDP,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/stvvdp1.o       \
			$(VIDEOOBJ)/stvvdp2.o
endif

#-------------------------------------------------
#
#@src/emu/video/tlc34076.h,VIDEOS += TLC34076
#-------------------------------------------------

ifneq ($(filter TLC34076,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/tlc34076.o
endif

#-------------------------------------------------
#
#@src/emu/video/tms34061.h,VIDEOS += TMS34061
#-------------------------------------------------

ifneq ($(filter TMS34061,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/tms34061.o
endif

#-------------------------------------------------
#
#@src/emu/video/tms3556.h,VIDEOS += TMS3556
#-------------------------------------------------

ifneq ($(filter TMS3556,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/tms3556.o
endif

#-------------------------------------------------
#
#@src/emu/video/tms9927.h,VIDEOS += TMS9927
#-------------------------------------------------

ifneq ($(filter TMS9927,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/tms9927.o
endif

#-------------------------------------------------
#
#@src/emu/video/tms9928a.h,VIDEOS += TMS9928A
#-------------------------------------------------

ifneq ($(filter TMS9928A,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/tms9928a.o
endif

#-------------------------------------------------
#
#@src/emu/video/upd3301.h,VIDEOS += UPD3301
#-------------------------------------------------

ifneq ($(filter UPD3301,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/upd3301.o
endif

#-------------------------------------------------
#
#@src/emu/video/upd7220.h,VIDEOS += UPD7220
#-------------------------------------------------

ifneq ($(filter UPD7220,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/upd7220.o
endif

#-------------------------------------------------
#
#@src/emu/video/upd7227.h,VIDEOS += UPD7227
#-------------------------------------------------

ifneq ($(filter UPD7227,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/upd7227.o
endif

#-------------------------------------------------
#
#@src/emu/video/v9938.h,VIDEOS += V9938
#-------------------------------------------------

ifneq ($(filter V9938,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/v9938.o
endif

#-------------------------------------------------
#
#@src/emu/video/voodoo.h,VIDEOS += VOODOO
#-------------------------------------------------

ifneq ($(filter VOODOO,$(VIDEOS)),)
VIDEOOBJS+= $(VIDEOOBJ)/voodoo.o
endif
