#!/usr/bin/env python3
#
# Test the Import Export command group

import subprocess
import xml.etree.ElementTree as ET
import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"

# test import
def test_import():

    # create new db
    shutil.rmtree(db1, True)
    H.create(db1)

    # import block size 4096
    lines=H.hashdb(["import", db1, "sample_dfxml4096.xml"])

    changes = H.parse_changes(lines)

    H.int_equals(changes["hashes_inserted"], 74)

    # cleanup
    shutil.rmtree(db1)
    print("Test Done.")

if __name__=="__main__":
    test_import()

