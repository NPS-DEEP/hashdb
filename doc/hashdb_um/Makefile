DEPENDENCIES = ../../src/hashdb.hpp hashdbScannerUsage.txt hashdbUsage.txt \
     hashdbUsersManual.bbl \
     hashdbUsersManual.tex hashdbWorkedExamplesStandalone.tex \
     hashdbWorkedExamples.tex Makefile references.bib \
     WindowsInstaller.png windowsWarning.png drawings/* \
     ../hashdb_quick_reference/hashdb_quick_reference_text.tex

hashdbUsersManual.pdf: $(DEPENDENCIES)
	cd drawings; make
	pdflatex hashdbUsersManual.tex
	pdflatex hashdbUsersManual.tex

clean:
	rm -rf hashdbUsersManual.aux hashdbUsersManual.log hashdbUsersManual.out hashdbUsersManual.pdf hashdbUsersManual.toc

