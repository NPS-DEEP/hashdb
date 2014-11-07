		Announcing hashdb 1.0.0
		     August 5, 2013

hashdb Version 1.0.0 has been released for Linux, MacOS and Windows.

Major improvements
==================
Version 1.0.0 is a significant overhaul from Version 0.9.1. Major improvements in 1.0.0 include:

* The ability to build databases directly from media. Users can now generate sector hashes
of target media and add them to a database in a single operation by using the import
function of the bulk_extractor hashdb scanner. It is no longer necessary to generate an
intermediate dfxml file before adding hashes to a database. This enhancement yields
increased speed and reduced disk space requirements.

* Increased speed, particularly during the database build process. This was achieved by
merging separate map and multimap stores into a single multimap store. Timing analysis
demonstrated that multimap access speed was equivalent to map access speed and that
maintaining separate stores slowed performance when importing hashes. Merging the
stores reduced code complexity and improved timing performance.

* Complete support for set operations on hash databases. hashdb now supports the add_multiple
(i.e. set union) and subtract database manipulation commands in addition to previously
supported intersect command.

* Statistical summaries. Statistics commands were added to allow users to obtain informa-
tion about the contents of the database.
Statistics now available include:
    * The list of repository names and filenames stored in the database.
    * The sizes of internal database tables.
    * Information about the frequency of hash duplicates.
    * The list of all hashes along with source information.

* A simplified user interface. Database creation was moved into its own function and sep-
arated from the copy commands to eliminate the need for complex command syntax.

* A simplified API. The hashdb library API was redesigned to dramatically reduce com-
plexity. It now supports the following two core operations: 1) importing hashes into a
new database and 2) scanning for hashes. Other operations, such as copying databases,
can be performed file system copy or using commands in the hashdb tool.

* Built-in tools for performance testing. hashdb now offers the add_random and scan_random
commands for evaluating the speed of building and scanning against a database, respec-
tively.

* A streamlined data store implementation. Performance analysis on a variety of experi-
mental data store implementations was completed and demonstrated that the btree data
store was optimal. Sub-optimal data store types unordered-hash, red-black-tree, and flat-
sorted-vector were removed.

* Standardization to MD5 as the primary hashing algorithm. hashdb can be rebuilt to store
entries other than MD5 hashes such as actual data or other hash types, but these configura-
tions are incompatible with normal hashdb operation. Simultaneous support for multiple
hashing algorithms is unnecessary for most users and has been removed in exchange for
a smaller application footprint and a more streamlined code base.

* Improved client/server socket backbone. The client/server socket backbone was changed
from using the ZMQ library to the Boost.asio library. This decision was motivated by
unresolved cross-compiler issues encountered with ZMQ, an unresolved resource deal-
location bug, and because support for Boost is easier to maintain than support for the
ZMQ package. As a result of this change, the hashdb environment is easier to prepare.
An additional benefit is that unnecessary data copy operations were removed during the
transition, improving efficiency.

* The “duplicate filename” bug was fixed. This bug caused some duplicate hash values to
not be added.

Availability
============
* Release source code: http://digitalcorpora.org/downloads/hashdb/
* Windows installer: http://digitalcorpora.org/downloads/hashdb/
* GIT repository: https://github.com/simsong/hashdb

Contacts
========
* Developer: mailto:bdallen@nps.edu
* Bulk Extractor Users Group: http://groups.google.com/group/bulk_extractor-users

