                     Announcing hashdb 2.0.1
                         March 24, 2015

                          RELEASE NOTES

hashdb Version 2.0.1 has been released for Linux, MacOS and Windows.

# 2.0.1 Improvements over Version 2.0.0
* A bug is fixed where on Windows systems, when importing more than 300 million hashes at once, some hashes are silently lost.
The fix is to grow the DB size sooner, when the available page size gets down to 10 instead of down to 2.  Additionally, code is added to detect this failure.

# Functional Changes in v2.0.0
_hashdb_ Version 2.0.0 introduces new capability:

* Faster performance.  The database storage system has been replaced with LMDB, see http://symas.com/mdb/doc/index.html.  As a result:
 * Performance is faster than the previous B-Tree implementation.
 * The database size is slightly larger.
 * LMDB is memory-mapped.
 * LMDB read operations are performed concurrently without requiring locks.
 * Hash sizes are no longer fixed, allowing _hashdb_ to store hashes of any size.

* New byte alignment option `-a` has been added to allow storing hashes generated starting at any byte, not just on 512-byte aligned boundaries.

* New hash truncation option `-t` has been added to allow storing truncated block hash values, enabling hash databases to be smaller in exchange for increased probability of collision.

* An optional label may now be stored with each hash to enable classifying block types.
 * The optional label is exported to and imported from DFXML using the new `label` attribute in the DFXML `byte_run` tag.  For example in tag `<byte_run file_offset='0' len='4096' label="L"`, label value `L` might indicate that the data has low entropy.
 * When scanning for matching hashes, _hashdb_ displays label information, if present, in JSON format.  For example the following JSON might be displayed during a hash match: `{"count":2, "label":"L"}`.
 * _hashdb_ makes no assumptions about the meanings of block classification labels, but the label text should be compatible with JSON and XML without requiring escape codes, and it should be short to prevent undue database bloat.
 * The `label` attribute does not take any space in the database or in DFXML when not used.
 * In the _hashdb_ API, the optional label may be imported using new field `hash_label`, see _hashdb_ header file `hashdb.hpp`.  The _bulk\_extractor_ scanner will use this `hash_label` field to help classify hashes.

* _hashdb_ dependency on Boost was removed.  This impacts _hashdb_ several ways:
 * By not relying on Boost, it is now easier to install _hashdb_ on various systems.
 * Client/Server code which required `Boost::asio` was removed.
It is possible that in a future version of _hashdb_, equivalent client/server capability may be provided using Python and the Python Twisted module.
 * Invalid input values will use 0 instead of failing.
 * The _hshdb_ Bloom Filter now uses `mmap` for POSIX systems and File Mapping for Windows systems instead of Boost for its file mapping.
 * _hashdb_ now uses CRC code from http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c rather than from `Boost::crc`.

* New command `subtract_repository` was added to allow removal of specific repositories.  Before, this functionality was achieved by iteratively using the `add_repository` command.  This addition fills a functional deficiency.

* Calculation of the `source_list_id` value used to identify distinct source groups has been fixed so that IDs of same source groups resolve to the same value.  Before, the value was incorrectly derived by considering sources multiple times or in arbitrary order.  Now, the value is derived by considering sources exactly once, and in order.  

* Performance analysis command `test_random` was improved to be more random on Windows systems, which truncated some bits to 0.

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
