ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src src/libhashdb src/hashdb_checker doc

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
	cp src/hashdb_manager.exe win64/hashdb_manager.exe
	cp src/hashdb_checker/hashdb_checker.exe win64/hashdb_checker.exe

.PHONY: win32 win64
