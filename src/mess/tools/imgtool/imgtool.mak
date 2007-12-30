###########################################################################
#
#   imgtool.mak
#
#   MESS imgtool makefile
#
###########################################################################


# imgtool executable name
IMGTOOL = imgtool$(EXE)

# add path to imgtool headers
CFLAGS += -I$(SRC)/$(TARGET)/tools/imgtool

# imgtool directories
IMGTOOLOBJ = $(MESSOBJ)/tools/imgtool
IMGTOOL_MODULES = $(IMGTOOLOBJ)/modules



#-------------------------------------------------
# imgtool objects
#-------------------------------------------------

OBJDIRS += \
	$(IMGTOOLOBJ) \
	$(IMGTOOL_MODULES)

LIBIMGTOOL = $(OBJ)/libimgtool.a

# imgtool lib objects
IMGTOOL_LIB_OBJS =						\
	$(OBJ)/version.o					\
	$(EMUOBJ)/mamecore.o				\
	$(MESSOBJ)/tagpool.o				\
	$(MESSOBJ)/utils.o					\
	$(MESSOBJ)/tagpool.o				\
	$(MESSOBJ)/opresolv.o				\
	$(MESS_FORMATS)/ioprocs.o			\
	$(MESS_FORMATS)/flopimg.o			\
	$(MESS_FORMATS)/cassimg.o			\
	$(MESS_FORMATS)/basicdsk.o			\
	$(MESS_FORMATS)/coco_dsk.o			\
	$(MESS_FORMATS)/coco_cas.o			\
	$(MESS_FORMATS)/pc_dsk.o			\
	$(MESS_FORMATS)/ap2_dsk.o			\
	$(MESS_FORMATS)/ap_dsk35.o			\
	$(MESS_FORMATS)/wavfile.o			\
	$(MESS_FORMATS)/vt_dsk.o			\
	$(IMGTOOLOBJ)/stream.o				\
	$(IMGTOOLOBJ)/library.o				\
	$(IMGTOOLOBJ)/modules.o				\
	$(IMGTOOLOBJ)/iflopimg.o			\
	$(IMGTOOLOBJ)/filter.o				\
	$(IMGTOOLOBJ)/filteoln.o			\
	$(IMGTOOLOBJ)/filtbas.o				\
	$(IMGTOOLOBJ)/imgtool.o				\
	$(IMGTOOLOBJ)/imgterrs.o			\
	$(IMGTOOLOBJ)/imghd.o				\
	$(IMGTOOLOBJ)/charconv.o			\
	$(IMGTOOL_MODULES)/macbin.o			\
	$(IMGTOOL_MODULES)/rsdos.o			\
	$(IMGTOOL_MODULES)/os9.o			\
	$(IMGTOOL_MODULES)/mac.o			\
	$(IMGTOOL_MODULES)/ti99.o			\
	$(IMGTOOL_MODULES)/ti990hd.o		\
	$(IMGTOOL_MODULES)/concept.o		\
	$(IMGTOOL_MODULES)/fat.o			\
	$(IMGTOOL_MODULES)/pc_flop.o		\
	$(IMGTOOL_MODULES)/pc_hard.o		\
	$(IMGTOOL_MODULES)/prodos.o			\
	$(IMGTOOL_MODULES)/vzdos.o			\
	$(IMGTOOL_MODULES)/thomson.o		\
	$(IMGTOOL_MODULES)/macutil.o		\
	$(IMGTOOL_MODULES)/cybiko.o
#	$(IMGTOOLOBJ)/tstsuite.o			\
#	$(MESS_FORMATS)/fmsx_cas.o			\
#	$(MESS_FORMATS)/svi_cas.o			\
#	$(MESS_FORMATS)/ti85_ser.o			\
#	$(IMGTOOL_MODULES)/imgwave.o		\
#	$(IMGTOOL_MODULES)/cococas.o		\
#	$(IMGTOOL_MODULES)/vmsx_tap.o		\
#	$(IMGTOOL_MODULES)/vmsx_gm2.o		\
#	$(IMGTOOL_MODULES)/fmsx_cas.o		\
#	$(IMGTOOL_MODULES)/svi_cas.o		\
#	$(IMGTOOL_MODULES)/msx_dsk.o		\
#	$(IMGTOOL_MODULES)/xsa.o			\
#	$(IMGTOOL_MODULES)/t64.o			\
#	$(IMGTOOL_MODULES)/lynx.o			\
#	$(IMGTOOL_MODULES)/crt.o			\
#	$(IMGTOOL_MODULES)/d64.o			\
#	$(IMGTOOL_MODULES)/rom16.o			\
#	$(IMGTOOL_MODULES)/nccard.o			\
#	$(IMGTOOL_MODULES)/ti85.o			\

$(LIBIMGTOOL): $(IMGTOOL_LIB_OBJS)

IMGTOOL_OBJS = \
	$(IMGTOOLOBJ)/main.o \
	$(IMGTOOLOBJ)/stubs.o \
	$(MESSOBJ)/toolerr.o



#-------------------------------------------------
# rules to build the imgtool executable
#-------------------------------------------------

$(IMGTOOL): $(IMGTOOL_OBJS) $(LIBIMGTOOL) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
