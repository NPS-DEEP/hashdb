#!/usr/bin/env python3
#
# Test the Import Export command group

import subprocess
#import xml.etree.ElementTree as ET
import shutil
import hashdb_helpers as H
import os

db1 = "temp_1.hdb"
xml1 = "temp_1.xml"

# test import
def test_import():

    # default
    shutil.rmtree(db1, True)
    if os.path.exists(xml1):
        os.remove(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash()
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals()

if __name__=="__main__":
    test_import()
    print("Test Done.")

