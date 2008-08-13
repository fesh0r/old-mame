###########################################################################
#
#   ldplayer.mak
#
#   Small makefile to build a standalone laserdisc player
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


LDPSRC = $(SRC)/ldplayer
LDPOBJ = $(OBJ)/ldplayer

OBJDIRS += \
	$(LDPOBJ) \



#-------------------------------------------------
# specify required CPU cores (none)
#-------------------------------------------------



#-------------------------------------------------
# specify required sound cores
#-------------------------------------------------

SOUNDS += CUSTOM


#-------------------------------------------------
# this is the list of driver libraries that
# comprise MAME plus mamedriv.o which contains
# the list of drivers
#-------------------------------------------------

DRVLIBS = \
	$(LDPOBJ)/ldpdriv.o \
	$(LDPOBJ)/ldplayer.o \
