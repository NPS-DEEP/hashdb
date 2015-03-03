                     Announcing hashdb 2.0.0
                        February 20, 2015

                          RELEASE NOTES (DRAFT)

hashdb Version 2.0.0 has been released for Linux, MacOS and Windows.

# Functional Changes
_hashdb_ Version 2.0.0 introduces new capability:

* Faster performance.  The database storeage system has been replaced with LMDB, see http://symas.com/mdb/doc/index.html.  As a result:
 * Performance is faster than the previous B-Tree implementation.
 * The database size is slightly larger.
 * LMDB is memory-mapped.
 * LMDB read operations are performed concurrently without requiring locks.
 * Hash sizes are no longer fixed, allowing _hashdb_ to store hashes of any size.

* New byte alignment option `-a` has been added to allow storing hashes generated starting at any byte, not just on 512-byte aligned boundaries.

* New hash truncation option `-t` has been added to allow storing truncated block hash values, enabling hash databases to be smaller.

* An optional hash label may be stored with each hash to serve as a source classifier.
 * The optional label may be imported from DFXML as text in the new `label` attribute of the `byte_run` tag, for example `<byte_run file_offset='0' len='4096' label="L"` where `L` might indicate that the data has low entropy.
 * The optional label may be imported using new field `hash_label` in the _hashdb_ API.  The _bulk\_extractor_ scanner may use this field for classifying hashes.
 * When scanning for matching hashes, _hashdb_ displays label information, if present, in JSON format, for example: `{"count":2, "label":"L"}`.
* _hashdb_ dependency on Boost was removed.  This impacts _hashdb_ several ways:
 * By not relying on Boost, it is now easier to install _hashdb_ on various systems.
 * Client/Server code which required `Boost::asio` was removed.
It is possible that in a future version of _hashdb_, equivalent client/server capability may be provided using Python and the Python Twisted module.
 * Invalid input values will use 0 instead of failing.
 * _hashdb_ now uses CRC code from http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c rather than from `Boost::crc`.

* New command `subtract_repository` was added to allow removal of specific repositories.  Before, this functionality was achieved by iteratively using the `add_repository` command.  This addition fills a functional deficiency.

* Additional test modules were added to help validate operational integrity.

_hashdb_ Version 2.0.0 is not compatible with previous versions of _hashdb_.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb
* Wiki: https://github.com/simsong/hashdb/wiki

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users
