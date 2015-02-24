#!/usr/bin/env python3
#
# Test the New Database command group

#from subprocess import call
import subprocess
import shutil
import xml.etree.ElementTree as ET
import hashdb_helpers as H

db1 = "temp_1.hdb"

# check basic settings
def test_basic_settings():
    # remove existing DB
    shutil.rmtree(db1, True)

    # create new DB
    H.hashdb(["create", db1, "-p1024", "-m3", "-a 128", "-t 7", "--bloom=disabled", "--bloom_kM=4:14"])

    # validate settings parameters
    settings = H.parse_settings(db1)
    H.int_equals(settings['settings_version'], 2)
    H.int_equals(settings['byte_alignment'], 128)
    H.int_equals(settings['hash_truncation'], 7)
    H.int_equals(settings['hash_block_size'], 1024)
    H.int_equals(settings['maximum_hash_duplicates'], 3)
    H.bool_equals(settings['bloom_used'], False)
    H.int_equals(settings['bloom_k_hash_functions'], 4)
    H.int_equals(settings['bloom_M_hash_size'], 14)

    # byte alignment boundary
    H.write_temp_dfxml_hash(byte_run_len=1024)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

# hash block size
def test_hash_block_size():
    print("TBD")

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
    #test_hash_block_size()
    test_basic_settings()
    print("Test Done.")

