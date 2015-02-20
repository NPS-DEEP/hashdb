#!/usr/bin/env python3
#
# Test the New Database command group

#from subprocess import call
import subprocess
import shutil
import xml.etree.ElementTree as ET
import hashdb_helpers as H

# check that input parameters get into settings.xml
def test_create():
    db1 = "temp_1.hdb"

    # create new db
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1, "-p1024", "-m10", "--bloom=disabled", "--bloom_kM=4:14"])

    settings = H.parse_settings(db1)
    H.int_equals(settings['settings_version'], 2)
    H.int_equals(settings['byte_alignment'], 512)
    H.int_equals(settings['hash_truncation'], 0)
    H.int_equals(settings['hash_block_size'], 1024)
    H.int_equals(settings['maximum_hash_duplicates'], 10)
    H.bool_equals(settings['bloom_used'], False)
    H.int_equals(settings['bloom_k_hash_functions'], 4)
    H.int_equals(settings['bloom_M_hash_size'], 14)

    # cleanup
    shutil.rmtree(db1)

def option_p():
    # only accept 512
    db1 = "temp_1.hdb"
    H.hashdb(["create", db1, "-p512"])
    lines=H.hashdb(["import", db1, "sample_dfxml4096.xml"])
    changes = H.parse_changes(lines)
    H.int_equals(changes["hashes_inserted"], 0)
    lines=H.hashdb(["import", db1, "sample_dfxml512.xml"])
    changes = H.parse_changes(lines)
    H.int_equals(changes["hashes_inserted"], 24)

if __name__=="__main__":
    test_create()
    option_p()
    print("Test Done.")

