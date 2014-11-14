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
* _hashdb_ fails more gracefully when attempting to access a corrupted hash database.  A hash database can be corrupted when a  _hashdb_ command that modifies it is aborted.

# Functional Changes
* Several changes are made to the `scan_expanded` command to improve usability:

 * Hashes are scanned immediately as they are parsed from DFXML input rather than saving them up in a vector and hashing the vector when the `</fileobject>` close tab is received.  Because of this, a fileobject's filename must be defined before its block hashes, or else the printed name of the file being processed will be blank.
 * A `-m <max>` option is added so that when the number of sources a hash has exceeds this maximum, the sources associated with the hash are not displayed.
 * All the sources for a hash are displayed on one line instead of printing each source for a hash on a separate line with the hash value displayed at the beginning.
 * A new `source_list_id` field is added to provide the ability to easily distinguish between various source lists.  The `source_list_id` value is calculated by applying a 32-bit CRC function for each `source_id` associated with the given hash.  The CRC function is `boost::crc_32_type` from `boost/crc.hpp`.
 * If a particular source list has more than one entry, it is printed only once.  This is managed by tracking source list IDs with count>1 in a set.
* New command `import_tab [-r <repository name>] [-s <sector size>] <hashdb> <tab file>` is added to support importing hashes from tab-delimited text files.  Tab-delimited import files contain lines of block hashes, where each line consists of a filename, a tab, a block hash, and a sector index starting at 1, followed by a carriage return.  Comment lines are allowed by starting them with the # character. 
For example input line:

        file1	63641a3c008a3d26a192c778dd088868	1

 will import block hash `63641...` at file offset `0` for file `file1`.

 This tab-delimited syntax is compatible with  Sector Hash Datasets being made available by NIST.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users 

