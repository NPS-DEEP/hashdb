                     Announcing hashdb 2.1.0
                         <date>

                          RELEASE NOTES (DRAFT)

hashdb Version 2.1.0 has been released for Linux, MacOS and Windows.

# Changes in 2.1.0 over 2.0.1
## Functional changes
* The storage and the reporting of entropy labels is improved.
 Previously, entropy label flags calculated from blocks being hashed
 were stored with source data for each source imported
 and were reported with each source.
 Entropy label flags are now correctly stored and reported with their
 associated hash.

 Storing entropy labels with sources rather than hashes caused several
 problems:
 * Labels were stored redundantly for each source of a hash.
 * If the entropy label algorithm changed, labels became inconsistent.

 The following functional changes result from this change:
 * JSON output provides entropy labels for hashes rather than for each source
 associated with each hash.
 * The `import` and `export` commands now import and export using JSON
 rather than DFXML.  The JSON syntax is hierarchical and does not
 produce redundant information, which makes it more compact.
 The DFXML syntax provides one hash and source pair per line,
 which requires duplicate output, is not hierarchical, and is bulky.
 * The existing `import` command is renamed to `import_dfxml`.
 It is retained to provide continued support for importing from tools
 such as `md5deep` which generate DFXML output.
 * If the entropy label algorithm changes, old labels are replaced.

 This change affects the internal operation of hashdb as follows:
 * hashdb contains a new and separate LMDB storage Map
 called `lmdb_flag_store` for storing flags for hashes.
 * The `lmdb_flag_store` is a Map (not Multimap) of key=hash and value=flag.
 * The `lmdb_flag_store` will not be a massive database, for example
 it should easily be less than 1% the size of the hash store.

* DB Add failure is now detected at the point the LMDB transaction is committed
rather than verifying the transaction afterwords.

* Output from commands that produce expanded source output for a hash
 are changed to not re-print source output for the same hash when scanned later.
 This results in more compact output.  Measurements indicate that expanding
 `identified_blocks.txt` using the `expand_identified_blocks` command
 now results in expanded output that is about two times larger
 rather than about 33 times larger.

 Any tools that parse this expanded output will need to cache
 the source information since it will not be included
 the next time that hash is matched.

## Bug fix
* The Windows installer is corrected to install to the 64-bit executable directory rather than to the 32-bit directory.

* The displayed number of items being processed in the `add_multiple` command
 is corrected to reflect the sum of the sizes of the input databases
 rather than just the size of the first input database.

* The `-m` option in the `expand_identified_blocks` and
 `explain_identified_blocks` commands now accepts
 a value of zero to enable selecting no maximum.


## Data Storage Changes
Data storage changes result in a significant changes to the *hashdb* data store and the *hashdb* library API.  These are the specific changes to the data store:

* Do not re-count hashes from identical sources.
* Track the count of hashes actually imported for a source.

To support this, *hashdb* databases now store the following data structures:

* *hashes* - A LMDB multimap of `key=MD5, value=(source ID, source offset)`.
* *hash_labels* - A LMDB map of `key=MD5, value=label` for keys where labels are produced.
* *source_id_to_file_hash* - A SQL lookup map of `key=source_id, value=file_MD5`.
* *source_metadata* - A SQL lookup of `key=file_MD5, value=(source_id, filesize, import_count)`.
* *sources* - A SQL lookup multimap of `key=fileMD5, value=(repository_name, filename`.

The *hashdb* `import` API is now as follows:

* `bool has_file_hash(file_MD5)` - used to detect if this file has already been imported.
* `import_hashes(file_md5, repository_name, filename, file_size, vector(file_offset, block_hash, label="")` - used to import all interesting hashes for a new file_md5.
* `import_alternate_source(file_md5, repository_name, filename)` - used to map another filename to this file hash.

Impact on existing workflow for `import`:

* A new `import <directory path> [--whitelist <hashdb>] [--ignore_low_entropy]` *hashdb* command is added to import hashes from files in directories.  *hashdb* delegates files to threads for calculating MD5s and importing.  This is the primary means for importing.
* The bulk_extractor hashdb import scanner will be refitted to use these interfaces.
* The *md5deep* Import capability will be discontinued but can be refitted if desired.
* The NSRL 512-byte block hash import capability can be refitted but since file hashes are unavailable, a file hash of "0" may be substituted to simulate one source.

Impact on existing workflow for `scan`:

* The *bulk_extractor* *hashdb* scanner continues to be the primary mode for scanning.

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
