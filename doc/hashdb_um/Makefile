DEPENDENCIES = ../../src_libhashdb/hashdb.hpp hashdb_be_usage.txt hashdb_usage.txt \
     hashdb_um.bbl \
     hashdb_um.tex hashdbWorkedExamplesStandalone.tex \
     hashdbWorkedExamples.tex Makefile references.bib \
     WindowsInstaller.png windowsWarning.png drawings/* \
     ../hashdb_quick_reference/hashdb_quick_reference_text.tex

hashdb_um.pdf: $(DEPENDENCIES)
	cd drawings; make
	pdflatex hashdb_um.tex
	pdflatex hashdb_um.tex

clean:
	rm -rf hashdb_um.aux hashdb_um.log hashdb_um.out hashdb_um.pdf \
               hashdb_um.toc

