# nasm for Windows (but not cygwin) has a "w" at the end
ifndef COMPILESYSTEM_CYGWIN
ASM = @nasmw
endif

# only Windows specific output files and rules
# the first two targets generate the prefix.h header
# note this requires that OSOBJS be the first target
#
OSOBJS = $(OBJ)/windows/winmain.o $(OBJ)/windows/fileio.o $(OBJ)/windows/config.o \
	 $(OBJ)/windows/ticker.o $(OBJ)/windows/fronthlp.o $(OBJ)/windows/video.o \
	 $(OBJ)/windows/input.o $(OBJ)/windows/sound.o $(OBJ)/windows/blit.o \
	 $(OBJ)/windows/snprintf.o $(OBJ)/windows/rc.o $(OBJ)/windows/misc.o \
	 $(OBJ)/windows/window.o $(OBJ)/windows/wind3d.o $(OBJ)/windows/wind3dfx.o \
	 $(OBJ)/windows/winddraw.o \
	 $(OBJ)/windows/asmblit.o $(OBJ)/windows/asmtile.o

# add resource file if no UI
ifeq ($(WINUI),)
OSOBJS += $(OBJ)/windows/mame.res
endif

ifdef NEW_DEBUGGER
OSOBJS += $(OBJ)/windows/debugwin.o
endif

# enable guard pages on all memory allocations in the debug build
ifdef DEBUG
DEFS += -DMALLOC_DEBUG
OSDBGOBJS = $(OBJ)/windows/winalloc.o
OSDBGLDFLAGS = -Wl,--allow-multiple-definition
else
OSDBGOBJS =
OSDBGLDFLAGS =
endif

# video blitting functions
$(OBJ)/windows/asmblit.o: src/windows/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# tilemap blitting functions
$(OBJ)/windows/asmtile.o: src/windows/asmtile.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# add our prefix files to the mix (we need -Wno-strict-aliasing for DirectX)
CFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h
# CFLAGSOSDEPEND += -Wno-strict-aliasing

# add the windows libaries
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

# if building with a UI, set the C flags and include the ui.mak
ifneq ($(WINUI),)
CFLAGS+= -DWINUI=1
include src/ui/ui.mak
endif

# if we are not using x86drc.o, we should be
ifndef X86_MIPS3_DRC
COREOBJS += $(OBJ)/x86drc.o
endif

#####################################################################
# Resources

RC = @windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/windows

$(OBJ)/windows/%.res: src/windows/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<

