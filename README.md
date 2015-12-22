HASHDB
======
Welcome to hashdb!

The hashdb tools are used for finding blacklist data in raw media
by using cryptographic hashes calculated from hash blocks.

The toolset provides facilities for creating hash databases
of MD5 hashes on files aligned along hash block boundaries as well as querying
hash databases, merging hash databases, and performing hash lookups.

The hashdb toolset includes the hashdb tool, hashdb library file
libhashdb.a, and header file hashdb.hpp.

hashdb builds and installs on Linux and OS X systems using
configure; make; make install.  hashdb also cross-compiles to Windows
from Fedora 20+ using mingw.

Web links
----------
* Installing hashdb: https://github.com/NPS-DEEP/hashdb/wiki/Installing-hashdb
* hashdb home page: https://github.com/NPS-DEEP/hashdb/wiki

Bugs
----
Plese enter bugs on the [github issue tracker](https://github.com/NPS-DEEP/hashdb/issues?state=open)

Maintainer
----------
Bruce Allen <bdallen@nps.edu>

License
-------
Please see [Licence](https://github.com/NPS-DEEP/hashdb/wiki/License)

Program Documentation
---------------------
To generate the program documentation with Doxygen, please type:

    cd doc/doxygen
    make

