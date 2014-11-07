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

# Functional Changes
* Several changes are made to the `scan_expanded` command to improve usability:

 * Hashes are scanned immediately as they are parsed from DFXML input rather than saving them up in a vector and hashing the vector when the `</fileobject>` close tab is received.  Because of this, a fileobject's filename must be defined before its block hashes, or else the printed name of the file being processed will be blank.
 * A `-m <max>` option is added so that when the number of sources a hash has exceeds this maximum, the sources associated with the hash are not displayed.
 * All the sources for a hash are displayed on one line instead of printing each source for a hash on a separate line with the hash value displayed at the beginning.
 * A new `source_list_id` field is added to provide the ability to easily distinguish between various source lists.  The `source_list_id` value is calculated by applying a 64-bit hash for each `source_id` associated with the given hash.
 * Maybe: skip printing the source list if the `source_list_id` has been seen before.


Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users
