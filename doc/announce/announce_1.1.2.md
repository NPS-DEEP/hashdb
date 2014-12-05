                     Announcing hashdb 1.1.2
                        31, 2014

                          RELEASE NOTES

hashdb Version 1.1.2 has been released for Linux, MacOS and Windows.

Release source code and Windows installer: http://digitalcorpora.org/downloads/bulk_extractor/

GIT repository: https://github.com/simsong/bulk_extractor

#Bug Fixes

* Add the report of database changes to the destination database for the `add_multiple` command.  Database changes should be reported for all commands that change the database.  This was an omission.
* Database manipulation commands are fixed to additionally copy source metadata.  Previously, source metadata information (the filesize and the file hashdigest) was not copied.  This was an omission.
* A regression introduced in 1.1.1 was fixed so that command `scan_expanded` does not require the input to have file hashdigest or filesize tags in order to hash.
* _hashdb_ fails more gracefully and suggests running the `upgrade` command when the source metadata store is missing.
* _hashdb_ fails more gracefully when attempting to access a corrupted hash database.  For example a hash database is corrupted when a  _hashdb_ command that modifies it is aborted.
* Fix regression in the `scan_random` performance analysis command that made it fail to scan the database copy.
* Simplify the `scan_random` command to not require a database copy.  Python reporting about the copy, now removed, was broken.
* Fix hashdb API interface `import_metadata` to add metadata to a source even if hashes have not been added to the source yet.
This deficiency can result in missing source metadata.
The similar hashdb `import` command does not have this deficiency.

# Functional Changes
* Because output can be so large, the output from commands that print expanded source information has been reduced by not printing information multiple times.  Specifically:
 * Source information for a given source ID is printed only once.
 * The source list associated with a given hash value is printed only once.
 * A `-m <max>` option is added so that when the number of sources a hash has exceeds this maximum, the sources associated with the hash are not displayed. 
* A new `source_list_id` field is added for commands that print source information to provide the ability to distinguish between various source lists.  The `source_list_id` value is calculated by applying a 32-bit CRC function for each `source_id` associated with the given hash.  The CRC function is `boost::crc_32_type` from `boost/crc.hpp`.
* The `hash_table` command is changed to look up sources by source index rather than by the repository name, filename text pair.
This change was made because:
 * It is much easier to enter the index value rather than the text.
 * On Windows systems, not all text can be typed, rendering the command useless.
 * The required source index value is readily available in the `source_id` fields of some commands, in particular the `sources` command.
* Several changes are made to the `scan_expanded` command to improve usability:
 * Hashes are scanned immediately as they are parsed from DFXML input rather than saving them up in a vector and hashing the vector when the `</fileobject>` close tab is received.  Because of this, a fileobject's filename must be defined before its block hashes, or else the printed name of the file being processed will be blank.
 * All the sources for a hash are displayed on one line instead of printing each source for a hash on a separate line with the hash value displayed at the beginning.
* New command `import_tab [-r <repository name>] [-s <sector size>] <hashdb> <tab file>` is added to support importing hashes from tab-delimited text files.  Tab-delimited import files contain lines of block hashes, where each line consists of a filename, a tab, a block hash, and a sector index starting at 1, followed by a carriage return.  Comment lines are allowed by starting them with the # character. 
For example input line:

        file1	63641a3c008a3d26a192c778dd088868	1

 will import block hash `63641...` at file byte offset `0` for file `file1`.

 This tab-delimited syntax is compatible with  Sector Hash Datasets being made available by NIST.
* New hashdb library API interface `open_scan_pthread` is added.  This mode opens hashdb for scanning with a separate scan resource per thread instead of opening for scanning with a lock around one scan resource, as is done with the existing `open_scan` interface.
This interface is recommended for scan queries that run in a multi-threaded environment.
* Commands that return outupt also include header information about the command including the command typed to generate the output.
* Regression tests are added for validating output for comands that generate output.
* For completeness, the database changes displayed during a change operation include changes with count zero.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users 
