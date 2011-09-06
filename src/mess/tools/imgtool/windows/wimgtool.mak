###########################################################################
#
#   wimgtool.mak
#
#   MESS wimgtool makefile
#
###########################################################################


# wimgtool executable name
WIMGTOOL = wimgtool$(EXE)

# wimgtool directories
WIMGTOOLSRC = $(MESSSRC)/tools/imgtool/windows
WIMGTOOLOBJ = $(MESS_TOOLS)/imgtool/windows



#-------------------------------------------------
# wimgtool objects
#-------------------------------------------------

OBJDIRS += $(WIMGTOOLOBJ)

WIMGTOOL_OBJS = \
	$(MESS_TOOLS)/pile.o					\
	$(MESS_TOOLS)/imgtool/windows/opcntrl.o	\
	$(MESS_TOOLS)/imgtool/windows/winutils.o	\
	$(MESS_TOOLS)/imgtool/windows/wmain.o		\
	$(MESS_TOOLS)/imgtool/windows/wimgtool.o	\
	$(MESS_TOOLS)/imgtool/windows/attrdlg.o	\
	$(MESS_TOOLS)/imgtool/windows/assoc.o		\
	$(MESS_TOOLS)/imgtool/windows/assocdlg.o	\
	$(MESS_TOOLS)/imgtool/windows/hexview.o	\
	$(MESS_TOOLS)/imgtool/windows/secview.o	\
	$(MESS_TOOLS)/imgtool/windows/wimgtool.res	\


#-------------------------------------------------
# linker flags
#-------------------------------------------------

ifdef MSVC_BUILD
# workaround to allow linking with MSVC
LDFLAGS_WIN = $(subst /ENTRY:wmainCRTStartup,,$(LDFLAGS))
LIBS_WIN = $(LIBS) shell32.lib
else
LDFLAGS_WIN = $(LDFLAGS)
LIBS_WIN = $(LIBS)
endif

#-------------------------------------------------
# rules to build the wimgtool executable
#-------------------------------------------------

$(WIMGTOOLOBJ)/%.res: $(WIMGTOOLSRC)/%.rc | $(OSPREBUILD)
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WIMGTOOLSRC) -o $@ -i $<

$(WIMGTOOL): $(VERSIONOBJ) $(WIMGTOOL_OBJS) $(LIBIMGTOOL) $(FORMATS_LIB) $(LIBEMU) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE_NOMAIN)
	@echo Linking $@...
	$(LD) $(LDFLAGS_WIN) -municode -mwindows $^ $(LIBS_WIN) -o $@
