###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make TARGET=mess SUBTARGET=tiny to build
#
#   As an example this makefile builds MESS with the three Colecovision
#   drivers enabled only.
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

# disable messui for tiny build
MESSUI = 0

# include MESS core defines
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak


#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

CPUS += Z80
CPUS += MCS48



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

SOUNDS += SN76496



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in tiny.c
#-------------------------------------------------

DRVLIBS = \
	$(MESSOBJ)/tiny.o \
	$(MESS_DRIVERS)/coleco.o \
	$(EMU_VIDEO)/tms9928a.o \
	$(MESS_DEVICES)/cartslot.o \
	$(MESS_DEVICES)/cassette.o	\
	$(MESS_DEVICES)/messram.o	\
	$(MESS_DEVICES)/multcart.o	\
	$(MESS_FORMATS)/cassimg.o	\
	$(MESS_FORMATS)/ioprocs.o	\
	$(MESS_FORMATS)/wavfile.o	\



#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(MESSOBJ)/mess.o:	$(MESS_LAYOUT)/lcd.lh
$(MESSOBJ)/mess.o:	$(MESS_LAYOUT)/lcd_rot.lh


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
