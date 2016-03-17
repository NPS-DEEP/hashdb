# To cross-compile Windows executables and build the Windows installer:
#   1) Make sure MinGW is configured, see
#      bulk_extractor/src_win/CONFIGURE_F20.sh configure script
#   2) Make sure NSIS is installed.

EXTRA_DIST = \
	build_installer.nsi \
	EnvVarUpdate.nsi \
	run_tests.bat \
	Makefile

PDFDOCS = hashdbUsersManual.pdf

EXES = hashdb.exe

HASHDBVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

UNVERSIONED_INSTALLER = hashdb-windowsinstaler.exe
INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

all: $(UNVERSIONED_INSTALLER)

$(UNVERSIONED_INSTALLER): build_installer.nsi EnvVarUpdate.nsi $(PDFDOCS) \
                          $(EXES)
	makensis -DVERSION=$(HASHDBVERSION) build_installer.nsi
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)

hashdb.exe:
	cp ../src/hashdb.exe .

hashdbUsersManual.pdf:
	echo TBD > hashdbUsersManual.pdf

windist:
	rm -rf $(distdir).zip
	mkdir $(distdir)
	@echo
	cp win64/hashdb64.exe $(distdir)
	@echo ====================================
	@echo 
	@echo Creating ZIP archive
	@echo 
	zip -r9 $(distdir).zip $(distdir)
	@echo ====================================
	@echo 
	@echo Adding text files to $(distdir).zip 
	@echo 
	cp COPYING		$(distdir)/COPYING.txt
	md5deep -r  $(distdir) > md5list.txt
	md5deep -rd $(distdir) > md5list.xml
	mv md5list.txt md5list.xml $(distdir)
	zip --to-crlf $(distdir).zip $(distdir)/*.txt $(distdir)/*.xml

	rm -rf $(distdir)
	@echo "***********************"
	@echo "*** WINDIST IS MADE ***"
	@echo "***********************"
	@echo ""
	ls -l $(distdir).*
	@echo ""
	@unzip -l $(distdir).zip

clean-local:
	rm -rf hashdb.exe hashdbUsersManual.pdf \
               $(INSTALLER) $(UNVERSIONED_INSTALLER)

