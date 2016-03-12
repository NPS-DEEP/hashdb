# To cross-compile Windows executables and build the Windows installer:
#   1) Make sure MinGW is configured, see
#      bulk_extractor/src_win/CONFIGURE_F20.sh configure script
#   2) Make sure the Windows configuration is built at hashdb/win64:
#      mkdir win64; cd win64; mingw64-configure; make
#   3) Now build the Windows installer from this directory:
#      make installer
#
# 1) Run the latest bulk_extractor/src_win/CONFIGURE_F20.sh configure script
# to set up Fedora with the mingw environment required for cross-compiling
# to Windows.  Please obtain the CONFIGURE_F20.sh script from
# http://github.com/simsong/bulk_extractor.
#
# 2) type "make" to build the Windows installer.

EXTRA_DIST = \
	build_installer.nsi \
	EnvVarUpdate.nsi \
	run_tests.bat \
	Makefile

PDFDOCS = hashdbUsersManual.pdf

HASHDBVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

INSTALLER = hashdb-$(HASHDBVERSION)-windowsinstaller.exe

installer: $(INSTALLER)

$(INSTALLER): build_installer.nsi EnvVarUpdate.nsi $(PDFDOCS)
	@echo Making hashdb version $(HASHDBVERSION) Windows Installer
	makensis -DVERSION=$(HASHDBVERSION) build_installer.nsi
