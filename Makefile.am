if HAVE_MINGW
SRC_WIN_DIR = src_win
endif

if HAVE_PYTHON
TEST_PY_DIR = test_py
endif

if HAVE_SWIG
PYTHON_BINDINGS_DIR = python_bindings
endif

SUBDIRS = src_libhashdb src doc $(SRC_WIN_DIR) test $(TEST_PY_DIR) \
          $(PYTHON_BINDINGS_DIR) man

RELEASE	     = $(PACKAGE)-$(VERSION)

