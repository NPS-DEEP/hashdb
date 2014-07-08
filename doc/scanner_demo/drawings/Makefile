drawings = \
	scan_hashdb.pdf \
	source_lookup.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
