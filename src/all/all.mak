###########################################################################
#
#   mame.mak
#
#   MAME target makefile
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################
EMULATOR=

EMU_EXE = $(PREFIX)mame$(SUFFIX)$(SUFFIX64)$(SUFFIXDEBUG)$(EXE)

BUILD += $(EMU_EXE)

$(EMU_EXE): $(VERSIONOBJ) $(DRIVLISTOBJ) $(DRVLIBS) $(LIBOSD) $(LIBCPU) $(LIBEMU) $(LIBDASM) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(SOFTFLOAT) $(FORMATS_LIB) $(ZLIB) $(LIBOCORE) $(RESFILE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

include $(SRC)/mame/mame.mak

#EMU_EXE2 = $(PREFIX)mess$(SUFFIX)$(SUFFIX64)$(SUFFIXDEBUG)$(EXE)

#BUILD += $(EMU_EXE2)

#include $(SRC)/mess/mess.mak
