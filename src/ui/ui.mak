#####################################################################
# make SUFFIX=32

# don't create gamelist.txt
# TEXTS = gamelist.txt

ifndef MSVC
# remove pedantic
$(OBJ)/ui/%.o: src/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

# remove pedantic
$(OBJ)/mess/windowsui/%.o: mess/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@
endif

OBJDIRS += $(OBJ)/ui

# only OS specific output files and rules
OSOBJS += \
        $(OBJ)/ui/m32util.o \
        $(OBJ)/ui/directinput.o \
        $(OBJ)/ui/dijoystick.o \
        $(OBJ)/ui/directdraw.o \
        $(OBJ)/ui/directories.o \
        $(OBJ)/ui/audit32.o \
        $(OBJ)/ui/columnedit.o \
        $(OBJ)/ui/screenshot.o \
        $(OBJ)/ui/treeview.o \
        $(OBJ)/ui/splitters.o \
        $(OBJ)/ui/bitmask.o \
        $(OBJ)/ui/datamap.o \
        $(OBJ)/ui/dxdecode.o \
		$(OBJ)/ui/help.o \
		$(OBJ)/ui/history.o \
		$(OBJ)/ui/dialogs.o \
		$(OBJ)/ui/properties.o \
 		$(OBJ)/mess/ui/ms32main.o \
 		$(OBJ)/mess/ui/layoutms.o \
		$(OBJ)/mess/ui/mess32ui.o \
		$(OBJ)/mess/ui/ms32util.o \
		$(OBJ)/mess/ui/optionsms.o \
		$(OBJ)/mess/ui/propertiesms.o \
		$(OBJ)/mess/ui/smartlistview.o \
		$(OBJ)/mess/ui/softwarelist.o

# add resource file
GUIRESFILE = $(OBJ)/mess/ui/mess32.res

#####################################################################
# compiler

#
# Preprocessor Definitions
#

DEFS += -DDIRECTSOUND_VERSION=0x0300 \
        -DDIRECTINPUT_VERSION=0x0500 \
        -DDIRECTDRAW_VERSION=0x0300 \
        -DWINVER=0x0500 \
        -D_WIN32_IE=0x0500 \
        -D_WIN32_WINNT=0x0400 \
        -DWIN32 \
        -UWINNT \
        -DCLIB_DECL=__cdecl \
	-DDECL_SPEC= \
        -DZEXTERN=extern \


#####################################################################
# Resources

ifndef MSVC
RC = windres --use-temp-file

RCDEFS = -DMESS -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src --include-dir mess/ui --include-dir src/ui

ifdef DEBUG
RCFLAGS += -DMAME_DEBUG
endif

$(OBJ)/ui/%.res: src/windowsui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<

$(OBJ)/mess/windows/%.res: mess/windows/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<

$(OBJ)/mess/ui/%.res: mess/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<
endif

#####################################################################
# Linker

ifndef MSVC
LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 
#        -lhtmlhelp
endif

#####################################################################

