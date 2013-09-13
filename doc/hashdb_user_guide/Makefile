PARTS=hashdb_user_guide.tex drawings/create_hashdb.pdf drawings/scan_hashdb.pdf drawings/sector_boundaries.pdf

hashdb_user_guide.pdf: $(PARTS)
	pdflatex hashdb_user_guide
	pdflatex hashdb_user_guide

clean:
	rm -f *.{aux,bbl,blg,log,out,toc} *~
	rm -f hashdb_user_guide.pdf
