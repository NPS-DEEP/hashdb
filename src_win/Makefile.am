# To cross-compile Windows executables and build the Windows installer:
#   1) run CONFIGURE_F23.bash to configure the MinGW build environment.
#   2) Make sure NSIS is installed.

EXTRA_DIST = \
	build_installer.nsi \
	EnvVarUpdate.nsi

PDFDOCS = hashdbUsersManual.pdf

EXES = hashdb.exe

HASHDBVERSION=`grep ^AC_INIT $(top_srcdir)/configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

UNVERSIONED_INSTALLER = hashdb-windowsinstaler.exe
INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

all: $(UNVERSIONED_INSTALLER)

# we copy files here since NSIS likes to work from the .nsi directory
$(UNVERSIONED_INSTALLER): ${top_srcdir}/src_win/build_installer.nsi \
                          ${top_srcdir}/src_win/EnvVarUpdate.nsi \
                          ${top_srcdir}/python_bindings/test_hashdb.py \
                          hashdb_um.pdf \
                          ../src/hashdb.exe \
                          _hashdb.pyd
	rm -rf build_installer.nsi EnvVarUpdate.nsi test_hashdb.py hashdb.exe
	ln -s ${top_srcdir}/src_win/build_installer.nsi .
	ln -s ${top_srcdir}/src_win/EnvVarUpdate.nsi .
	ln -s ${top_srcdir}/python_bindings/test_hashdb.py .
	ln -s ../src/hashdb.exe .
	makensis -NOCD -DVERSION=$(HASHDBVERSION) build_installer.nsi
	rm build_installer.nsi EnvVarUpdate.nsi test_hashdb.py hashdb.exe
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)

# ############################################################
# hashdb UM
# get hashdb UM from URL rather than building it to prevent
# unnecessary date change and to simplify the build.
UM_URL = http://digitalcorpora.org/downloads/hashdb
hashdb_um.pdf:
	wget $(UM_URL)/hashdb_um.pdf

# ############################################################
# hashdb Python binding

# constants used for preparing Python parts to compile and link against
MSI_URL_PATH=https://www.python.org/ftp/python/2.7.6
DLL_TOOL=/usr/x86_64-w64-mingw32/bin/dlltool
MSI_FILE=python-2.7.6.amd64.msi
DLL_FILE=python27.dll
DEF_FILE=python27.def
LIB_FILE=libpython27.a

# the hashdb DLL
_hashdb.pyd: ../src_libhashdb/libhashdb.o py_parts/$(LIB_FILE) hashdb_wrap.o
	$(CXX) -Wall -Wextra -shared -o _hashdb.pyd \
	       -static-libgcc -static-libstdc++ \
	       ../src_libhashdb/libhashdb.o \
	       ../src_libhashdb/liblmdb/liblmdb_la-mdb.o \
	       ../src_libhashdb/liblmdb/liblmdb_la-midl.o \
	       ../src_libhashdb/hex_helper.o \
	       ../src_libhashdb/ingest.o \
	       ../src_libhashdb/dig.o \
	       hashdb_wrap.o \
	       ${libdir}/libwinpthread.a \
	       py_parts/$(LIB_FILE)

# the Python parts to compile and link against
py_parts/$(LIB_FILE):
	rm -rf py_parts
	mkdir py_parts
	cd py_parts; wget $(MSI_URL_PATH)/$(MSI_FILE)
	cd py_parts; cabextract $(MSI_FILE) -F '*.h'
	cd py_parts; cabextract $(MSI_FILE) -F $(DLL_FILE)
	cd py_parts; pexports -v $(DLL_FILE) > $(DEF_FILE)
	cd py_parts; $(DLL_TOOL) -A -dllname $(DLL_FILE) --def $(DEF_FILE) \
	             --output-lib $(LIB_FILE)

# the generated hashdb_wrap.cpp and hashdb.py files, both made by SWIG
# note that -D_hypot=hypot addresses a name clash between Python and C++11.
hashdb_wrap.cpp: ${top_srcdir}/python_bindings/hashdb.i \
	         ${top_srcdir}/src_libhashdb/hashdb.hpp
	ln -s ${top_srcdir}/src_libhashdb/hashdb.hpp .
	/usr/bin/swig -c++ -python -threads -o hashdb_wrap.cpp ${top_srcdir}/python_bindings/hashdb.i
	rm hashdb.hpp
	$(CC) -std=c++11 -Ipy_parts -I${top_srcdir}/src_libhashdb -c hashdb_wrap.cpp -D_hypot=hypot -D MS_WIN64

# the C++ wrapper around libhashdb
hashdb_wrap.o: ${top_srcdir}/python_bindings/hashdb.i ../src_libhashdb/libhashdb.o hashdb_wrap.cpp

# ############################################################
# distribution wrapped in a .zip
windist:
	rm -rf $(distdir).zip
	mkdir $(distdir)
	@echo
	cp ../src/hashdb64.exe $(distdir)
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

