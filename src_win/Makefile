# NOTE 1: This Makefile does not define a primary, so source files are
# manually included using EXTRA_DIST.

# NOTE 2: This Makefile is invoked from the parent directory, but as part
#         of building the installer it invokes 'make distclean' in the
#         parent directory and makes new subdirectory:
#             win64/   - 64-bit windows

# NOTE 3: This makefile is not automatically generated.  If it were,
#         then 'make distclean' would wipe it.

# NOTE: To build just win64 or windist, you may use hashdb/Makefile.

HASHDBVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

UNVERSIONED_INSTALLER = temp-hashdb-windowsinstaller.exe
INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

PDFDOCS = hashdbUsersManual.pdf

# The prerequisites for the installer can't be listed after the colon 
# becuase then the system will try to generate them simultaneously,
# which causes problems if both try to do a distclean at the same time.
# So this just manually checks to see if the prerequists are present and,
# if not, it makes them.
#
$(UNVERSIONED_INSTALLER): build_installer.nsi EnvVarUpdate.nsi $(PDFDOCS)
	@echo Making hashdb version $(HASHDBVERSION) Windows Installer
	if [ ! -r ../win64/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../win64 ]; then mkdir ../win64 ; fi; \
	   if [ ! -r ../win64/Makefile ]; then (cd ../win64;mingw64-configure); fi; \
	fi
	echo '**** running make in win64/ to create win64 .exe files ****'; \
	(cd ../win64;$(MAKE))
	cp ../win64/src/hashdb.exe hashdb.exe
	@echo
	@echo
	makensis -DVERSION=$(HASHDBVERSION) build_installer.nsi
	echo '**************** INSTALLER IS MADE ****************'
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)
	ls -lt *exe

win_tests: $(UNVERSIONED_INSTALLER)
	(cd ../win64/src/test;$(MAKE))
	rm -rf win_tests
	mkdir win_tests
	mkdir win_tests/win64
	cp ../win64/src/test/*.exe win_tests/win64
	cp run_tests.bat win_tests
	cp ../src/test/identified_blocks.txt win_tests
	cp ../src/test/sample_dfxml win_tests
	zip -r9 win_tests-$(HASHDBVERSION).zip win_tests

$(SRC_WIN)clean:
	/bin/rm -rf ../win64 *.exe
	/bin/rm -rf $(PDFDOCS)

$(SRC_WIN)distclean:
	if [ -r ../Makefile ]; then (cd ..;$(MAKE) distclean); fi
	$(MAKE) clean

$(SRC_WIN)install:
	(cd ../win64/src;$(MAKE) install)

$(SRC_WIN)uninstall:
	(cd ../win64/src;$(MAKE) uninstall)

################################################################
##
## Get the PDF documentation from the download site
##
DOWNLOAD = http://digitalcorpora.org/downloads/hashdb
hashdbUsersManual.pdf:
	wget $(DOWNLOAD)/hashdbUsersManual.pdf

.PHONY: clean distclean install uninstall win_tests

