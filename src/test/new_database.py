#!/usr/bin/env python3
#
# Test the New Database command group

import shutil
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

def test_hash_block_size():
    # create new DB
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])

    # wrong hash block size
    H.write_temp_dfxml_hash(byte_run_len=1024)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 0)
    H.int_equals(changes['hashes_not_inserted_mismatched_hash_block_size'], 1)

def test_max_duplicates():
    # create new DB with max 2
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1, "-m2"])

    # add three entries where only two are allowed
    H.write_temp_dfxml_hash(byte_run_file_offset=4096*1)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.write_temp_dfxml_hash(byte_run_file_offset=4096*2)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.write_temp_dfxml_hash(byte_run_file_offset=4096*3)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 0)
    H.int_equals(changes['hashes_not_inserted_exceeds_max_duplicates'], 1)
    sizes = H.parse_sizes(H.hashdb(["size", db1]))
    H.int_equals(sizes['hash_store_size'], 2)

def test_byte_alignment():
    # create new DB with byte alignment 2
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1, "-a2"])

    # valid
    H.write_temp_dfxml_hash(byte_run_file_offset=6)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

    # invalid
    H.write_temp_dfxml_hash(byte_run_file_offset=7)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 0)
    H.int_equals(changes['hashes_not_inserted_invalid_byte_alignment'], 1)
    
    # valid
    H.write_temp_dfxml_hash(byte_run_file_offset=8)
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

def test_hash_truncation():
    # create new DB with 3 byte hash truncation, no Bloom
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1, "-t3", "--bloom", "disabled"])

    # valid entry
    H.write_temp_dfxml_hash(byte_run_hashdigest='00112233')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

    # duplicate element
    H.write_temp_dfxml_hash(byte_run_hashdigest='00112244')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_not_inserted_duplicate_element'], 1)

    # valid entry
    H.write_temp_dfxml_hash(byte_run_hashdigest='00114433')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

    # create new DB with 3 byte hash truncation, with Bloom
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1, "-t3"])

    # valid entry
    H.write_temp_dfxml_hash(byte_run_hashdigest='00112233')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

    # duplicate element
    H.write_temp_dfxml_hash(byte_run_hashdigest='00112244')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_not_inserted_duplicate_element'], 1)

    # valid entry
    H.write_temp_dfxml_hash(byte_run_hashdigest='00114433')
    changes = H.parse_changes(H.hashdb(["import", db1, "temp_dfxml_hash"]))
    H.int_equals(changes['hashes_inserted'], 1)

#def test_bloom():
#    # no action, see C test

if __name__=="__main__":
    test_basic_settings()
    test_hash_block_size()
    test_max_duplicates()
    test_byte_alignment()
    test_hash_truncation()
    #test_bloom()
    print("Test Done.")

