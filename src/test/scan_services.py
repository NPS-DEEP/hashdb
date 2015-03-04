#!/usr/bin/env python3
#
# Test scan services

import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"
db2 = "temp_2.hdb"
xml1 = "temp_1.xml"

def setup():
    # create DFXML with hsah values 00, 11, 22
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.rm_tempfile(xml1)
    H.write_temp_dfxml_hash(byte_run_hashdigest="00", byte_run_file_offset=1*4096)
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="11", byte_run_file_offset=2*4096)
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="22", byte_run_file_offset=3*4096)
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["export", db1, xml1])

    # create DB with two entries of hash 00 and one entry of hash 11
    # to provide two duplicate hash entries and two duplicate source entries
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00", repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00", repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="11", repository_name="r2", byte_run_hash_label="L")
    H.hashdb(["import", db1, "temp_dfxml_hash"])

def test_scan():
    lines = H.hashdb(["scan", db1, xml1])
    #print(*lines, sep='\n')
    H.str_equals(lines[4], '["00",{"count":2}]')
    H.str_equals(lines[7], '["11",{"count":1}]')
    H.int_equals(len(lines), 10)

def test_scan_hash():
    # hash present
    lines = H.hashdb(["scan_hash", db1, "00"])
    H.str_equals(lines[0], '["00",{"count":2}]')
    H.int_equals(len(lines), 2)

    # hash not present
    lines = H.hashdb(["scan_hash", db1, "33"])
    H.str_equals(lines[0], '["33",{"count":0}]')
    H.int_equals(len(lines), 2)

def test_scan_expanded():
    lines = H.hashdb(["scan_expanded", db1, xml1])
    print(*lines, sep='\n')
    H.str_equals(lines[4], '{"block_hashdigest":"00", "count":2, "source_list_id":1104745215, "sources":[{"source_id":1,"file_offset":0,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"},{"source_id":2,"file_offset":0,"repository_name":"r2","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}]}')
    H.str_equals(lines[7], '{"block_hashdigest":"11", "count":1, "label":"L", "source_list_id":3098726271, "sources":[{"source_id":2,"file_offset":0}]}')
    H.int_equals(len(lines), 10)


if __name__=="__main__":
    setup()
    test_scan()
    test_scan_hash()
    test_scan_expanded()
    print("Test Done.")

