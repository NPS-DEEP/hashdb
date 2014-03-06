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

HASHDBVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

UNVERSIONED_INSTALLER = temp-hashdb-windowsinstaller.exe
INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

PDFDOCS = hashdb_user_guide.pdf

# The prerequisites for the installer can't be listed after the colon 
# becuase then the system will try to generate them simultaneously,
# which causes problems if both try to do a distclean at the same time.
# So this just manually checks to see if the prerequists are present and,
# if not, it makes them.
#
$(UNVERSIONED_INSTALLER): build_installer.nsi $(PDFDOCS)
	@echo Making hashdb version $(HASHDBVERSION) Windows Installer
	if [ ! -r ../win32/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../win32 ]; then mkdir ../win32 ; fi; \
	   if [ ! -r ../win32/Makefile ]; then (cd ../win32;mingw32-configure); fi; \
	fi
	echo '**** running make in win32/ to create win32 .exe files ****'; \
	(cd ../win32;$(MAKE))
	cp ../win32/src/hashdb.exe hashdb32.exe
	@echo 
	@echo
	if [ ! -r ../win64/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../win64 ]; then mkdir ../win64 ; fi; \
	   if [ ! -r ../win64/Makefile ]; then (cd ../win64;mingw64-configure); fi; \
	fi
	echo '**** running make in win64/ to create win64 .exe files ****'; \
	(cd ../win64;$(MAKE))
	cp ../win64/src/hashdb.exe hashdb64.exe
	@echo
	@echo
	makensis -DVERSION=$(HASHDBVERSION) build_installer.nsi
	echo '**************** INSTALLER IS MADE ****************'
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)
	ls -lt *exe

$(SRC_WIN)clean:
	/bin/rm -rf ../win32 ../win64 *.exe
	/bin/rm -rf $(PDFDOCS)

$(SRC_WIN)distclean:
	if [ -r ../Makefile ]; then (cd ..;$(MAKE) distclean); fi
	$(MAKE) clean

$(SRC_WIN)install:
	(cd ../win32/src/libhashdb;$(MAKE) install)
	(cd ../win64/src/libhashdb;$(MAKE) install)

$(SRC_WIN)uninstall:
	(cd ../win32/src/libhashdb;$(MAKE) uninstall)
	(cd ../win64/src/libhashdb;$(MAKE) uninstall)

################################################################
##
## Get the PDF documentation from the download site
##
DOWNLOAD = http://digitalcorpora.org/downloads/hashdb
hashdb_user_guide.pdf:
	wget $(DOWNLOAD)/hashdb_user_guide.pdf

.PHONY: clean distclean install uninstall

