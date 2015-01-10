                     Announcing hashdb 1.2.0
                        , 2015

                          RELEASE NOTES

hashdb Version 1.2.0 has been released for Linux, MacOS and Windows.

Release source code and Windows installer: http://digitalcorpora.org/downloads/bulk_extractor/

GIT repository: https://github.com/simsong/bulk_extractor

#Bug Fixes
* Performance of the `rebuild_bloom` command is improved when the filter is being disabled.

# Functional Changes
* Replace hash store B-Tree database with LMDB database,
see http://symas.com/mdb/doc/index.html.
Note:
 * Performance is faster than the previous B-Tree implementation.
 * LMDB requires about 50 bytes per hash instead of about 35 bytes per hash.
 * LMDB is memory-mapped.
 * LMDB read operations are performed concurrently without a lock.
 * Source lookup and metadata lookup databases still use B-Tree.

* The hashdb library API interface `open_scan_pthread` added in v1.1.2 was removed.  It provided locked threaded B-Tree access but it did not improve performance.  LMDB does not use locks for reading.  hashdb locks LMDB write accesses.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users
