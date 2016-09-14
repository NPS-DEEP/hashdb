                     Announcing hashdb 3.0.0
                       September 13, 2016

                          RELEASE NOTES

hashdb Version 3.0.0 has been released for Linux, MacOS and Windows.

## Changes
*hashdb* 3.0.0 provides significant improvements over previous versions:

* Database ingest operations and media scan operations are now built into *hashdb*.  *bulk_extractor* is no longer required.
* *hashdb* functions are now available using **Python** and **C++** library interfaces.
* Data stores are optimized for speed and compaction by implementing a hybrid B-Tree and list storage implementation.
* All DFXML input and output has been replaced with a much more dense JSON syntax, significantly reducing bulk and increasing efficiency of data moving in and out of *hashdb*.
* *hashdb* now scans and imports along a sliding window rather than just at block boundaries, allowing granularity to work with any media type.
* A streaming scan interface is added to maximize scan throughput efficiency.
* Sources are now tracked by file hash rather than by filename, correcting source attribution count and facilitating similarity detection.
* More metadata is tracked to help interpret matches, including tracking entropy and frequency.
* Ingest and scan operations accept uncompressed recursion paths, for example `500-zip-80`.
* Multiple scan modes are provided for customizing the level of scan speed and reported detail.

## Future Work
* To improve database write speed, the LMDB database storage engine may be replaced with an engine that implements localized write updates, likely RocksDB.
* Large databases exceed the size of memory, resulting in performance degradation.  *hashdb* may be updated to support distributed storage. Additionally, *hashdb* may be reworked to support sharding ranges of block hashes across multiple smaller databases.
* More decompression encodings may be added.  Currently, **zip** and **gzip** are supported.
* More image reader types may be added.  Currently, raw files and E01 images may be read.

## Known Limitations
* Python bindings are not built for Macintosh systems because of complications in making SWIG work with Homebrew.

## Resources
* Downloads (Windows installer, source code, users manual): http://digitalcorpora.org/downloads/hashdb/
* GIT repository (for developers): https://github.com/NPS-DEEP/hashdb
* Wiki: https://github.com/NPS-DEEP/hashdb/wiki
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users
* Developer: Bruce Allen bdallen nps edu
