drawings = \
	import_hashes.pdf \
	intersect_hashes.pdf \
	subtract_whitelist.pdf \
	print_sources.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
