drawings = \
	create_hashdb.pdf \
	query_path.pdf \
	query_socket.pdf \
	scan_hashdb.pdf \
	sector_boundaries.pdf

.SUFFIXES: .svg .pdf

.svg.pdf:
	inkscape --without-gui -D --export-pdf=$@ $<

all: $(drawings)

clean:
	rm -f *.pdf
