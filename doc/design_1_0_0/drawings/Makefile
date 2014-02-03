drawings = \
	hash_db.pdf \
	hash_duplicates_store.pdf \
	hash_store.pdf \
	source_lookup_store.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
