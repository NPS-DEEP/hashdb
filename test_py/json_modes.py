#!/usr/bin/env python3
#
# Test the Scan command group

import helpers as H

json_data = ["#","#", \
'{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","zero_count":20,"nonprobative_count":2,"name_pairs":["r1","f1"]}',
'{"block_hash":"8899aabbccddeeff","entropy":8.0,"block_label":"bl2","source_offsets":["0011223344556677",1,[0]]}'
]

# test all modes using the scan_list command
def test_json_modes():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.json", json_data)
    H.hashdb(["import", "temp_1.hdb", "temp_1.json"])

    # make hash file
    hash_file = [
"fp1	8899aabbccddeeff",
"fp2	8899aabbccddeeff"]
    H.make_tempfile("temp_1.txt", hash_file)

    # expanded
    returned_answer = H.hashdb(["scan_list", "-j", "e", "temp_1.hdb", "temp_1.txt"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'fp1	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","entropy":8.0,"block_label":"bl2","count":1,"source_list_id":2343118327,"sources":[{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","zero_count":20,"nonprobative_count":2,"name_pairs":["r1","f1"]}],"source_offsets":["0011223344556677",1,[0]]}',
'fp2	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","entropy":8.0,"block_label":"bl2","count":1,"source_list_id":2343118327,"sources":[{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","zero_count":20,"nonprobative_count":2,"name_pairs":["r1","f1"]}],"source_offsets":["0011223344556677",1,[0]]}',
'# scan_list completed.',
''
])

    # expanded optimized
    returned_answer = H.hashdb(["scan_list", "-j", "o", "temp_1.hdb", "temp_1.txt"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'fp1	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","entropy":8.0,"block_label":"bl2","count":1,"source_list_id":2343118327,"sources":[{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","zero_count":20,"nonprobative_count":2,"name_pairs":["r1","f1"]}],"source_offsets":["0011223344556677",1,[0]]}',
'fp2	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff"}',
'# scan_list completed.',
''
])

    # count only
    returned_answer = H.hashdb(["scan_list", "-j", "c", "temp_1.hdb", "temp_1.txt"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'fp1	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","count":1}',
'fp2	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","count":1}',
'# scan_list completed.',
''
])

    # approximate count
    returned_answer = H.hashdb(["scan_list", "-j", "a", "temp_1.hdb", "temp_1.txt"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'fp1	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","approximate_count":1}',
'fp2	8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","approximate_count":1}',
'# scan_list completed.',
''
])

def test_json_commands():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.json", json_data)
    H.hashdb(["import", "temp_1.hdb", "temp_1.json"])

    # scan_list: alredy done

    # scan_hash
    returned_answer = H.hashdb(["scan_hash", "-j", "c", "temp_1.hdb", "8899aabbccddeeff"])
    H.lines_equals(returned_answer, [
'{"block_hash":"8899aabbccddeeff","count":1}',
''
])

    # scan_media: skip

    # duplicates
    returned_answer = H.hashdb(["duplicates", "-j", "c", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","count":1}',
'# Processing 1 of 1 completed.',
''
])

    # hash_table
    returned_answer = H.hashdb(["hash_table", "-j", "c", "temp_1.hdb", "0011223344556677"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'8899aabbccddeeff	{"block_hash":"8899aabbccddeeff","count":1}',
'# Processing 1 of 1 completed.',
''
])

    # scan_random: nothing returned but accepts -j
    returned_answer = H.hashdb(["scan_random", "-j", "c", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'# Processing 1 of 1 completed.',
''
])

    # scan_same: nothing returned but accepts -j
    returned_answer = H.hashdb(["scan_same", "-j", "c", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'Match not found, hash 80000000000000000000000000000000:',
'# Processing 1 of 1 completed.',
''
])

if __name__=="__main__":
    test_json_modes()
    test_json_commands()
    print("Test Done.")

