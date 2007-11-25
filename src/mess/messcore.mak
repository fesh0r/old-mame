###########################################################################
#
#   messcore.mak
#
#   MESS emulation core makefile
#
###########################################################################


#-------------------------------------------------
# MESS core defines
#-------------------------------------------------

COREDEFS += -DMESS


# Root object directories
MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame
MESSSRC = $(SRC)/mess
MESSOBJ = $(OBJ)/mess
EMUSRC = $(SRC)/emu
EMUOBJ = $(OBJ)/emu

# MAME directories
EMU_AUDIO = $(EMUOBJ)/audio
EMU_MACHINE = $(EMUOBJ)/machine
EMU_VIDEO = $(EMUOBJ)/video
MAME_AUDIO = $(MAMEOBJ)/audio
MAME_MACHINE = $(MAMEOBJ)/machine
MAME_DRIVERS = $(MAMEOBJ)/drivers
MAME_VIDEO = $(MAMEOBJ)/video

# MESS directories
MESS_AUDIO = $(MESSOBJ)/audio
MESS_DEVICES = $(MESSOBJ)/devices
MESS_DRIVERS = $(MESSOBJ)/drivers
MESS_FORMATS = $(MESSOBJ)/formats
MESS_LAYOUT = $(MESSOBJ)/layout
MESS_MACHINE = $(MESSOBJ)/machine
MESS_VIDEO = $(MESSOBJ)/video


OBJDIRS += \
	$(EMU_AUDIO) \
	$(EMU_MACHINE) \
	$(EMU_VIDEO) \
	$(MAME_AUDIO) \
	$(MAME_DRIVERS) \
	$(MAME_LAYOUT) \
	$(MAME_MACHINE) \
	$(MAME_VIDEO) \
	$(MESS_AUDIO) \
	$(MESS_DEVICES) \
	$(MESS_DRIVERS) \
	$(MESS_FORMATS) \
	$(MESS_LAYOUT) \
	$(MESS_MACHINE) \
	$(MESS_VIDEO) \


#-------------------------------------------------
# MESS core objects
#-------------------------------------------------

EMUOBJS += \
	$(MESSOBJ)/mess.o		\
	$(MESSOBJ)/messopts.o	\
	$(MESSOBJ)/configms.o	\
	$(MESSOBJ)/mesvalid.o	\
	$(MESSOBJ)/image.o		\
	$(MESSOBJ)/device.o		\
	$(MESSOBJ)/hashfile.o	\
	$(MESSOBJ)/inputx.o		\
	$(MESSOBJ)/artworkx.o	\
	$(MESSOBJ)/uimess.o		\
	$(MESSOBJ)/filemngr.o	\
	$(MESSOBJ)/tapectrl.o	\
	$(MESSOBJ)/compcfg.o	\
	$(MESSOBJ)/utils.o		\
	$(MESSOBJ)/eventlst.o	\
	$(MESSOBJ)/mscommon.o	\
	$(MESSOBJ)/tagpool.o	\
	$(MESSOBJ)/cheatms.o	\
	$(MESSOBJ)/opresolv.o	\
	$(MESSOBJ)/muitext.o	\
	$(MESSOBJ)/infomess.o	\
	$(MESSOBJ)/climess.o	\

$(LIBEMU): $(EMUOBJS)
