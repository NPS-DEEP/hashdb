#!/usr/bin/env python3
#
# Test the Scan command group

import helpers as H

def test_size():
    # hash stores
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0011223344556677", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"00112233556677", "source_offset_pairs":["0000000000000000", 512]}'])
    expected_answer = [
'{"hash_data_store":2, "hash_store":1, "source_data_store":1, "source_id_store":1, "source_name_store":0}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, no name_pairs
    H.make_hashdb("temp_1.hdb", [
'{"file_hash":"0011223344556677","filesize":0,"name_pairs":[]}'])
    expected_answer = [
'{"hash_data_store":0, "hash_store":0, "source_data_store":1, "source_id_store":1, "source_name_store":0}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, name_pairs
    H.make_hashdb("temp_1.hdb", [
'{"file_hash":"0011223344556677","filesize":0,"name_pairs":["r1","f1","r2","f2"]}'])
    expected_answer = [
'{"hash_data_store":0, "hash_store":0, "source_data_store":1, "source_id_store":1, "source_name_store":2}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

def test_sources():

    # source stores, no name_pairs
    H.make_hashdb("temp_1.hdb", [
'{"file_hash":"0011223344556677","filesize":0,"name_pairs":[]}'])
    expected_answer = [
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}',
'']
    returned_answer = H.hashdb(["sources", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, two name_pairs
    H.make_hashdb("temp_1.hdb", [
'{"file_hash":"0011223344556677","filesize":0,"name_pairs":["r1","f1","r2","f2"]}'])
    expected_answer = [
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["r1","f1","r2","f2"]}',
'']
    returned_answer = H.hashdb(["sources", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

def test_histogram():
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0000000000000000", "source_offset_pairs":[]}',
'{"block_hash":"1111111111111111", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"2222222222222222", "source_offset_pairs":["0000000000000000", 0,"0000000000000000",512]}'])

    returned_answer = H.hashdb(["histogram", "temp_1.hdb"])
    H.lines_equals(returned_answer, [
'# hashdb-Version',
'# command_line: ../src/hashdb histogram temp_1.hdb',
'{"total_hashes": 3, "total_distinct_hashes": 1}',
'{"duplicates":1, "distinct_hashes":1, "total":1}',
'{"duplicates":2, "distinct_hashes":1, "total":2}',
'# Processing index 2 of 2 completed.',
''])

def test_duplicates():
    # hash 0... doesn't go in at all.
    # hash 1... has one source with one pair.
    # hash 2... has one source with two pairs.
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0000000000000000", "source_offset_pairs":[]}',
'{"block_hash":"1111111111111111", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"2222222222222222", "source_offset_pairs":["0000000000000000", 0,"0000000000000000",512]}'])

    # zero
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "0"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: ',
'# command_line: ../src/hashdb duplicates temp_1.hdb 0',
'No hashes were found with this count.',
'Processing index 4 of 4 completed.',
''])

    # one
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: ',
'# command_line: ../src/hashdb duplicates temp_1.hdb 1',
'1111111111111111	{"block_hash":"1111111111111111","entropy":0,"block_label":"","source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_offset_pairs":["0000000000000000",0]}',
'Processing index 4 of 4 completed.',
''])

    # two
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "2"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: ',
'# command_line: ../src/hashdb duplicates temp_1.hdb 2',
'2222222222222222	{"block_hash":"2222222222222222","entropy":0,"block_label":"","source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_offset_pairs":["0000000000000000",0,"0000000000000000",512]}',
'# Processing index 4 of 4 completed.',
''])

    # three
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "3"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: ',
'# command_line: ../src/hashdb duplicates temp_1.hdb 0',
'No hashes were found with this count.',
'Processing index 4 of 4 completed.',
''])

def test_hash_table():
    # note that the first hash doesn't go in at all, next goes in once, last goes in twice.
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0000000000000000", "source_offset_pairs":[]}',
'{"block_hash":"1111111111111111", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"2222222222222222", "source_offset_pairs":["0000000000000000", 0,"0000000000000000",512]}'])

    # no match
    returned_answer = H.hashdb(["hash_table", "temp_1.hdb", "0011223344556677"])
    H.lines_equals(returned_answer, [
'There is no source with this file hash',
''])

    # two matches
    returned_answer = H.hashdb(["hash_table", "temp_1.hdb", "0000000000000000"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: ',
'# command_line: ../src/hashdb hash_table temp_1.hdb 0000000000000000',
'1111111111111111	{"block_hash":"1111111111111111","entropy":0,"block_label":"","source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_offset_pairs":["0000000000000000",0]}',
'2222222222222222	{"block_hash":"2222222222222222","entropy":0,"block_label":"","source_list_id":1696784233,"sources":[],"source_offset_pairs":["0000000000000000",0,"0000000000000000",512]}',
'# Processing index 4 of 4 completed.',
''])

if __name__=="__main__":
    test_size()
    test_sources()
    test_histogram()
    test_duplicates()
    test_hash_table()

    print("Test Done.")

