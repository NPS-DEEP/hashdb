
all:design.pdf

design.pdf: design.tex design.bib
	pdflatex design
	bibtex design
	pdflatex design
	pdflatex design

force:
	pdflatex design
	bibtex design
	pdflatex design
	pdflatex design

clean:
	rm -rf design.aux design.log design.out design.pdf

.PHONY: force

