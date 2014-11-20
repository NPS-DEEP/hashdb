#!/usr/bin/env python3
#
# Test the Import Export command group

import subprocess
import xml.etree.ElementTree as ET
import hashdb_helpers
import sys

db1 = "temp_1.hdb"

# test import
def test_import():

    # create new db
    subprocess.call(["rm", "-rf", db1])
    subprocess.call(["hashdb", "create", db1])

    # import block size 4096
    lines=hashdb_helpers.run_command(["hashdb", "import", db1, "sample_dfxml4096.xml"])

    changes = hashdb_helpers.parse_changes(lines)
#    print("import_export " + str(changes["hashes_inserted"]))

    hashdb_helpers.test_equals(changes["hashes_inserted"], 74)

    # cleanup
    subprocess.call(["rm", "-rf", db1])
    print("Test Done.")

if __name__=="__main__":
    test_import()

