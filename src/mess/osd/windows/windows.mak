###########################################################################
#
#   windows.mak
#
#   MESS Windows-specific makefile
#
###########################################################################

ifndef MESSUI
MESSUI = 1
endif

# build the executable names
RCFLAGS += -DMESS

LIBS += -lcomdlg32

OBJDIRS += \
	$(MESSOBJ)/osd \
	$(MESSOBJ)/osd/windows

MESS_WINSRC = src/mess/osd/windows
MESS_WINOBJ = $(OBJ)/mess/osd/windows

RESFILE = $(MESS_WINOBJ)/mess.res

OSDOBJS += \
	$(MESS_WINOBJ)/dialog.o	\
	$(MESS_WINOBJ)/menu.o		\
	$(MESS_WINOBJ)/mess.res	\
	$(MESS_WINOBJ)/opcntrl.o

$(LIBOSD): $(OSDOBJS)

OSDCOREOBJS += \
	$(OBJ)/mess/osd/windows/winutils.o

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

#-------------------------------------------------
# generic rules for the resource compiler
#-------------------------------------------------

$(MESS_WINOBJ)/%.res: $(MESS_WINSRC)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/$(OSD) -o $@ -i $<

#-------------------------------------------------
# For building UI include ui.mak
#-------------------------------------------------
ifneq ($(MESSUI),0)
include $(SRC)/mess/osd/winui/winui.mak
endif