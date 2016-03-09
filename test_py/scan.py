#!/usr/bin/env python3
#
# Test the Scan command group

import helpers as H

json_data = ["#","#", \
'{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","nonprobative_count":2,"name_pairs":["r1","f1"]}',
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","nonprobative_count":4,"name_pairs":["r2","f2"]}',
'{"file_hash":"1111111111111111","filesize":5,"file_type":"ftc","nonprobative_count":6,"name_pairs":["r3","f3"]}',
'{"block_hash":"2222222222222222","entropy":7,"block_label":"bl1","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":8,"block_label":"bl2","source_offset_pairs":["0011223344556677",0,"0011223344556677",512,"0000000000000000",0]}',
'{"block_hash":"ffffffffffffffff","entropy":9,"block_label":"bl3","source_offset_pairs":["0011223344556677",1024]}']

def test_scan():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.json", json_data)
    H.hashdb(["import_json", "temp_1.hdb", "temp_1.json"])

    # test values: not present, valid, valid repeat, valid, valid, not valid
    hash_file = ["#","#", \
"# marker 1", \
"fp1	0000000000000000", \
"# marker 2", \
"fp1	2222222222222222", \
"# marker 3", \
"fp2	2222222222222222", \
"fp3	8899aabbccddeeff", \
"fp4	ffffffffffffffff", \
"# marker4", \
"fp4	invalid_hash_value", \
"# marker5"]
    H.make_tempfile("temp_1.txt", hash_file)

    returned_answer = H.hashdb(["scan", "temp_1.hdb", "temp_1.txt"])
    expected_answer = [
'# command: ',
'# hashdb-Version: ',
'#',
'#',
'# marker 1',
'# marker 2',
'fp1	2222222222222222	{"entropy":7,"block_label":"bl1","source_list_id":1303964917,"sources":[{"file_hash":"1111111111111111","filesize":5,"file_type":"ftc","nonprobative_count":6,"name_pairs":["r3","f3"]}],"source_offset_pairs":["1111111111111111",4096]}',
'# marker 3',
'fp2	2222222222222222',
'fp3	8899aabbccddeeff	{"entropy":8,"block_label":"bl2","source_list_id":36745675,"sources":[{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","nonprobative_count":4,"name_pairs":["r2","f2"]},{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","nonprobative_count":2,"name_pairs":["r1","f1"]}],"source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'fp4	ffffffffffffffff	{"entropy":9,"block_label":"bl3","source_list_id":2343118327,"sources":[],"source_offset_pairs":["0011223344556677",1024]}',
'# marker4',
'# marker5',
'# scan completed.',
'']

    H.lines_equals(returned_answer, expected_answer)

def test_scan_hash():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.json", json_data)
    H.hashdb(["import_json", "temp_1.hdb", "temp_1.json"])

    # test individual hash, hash present
    returned_answer = H.hashdb(["scan_hash", "temp_1.hdb", "ffffffffffffffff"])
    H.lines_equals(returned_answer, [
'{"entropy":9,"block_label":"bl3","source_list_id":2343118327,"sources":[{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","nonprobative_count":2,"name_pairs":["r1","f1"]}],"source_offset_pairs":["0011223344556677",1024]}',
''])
    # test individual hash, hash not present
    returned_answer = H.hashdb(["scan_hash", "temp_1.hdb", "0000000000000000"])
    H.lines_equals(returned_answer, [
'Hash not found for \'0000000000000000\'', \
''])

if __name__=="__main__":
    test_scan()
    test_scan_hash()
    print("Test Done.")

