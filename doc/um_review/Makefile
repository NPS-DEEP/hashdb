drafts = draft1.pdf draft1b.pdf draft2.pdf

.SUFFIXES: .tex .pdf

.tex.pdf:
	pdflatex $<

all: $(drafts)

zip:
	rm -rf draft_mail
	mkdir draft_mail
	cp *.tex draft_mail
	cp *.pdf draft_mail
	cp Makefile draft_mail
	cp README draft_mail
	zip -r9 draft_mail.zip draft_mail

clean:
	rm -f *.pdf *.log *.aux *.out

.PHONY: zip clean
