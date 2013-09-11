# NOTE 1: This Makefile does not define a primary, so source files are
# manually included using EXTRA_DIST.

# NOTE 2: This Makefile is invoked from the parent directory, but as part
#         of building the installer it invokes 'make distclean' in the
#         parent directory and makes two new subdirectories:
#             win32/   - 32-bit windows
#             win64/   - 64-bit windows

# NOTE 3: This makefile is not automatically generated.  If it were,
#         then 'make distclean' would wipe it.

# NOTE: To build just win32, win64, or windist, you may use hashdb/Makefile.

EXTRA_DIST = \
	CONFIGURE_F18.bash \
	icu-mingw32-libprefix.patch \
	icu-mingw64-libprefix.patch \
	zmq-configure.in.patch \
	zmq-configure.patch \
	zmq-zmq.h.patch \
	zmq-zmq_utils.h.patch

HASHDBVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

PDFDOCS = using_hashdb.pdf

# The prerequisites for the installer can't be listed after the colon 
# becuase then the system will try to generate them simultaneously,
# which causes problems if both try to do a distclean at the same time.
# So this just manually checks to see if the prerequists are present and,
# if not, it makes them.
#
$(INSTALLER): build_installer.nsi $(PDFDOCS)
	@echo Making hashdb version $(HASHDBVERSION) Windows Installer
	if [ ! -r ../win32/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../win32 ]; then mkdir ../win32 ; fi; \
	   if [ ! -r ../win32/Makefile ]; then (cd ../win32;mingw32-configure); fi; \
	fi
	echo '**** running make in win32/ to create win32 .exe files ****'; \
	(cd ../win32;$(MAKE))
	cp win32/src/hashdb_manager.exe win32/hashdb_manager32.exe
	cp win32/src/hashdb_checker/hashdb_checker.exe win32/hashdb_checker32.exe
	@echo 
	@echo
	if [ ! -r ../win64/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../win64 ]; then mkdir ../win64 ; fi; \
	   if [ ! -r ../win64/Makefile ]; then (cd ../win64;mingw64-configure); fi; \
	fi
	echo '**** running make in win64/ to create win64 .exe files ****'; \
	(cd ../win64;$(MAKE))
	cp win64/src/hashdb_manager.exe win64/hashdb_manager64.exe
	cp win64/src/hashdb_checker/hashdb_checker.exe win64/hashdb_checker64.exe
	@echo
	@echo
	makensis -DVERSION=$(HASHDBVERSION) build_installer.nsi
	echo '**************** INSTALLER IS MADE ****************'
	ls -lt *exe

$(SRC_WIN)clean:
	/bin/rm -rf ../win32 ../win64 *.exe

$(SRC_WIN)distclean:
	if [ -r ../Makefile ]; then (cd ..;$(MAKE) distclean); fi
	$(MAKE) clean

################################################################
##
## Get the PDF documentation from where it is built
##
using_hashdb.pdf: doc/using_hashdb/using_hashdb.pdf
	cp doc/using_hashdb/using_hashdb.pdf using_hashdb.pdf


