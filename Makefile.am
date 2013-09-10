ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src src/libhashdb src/hashdb_checker src_win man doc

EXTRA_DIST = bootstrap.sh

RELEASE	     = $(PACKAGE)-$(VERSION)

win32:
	rm -rf win32
	mkdir win32
	make distclean
	mingw32-configure
	make
	sudo make install
	cp src/hashdb_manager.exe win32/hashdb_manager32.exe
	cp src/hashdb_checker/hashdb_checker.exe win32/hashdb_checker32.exe

win64:
	rm -rf win64
	mkdir win64
	make distclean
	mingw64-configure
	make
	sudo make install
	cp src/hashdb_manager.exe win64/hashdb_manager64.exe
	cp src/hashdb_checker/hashdb_checker.exe win64/hashdb_checker64.exe

# useful during development when mingw configure is already set up
win32_dev:
	make
	cp src/hashdb_manager.exe win32/hashdb_manager32.exe
	cp src/hashdb_checker/hashdb_checker.exe win32/hashdb_checker32.exe

win64_dev:
	make
	cp src/hashdb_manager.exe win64/hashdb_manager64.exe
	cp src/hashdb_checker/hashdb_checker.exe win64/hashdb_checker64.exe

# windist makes win32 and win64 .exe files and puts them in a zip file
windist: win32 win64
	rm -rf $(distdir).zip $(distdir) src/*.exe
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

.PHONY: win32 win64 win32_dev win64_dev windist

