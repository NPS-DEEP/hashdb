DEPENDENCIES = hashdb.hpp hashdbScannerUsage.txt hashdbUsage.txt \
     hashdbUsersManual.bbl \
     hashdbUsersManual.tex hashdbWorkedExamplesStandalone.tex \
     hashdbWorkedExamples.tex Makefile references.bib \
     WindowsInstaller.png windowsWarning.png drawings/* \
     hashdb_quick_reference.pdf

DOWNLOAD = http://digitalcorpora.org/downloads/hashdb

hashdbUsersManual.pdf: $(DEPENDENCIES)
	cd drawings; make
	pdflatex hashdbUsersManual.tex
	pdflatex hashdbUsersManual.tex

################################################################
##
## Get the PDF documentation from the download site
##
hashdb_quick_reference.pdf:
	wget $(DOWNLOAD)/hashdb_quick_reference.pdf

clean:
	rm -rf hashdbUsersManual.aux hashdbUsersManual.log hashdbUsersManual.out hashdbUsersManual.pdf hashdbUsersManual.toc

