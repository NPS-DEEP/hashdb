                     Announcing hashdb 1.1.1
                        October 31, 2014

                          RELEASE NOTES

hashdb Version 1.1.1 has been released for Linux, MacOS and Windows.

Release source code and Windows installer: http://digitalcorpora.org/downloads/bulk_extractor/

GIT repository: https://github.com/simsong/bulk_extractor

#Bug Fixes

* The `explain_identified_blocks` command is changed to print source information for hashes in the `identified_blocks.txt` file for sources containing hashes that are not repeated more than a maximum number of times.  It was incorrectly returning hashes not repeated more than a maximum amount.
* The `import` command now fails more gracefully when the DFXML input file is invalid.



Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users

