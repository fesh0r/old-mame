###########################################################################
#
#   tools.mak
#
#   MAME tools makefile
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


TOOLSSRC = $(SRC)/tools
TOOLSOBJ = $(OBJ)/tools

OBJDIRS += \
	$(OBJ)/tools \



#-------------------------------------------------
# set of tool targets
#-------------------------------------------------

# file2str is a build tool, so it is built into the $(OBJ)
# directory; all other tools are output tools and get built
# into the root
FILE2STR = $(OBJ)/file2str$(EXE)

TOOLS += \
	$(FILE2STR) \
	romcmp$(EXE) \
	chdman$(EXE) \
	jedutil$(EXE) \
	makemeta$(EXE) \
	regrep$(EXE) \



#-------------------------------------------------
# file2str
#-------------------------------------------------

FILE2STROBJS = \
	$(TOOLSOBJ)/file2str.o \

$(FILE2STR): $(FILE2STROBJS) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# romcmp
#-------------------------------------------------

ROMCMPOBJS = \
	$(TOOLSOBJ)/romcmp.o \

romcmp$(EXE): $(ROMCMPOBJS) $(LIBUTIL) $(ZLIB) $(EXPAT) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# chdman
#-------------------------------------------------

CHDMANOBJS = \
	$(TOOLSOBJ)/chdman.o \
	$(TOOLSOBJ)/chdcd.o \

chdman$(EXE): $(VERSIONOBJ) $(CHDMANOBJS) $(LIBUTIL) $(ZLIB) $(EXPAT) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# jedutil
#-------------------------------------------------

JEDUTILOBJS = \
	$(TOOLSOBJ)/jedutil.o \

jedutil$(EXE): $(JEDUTILOBJS) $(LIBUTIL) $(LIBOCORE) $(ZLIB) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# makemeta
#-------------------------------------------------

MAKEMETAOBJS = \
	$(TOOLSOBJ)/makemeta.o \

makemeta$(EXE): $(MAKEMETAOBJS) $(LIBUTIL) $(LIBOCORE) $(ZLIB) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# regrep
#-------------------------------------------------

REGREPOBJS = \
	$(TOOLSOBJ)/regrep.o \

regrep$(EXE): $(REGREPOBJS) $(LIBUTIL) $(LIBOCORE) $(ZLIB) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
