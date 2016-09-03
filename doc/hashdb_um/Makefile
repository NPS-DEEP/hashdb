UM_FILES = ../../src_libhashdb/hashdb.hpp hashdb_usage.txt \
     hashdb_um.bbl \
     hashdb_um.tex title.tex hashdbWorkedExamplesStandalone.tex \
     hashdbWorkedExamples.tex Makefile references.bib \
     WindowsInstaller.png windowsWarning.png drawings/* \
     hashdb_quick_reference_text.tex

QUICK_REFERENCE_FILES = hashdb_quick_reference.tex \
     hashdb_quick_reference_text.tex

all: hashdb_um.pdf hashdb_quick_reference.pdf

hashdb_um.pdf: $(UM_FILES) hashdb_quick_reference_text.tex
	cd drawings; make
	pdflatex hashdb_um.tex
	pdflatex hashdb_um.tex

hashdb_quick_reference.pdf: $(QUICK_REFERENCE_FILES)
	pdflatex hashdb_quick_reference.tex

clean:
	rm -rf hashdb_um.aux hashdb_um.log hashdb_um.out hashdb_um.pdf \
               hashdb_um.toc
	rm -rf hashdb_quick_reference.aux hashdb_quick_reference.log hashdb_quick_reference.out hashdb_quick_reference.pdf

.PHONY: clean

