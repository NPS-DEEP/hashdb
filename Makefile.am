ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src src/libhashdb src/hashdb_checker src_win man doc

# don't include bootstrap. People run it, and they shoudln't
# It's only for people who check out the git repo
EXTRA_DIST = \
	ROADMAP.txt \
        m4/ax_boost_base.m4 \
        m4/ax_boost_filesystem.m4 \
        m4/ax_boost_system.m4 \
        m4/ax_pthread.m4

RELEASE	     = $(PACKAGE)-$(VERSION)

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win32
win32: win32/bulk_extractor32.exe

.PHONY: win64
win64: win64/bulk_extractor64.exe

win32/bulk_extractor32.exe:
	@echo '****************************************************'
	@echo '*** making win32 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win32  ]; then mkdir win32 ; fi
	if [ ! -r win32/Makefile ]; then (cd win32;mingw32-configure) ; fi
	(cd win32;$(MAKE))
	(cp win32/src/hashdb_manager.exe win32/hashdb_manager32.exe)
	(cp win32/src/hashdb_checker/hashdb_checker.exe win32/hashdb_checker32.exe)
	ls -l win32/*.exe

win64/bulk_extractor64.exe:
	@echo '****************************************************'
	@echo '*** making win64 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure) ; fi
	(cd win64;$(MAKE))
	(cp win64/src/hashdb_manager.exe win64/hashdb_manager64.exe)
	(cp win64/src/hashdb_checker/hashdb_checker.exe win64/hashdb_checker64.exe)
	ls -l win64/*.exe

# windist makes win32 and win64 .exe files and puts them in a zip file
# NOTE: this is no longer preferable since we have a hashdb windows installer
#       in src_win
windist: win32 win64
	rm -rf $(distdir).zip
	mkdir $(distdir)
	@echo
	cp win32/hashdb_manager32.exe $(distdir)
	cp win32/hashdb_checker32.exe $(distdir)
	cp win64/hashdb_manager64.exe $(distdir)
	cp win64/hashdb_checker64.exe $(distdir)
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

.PHONY: exels execlean win32 win64 windist

