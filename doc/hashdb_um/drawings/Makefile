drawings = \
	hashdb_system_overview.pdf \
	hash_interval.pdf \
	default_hash_interval.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
