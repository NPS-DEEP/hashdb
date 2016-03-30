#!/usr/bin/env python
# Checks the environment for the expected version of Python
# then tries to access the hashdb Python module.

import sys
is_okay = True

# Python must be version 2.7
if sys.version_info.major != 2 and sys.version_info.minor != 7:
    print("Invalid Python version: 2.7 is required.")
    print(sys.version_info)
    is_okay = False

# Python must be 64-bit
if sys.maxsize != 2**63 - 1:
    print("Invalid Python configuration: 64-bit Python is required.")
    print("found %d but expected %d" % (sys.maxsize, 2**64))
    is_okay = False

    raise "Requires Python 2.7."

if is_okay == False:
    sys.exit(1)

# now access the hashdb module and print the version
import hashdb
hashdb_version = hashdb.version()
print("hashdb version: %s") % hashdb_version

