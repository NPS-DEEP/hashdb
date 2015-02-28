#!/usr/bin/env python3
#
# Test the Import Export command group

import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"
xml1 = "temp_1.xml"
tab1 = "temp_1.tab"

# test import and export
def test_import_export():

    # default
    shutil.rmtree(db1, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash()
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals()

    # alternate hash block size
    shutil.rmtree(db1, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1, "-p512"])
    H.write_temp_dfxml_hash(byte_run_len=512)
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals(byte_run_len=512)

    # alternate repository name
    shutil.rmtree(db1, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(repository_name="rn")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals(repository_name="rn")

def test_import_tab():
    # default
    shutil.rmtree(db1, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1, "-p512"])
    H.write_temp_tab_hash()
    H.hashdb(["import_tab", "-r", "repositoryname", db1, "temp_tab_hash"])
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals(byte_run_len=512, file_hashdigest_type="", file_hashdigest="")

if __name__=="__main__":
    test_import_export()
    test_import_tab()
    print("Test Done.")

