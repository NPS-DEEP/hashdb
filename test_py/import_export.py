#!/usr/bin/env python3
#
# Test the Import Export command group

import helpers as H

# test basic DB integrity
def test_import_tab1():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hexdigest> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1",
          "0000000000000000	8899aabbccddeeff	1",
          "0011223344556677	8899aabbccddeeff	2",
          "0011223344556677	ffffffffffffffff	3",
          "1111111111111111	2222222222222222	9",
          "1111111111111111	2222222222222222	9"])
    H.hashdb(["import_tab", "temp_1.hdb", "temp_1.tab"])
    H.hashdb(["export", "temp_1.hdb", "temp_1.json"])

    returned_answer = H.read_file("temp_1.json")
    expected_answer = ["#","#", \
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}',
'{"block_hash":"2222222222222222","entropy":0.0,"block_label":"","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":0.0,"block_label":"","source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'{"block_hash":"ffffffffffffffff","entropy":0.0,"block_label":"","source_offset_pairs":["0011223344556677",1024]}']

    H.lines_equals(returned_answer, expected_answer)

# test repository name option
def test_import_tab2():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hexdigest> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_1.tab"])
    H.hashdb(["export", "temp_1.hdb", "temp_1.json"])

    returned_answer = H.read_file("temp_1.json")
    expected_answer = ["#","#", \
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["r","temp_1.tab"]}',
'{"block_hash":"8899aabbccddeeff","entropy":0.0,"block_label":"","source_offset_pairs":["0011223344556677",0]}']

    H.lines_equals(returned_answer, expected_answer)

# test whitelist directory option
def test_import_tab2():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hexdigest> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_1.tab"])

    H.rm_tempdir("temp_2.hdb")
    H.rm_tempfile("temp_2.json")
    H.hashdb(["create", "temp_2.hdb"])
    H.make_tempfile("temp_2.tab", [
          "# <file hexdigest> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1",
          "0000000000000000	8899aabbccddeeff	1",
          "0011223344556677	8899aabbccddeeff	2",
          "0011223344556677	ffffffffffffffff	3",
          "1111111111111111	2222222222222222	9",
          "1111111111111111	2222222222222222	9"])
    H.hashdb(["import_tab", "-w", "temp_1.hdb", "temp_2.hdb", "temp_2.tab"])
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])

    returned_answer = H.read_file("temp_2.json")
    expected_answer = ["#","#", \
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}',
'{"block_hash":"2222222222222222","entropy":0.0,"block_label":"","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":0.0,"block_label":"w","source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'{"block_hash":"ffffffffffffffff","entropy":0.0,"block_label":"","source_offset_pairs":["0011223344556677",1024]}'
]

    H.lines_equals(returned_answer, expected_answer)

# test import JSON
def test_import_json():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.rm_tempfile("temp_2.json")

    temp1_input = [
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","zero_count":4,"nonprobative_count":5,"name_pairs":["r2","f2"]}',
'{"file_hash":"0011223344556677","filesize":6,"file_type":"fta","zero_count":7,"nonprobative_count":8,"name_pairs":["r1","f1"]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}',
'{"block_hash":"2222222222222222","entropy":11.009,"block_label":"bl1","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":12.509,"block_label":"bl2","source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'{"block_hash":"ffffffffffffffff","entropy":13.259,"block_label":"bl3","source_offset_pairs":["0011223344556677",1024]}']

    expected_answer = ["#","#", \
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","zero_count":4,"nonprobative_count":5,"name_pairs":["r2","f2"]}',
'{"file_hash":"0011223344556677","filesize":6,"file_type":"fta","zero_count":7,"nonprobative_count":8,"name_pairs":["r1","f1"]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}',
'{"block_hash":"2222222222222222","entropy":11.0,"block_label":"bl1","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":12.5,"block_label":"bl2","source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'{"block_hash":"ffffffffffffffff","entropy":13.25,"block_label":"bl3","source_offset_pairs":["0011223344556677",1024]}']

    H.make_tempfile("temp_1.json", temp1_input)
    H.hashdb(["create", "temp_1.hdb"])
    H.hashdb(["import", "temp_1.hdb", "temp_1.json"])
    H.hashdb(["export", "temp_1.hdb", "temp_2.json"])

    returned_answer = H.read_file("temp_2.json")
    H.lines_equals(returned_answer, expected_answer)

if __name__=="__main__":
    test_import_tab1()
    test_import_tab2()
    test_import_json()
    print("Test Done.")

