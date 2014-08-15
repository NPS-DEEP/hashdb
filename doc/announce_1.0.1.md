		Announcing hashdb 1.0.1
		        <DATE>

hashdb Version 1.0.1 has been released for Linux, MacOS and Windows.

Major improvements
==================

Bug Fixes
=========
* Improve error detection and reporting in the event of invalid input
    * Discontinued command options -t, -a, and -b are removed.  Use of these options now provide an error rather than quietly disregarding their use.
    * Improve wording for the change report written to stdout.
    * Detect when the database configuration is different from the hashdb configuration, and fail gracefully.
    * Detect when multiple databases being referenced are not compatible with each other, specifically, if their hash block size is different or if the databases is the same one.
* Fix so test suite builds for Windows.
* Fix the command test so that it does not add a database to itself.  This is an error condition and it can corrupt the database.
* Add the Users Manual to the distribution.
* Organize the commands in the usage text into categories, and fix and clarify usage language.
* Set endian module to revision sha1=05ac935 since the newer endian module is not compatible with available Boost versions.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users

