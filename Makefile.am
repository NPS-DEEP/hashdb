ACLOCAL_AMFLAGS = -I m4

SRC_WIN = src_win
include src_win/Makefile

SUBDIRS = src_libhashdb src rapidjson test python_bindings test_py man doc

SRC_WIN_DIST = \
	src_win/build_installer.nsi \
	src_win/EnvVarUpdate.nsi \
	src_win/run_tests.bat \
	src_win/Makefile \
	src_win/README

# don't include bootstrap. People run it, and they shoudln't
# It's only for people who check out the git repo
EXTRA_DIST = \
        m4/ax_pthread.m4 \
	$(SRC_WIN_DIST)

RELEASE	     = $(PACKAGE)-$(VERSION)

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win64
win64: win64/hashdb.exe

win64/hashdb.exe:
	@echo '****************************************************'
	@echo '*** making win64 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure) ; fi
	(cd win64;$(MAKE))
	(cp win64/src/hashdb.exe win64/hashdb.exe)
	ls -l win64/*.exe

# windist makes the win64 .exe file and puts it in a zip file
# NOTE: this is no longer preferable since we have a hashdb windows installer
#       in src_win
windist: win64
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
	rm -rf $(distdir).tar.gz
	rm -rf $(distdir).zip

.PHONY: exels execlean win64 windist

