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
	cp src/hashdb_manager.exe win32/hashdb_manager.exe
	cp src/hashdb_checker.exe win32/hashdb_checker.exe

#	cp src/libhashdb/.libs/libhashdb.a win32/libhashdb.a
#	cp src/libhashdb/.libs/libhashdb.la win32/libhashdb.la

win64:
	rm -rf win64
	mkdir win64
	make distclean
	mingw64-configure
	make
	sudo make install
	cp src/hashdb_manager.exe win64/hashdb_manager.exe
	cp src/hashdb_checker.exe win64/hashdb_checker.exe

#	cp src/libhashdb/.libs/libhashdb.a win64/libhashdb.a
#	cp src/libhashdb/.libs/libhashdb.la win64/libhashdb.la

.PHONY: win32 win64
