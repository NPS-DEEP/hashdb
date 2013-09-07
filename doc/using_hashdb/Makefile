PARTS=using_hashdb.tex drawings/create_hashdb.pdf drawings/scan_hashdb.pdf drawings/sector_boundaries.pdf

using_hashdb.pdf: $(PARTS)
	pdflatex using_hashdb
	pdflatex using_hashdb

clean:
	rm -f *.{aux,bbl,blg,log,out,toc} *~
	rm -f using_hashdb.pdf
