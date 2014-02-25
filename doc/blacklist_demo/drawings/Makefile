drawings = \
	md5deep.pdf \
	import.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
