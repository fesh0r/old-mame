#####################################################################
# make SUFFIX=32
#####################################################################

#-------------------------------------------------
# object and source roots
#-------------------------------------------------

WINUISRC = $(SRC)/osd/winui
WINUIOBJ = $(OBJ)/osd/winui

OBJDIRS += $(WINUIOBJ)
	
MESS_WINUISRC = $(SRC)/mess/osd/winui
MESS_WINUIOBJ = $(OBJ)/mess/osd/winui
OBJDIRS += $(MESS_WINUIOBJ)
CFLAGS += -I$(WINUISRC) -I$(MESS_WINUISRC)



#-------------------------------------------------
# Windows UI object files
#-------------------------------------------------

WINUIOBJS += \
	$(WINUIOBJ)/mui_util.o \
	$(WINUIOBJ)/directinput.o \
	$(WINUIOBJ)/dijoystick.o \
	$(WINUIOBJ)/directdraw.o \
	$(WINUIOBJ)/directories.o \
	$(WINUIOBJ)/mui_audit.o \
	$(WINUIOBJ)/columnedit.o \
	$(WINUIOBJ)/screenshot.o \
	$(WINUIOBJ)/treeview.o \
	$(WINUIOBJ)/splitters.o \
	$(WINUIOBJ)/bitmask.o \
	$(WINUIOBJ)/datamap.o \
	$(WINUIOBJ)/dxdecode.o \
	$(WINUIOBJ)/picker.o \
	$(WINUIOBJ)/properties.o \
	$(WINUIOBJ)/tabview.o \
	$(WINUIOBJ)/help.o \
	$(WINUIOBJ)/history.o \
	$(WINUIOBJ)/dialogs.o \
	$(WINUIOBJ)/dirwatch.o	\
	$(WINUIOBJ)/datafile.o	\
	$(WINUIOBJ)/mui_opts.o \
	$(WINUIOBJ)/winui.o \
	$(WINUIOBJ)/helpids.o \
	$(MESS_WINUIOBJ)/messui.o \
	$(MESS_WINUIOBJ)/optionsms.o \
 	$(MESS_WINUIOBJ)/layoutms.o \
	$(MESS_WINUIOBJ)/msuiutil.o \
	$(MESS_WINUIOBJ)/propertiesms.o \
	$(MESS_WINUIOBJ)/softwarepicker.o \
	$(MESS_WINUIOBJ)/devview.o


# add resource file
GUIRESFILE = $(MESS_WINUIOBJ)/messui.res

$(LIBOSD): $(WINUIOBJS)


#-------------------------------------------------
# rules for creating helpids.c 
#-------------------------------------------------

$(WINUIOBJ)/helpids.o : $(WINUIOBJ)/helpids.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

$(WINUIOBJ)/helpids.c : $(WINUIOBJ)/mkhelp$(EXE) $(WINUISRC)/resource.h $(WINUISRC)/resource.hm $(WINUISRC)/mameui.rc
	$(WINUIOBJ)/mkhelp$(EXE) $(WINUISRC)/mameui.rc >$@

# rule to build the generator
$(WINUIOBJ)/mkhelp$(EXE): $(WINUIOBJ)/mkhelp.o $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@


#####################################################################
# compiler

#
# Preprocessor Definitions
#

ifdef MSVC_BUILD
DEFS += -DWINVER=0x0500 -D_CRT_NON_CONFORMING_SWPRINTFS
else
DEFS += -DWINVER=0x0400
endif

DEFS += \
	-D_WIN32_IE=0x0500 \
	-DDECL_SPEC= \
	-DZEXTERN=extern \

#	-DSHOW_UNAVAILABLE_FOLDER



#####################################################################
# Linker

ifndef MSVC
LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \

endif



#####################################################################
# Resources


$(MESS_WINUIOBJ)/messui.res:	$(WINUISRC)/mameui.rc $(MESS_WINUISRC)/messui.rc $(WINUISRC)/resource.h $(MESS_WINUISRC)/resourcems.h $(WINUIOBJ)/mamevers.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WINUISRC) --include-dir $(MESS_WINUISRC) --include-dir $(WINUIOBJ) -o $@ -i $(MESS_WINUISRC)/messui.rc

$(WINUIOBJ)/mamevers.rc: $(WINOBJ)/verinfo$(EXE) $(SRC)/version.c
	@echo Emitting $@...
	@$(VERINFO) $(SRC)/version.c > $@
