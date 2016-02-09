#!/usr/bin/env python3
#
# Test the Scan command group

import helpers as H

json_data = ["#","#", \
'{"file_hash":"0011223344556677","filesize":1,"file_type":"fta","nonprobative_count":2,"names":[{"repository_name":"r1","filename":"f1"}]}',
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","nonprobative_count":4,"names":[{"repository_name":"r2","filename":"f2"}]}',
'{"file_hash":"1111111111111111","filesize":5,"file_type":"ftc","nonprobative_count":6,"names":[{"repository_name":"r3","filename":"f3"}]}',
'{"block_hash":"2222222222222222","low_entropy_label":"le1","entropy":7,"block_label":"bl1","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","low_entropy_label":"le2","entropy":8,"block_label":"bl2","source_offset_pairs":["0011223344556677",0,"0011223344556677",512,"0000000000000000",0]}',
'{"block_hash":"ffffffffffffffff","low_entropy_label":"le3","entropy":9,"block_label":"bl3","source_offset_pairs":["0011223344556677",1024]}']

def _setup(json_data):
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.json", json_data)
    H.hashdb(["import_json", "temp_1.hdb", "temp_1.json"])

def test_size():
    # hash stores
    _setup([
'{"block_hash":"0011223344556677", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"00112233556677", "source_offset_pairs":["0000000000000000", 512]}'])
    expected_answer = [
'{"hash_data_store":2, "hash_store":1, "source_data_store":0, "source_id_store":1, "source_name_store":0}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, no names
    _setup([
'{"file_hash":"0011223344556677","filesize":0,"names":[]}'])
    expected_answer = [
'{"hash_data_store":0, "hash_store":0, "source_data_store":1, "source_id_store":1, "source_name_store":0}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, names
    _setup([
'{"file_hash":"0011223344556677","filesize":0,"names":[{"repository_name":"r1","filename":"f1","repository_name":"r2","filename":"f2"}]}'])
    expected_answer = [
'{"hash_data_store":0, "hash_store":0, "source_data_store":1, "source_id_store":1, "source_name_store":1}',
'']
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

def test_sources():

    # source stores, no names
    _setup([
'{"file_hash":"0011223344556677","filesize":0,"names":[]}'])
    expected_answer = [
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","nonprobative_count":0,"names":[]}',
'']
    returned_answer = H.hashdb(["sources", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

    # source stores, two names
    _setup([
'{"file_hash":"0011223344556677","filesize":0,"names":[{"repository_name":"r1","filename":"f1"},{"repository_name":"r2","filename":"f2"}]}'])
    expected_answer = [
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"r1","filename":"f1"},{"repository_name":"r2","filename":"f2"}]}',
'']
    returned_answer = H.hashdb(["sources", "temp_1.hdb"])
    H.lines_equals(expected_answer, returned_answer)

def test_histogram():
    _setup([
'{"block_hash":"0000000000000000", "source_offset_pairs":[]}',
'{"block_hash":"1111111111111111", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"2222222222222222", "source_offset_pairs":["0000000000000000", 0,"0000000000000000",512]}'])

    returned_answer = H.hashdb(["histogram", "temp_1.hdb"])
    H.lines_equals(returned_answer, [
'# hashdb-Version',
'# histogram-command-Version',
'# command_line: ../src/hashdb histogram temp_1.hdb',
'{"total_hashes": 3, "total_distinct_hashes": 1}',
'{"duplicates":1, "distinct_hashes":1, "total":1}',
'{"duplicates":2, "distinct_hashes":1, "total":2}',
'# Processing index 2 of 2 completed.',
''])

def test_duplicates():
    # note that the first one doesn't go in at all, next goes in once, last goes in twice.
    _setup([
'{"block_hash":"0000000000000000", "source_offset_pairs":[]}',
'{"block_hash":"1111111111111111", "source_offset_pairs":["0000000000000000", 0]}',
'{"block_hash":"2222222222222222", "source_offset_pairs":["0000000000000000", 0,"0000000000000000",512]}'])

    # zero
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "0"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: 3.0.0-dev-0',
'# duplicates-command-Version: 2',
'# command_line: ../src/hashdb duplicates temp_1.hdb 0',
'No hashes were found with this count.',
'Processing index 2 of 2 completed.',
''])

    # one
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: 3.0.0-dev-0',
'# duplicates-command-Version: 2',
'# command_line: ../src/hashdb duplicates temp_1.hdb 1',
'1111111111111111	[{"source_list_id":2844319735},{"sources":[{"source_id":1,"file_hash":"","filesize":0,"file_type":"","nonprobative_count":0,"names":[]}]},{"id_offset_pairs":[1,0]}]',
'Processing index 2 of 2 completed.',
''])

    # two
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "2"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: 3.0.0-dev-0',
'# duplicates-command-Version: 2',
'# command_line: ../src/hashdb duplicates temp_1.hdb 2',
'2222222222222222	[{"source_list_id":2390350426},{"sources":[{"source_id":1,"file_hash":"","filesize":0,"file_type":"","nonprobative_count":0,"names":[]}]},{"id_offset_pairs":[1,0,1,512]}]',
'# Processing index 2 of 2 completed.',
''])

    # three
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "3"])
    H.lines_equals(returned_answer, [
'# hashdb-Version: 3.0.0-dev-0',
'# duplicates-command-Version: 2',
'# command_line: ../src/hashdb duplicates temp_1.hdb 0',
'No hashes were found with this count.',
'Processing index 2 of 2 completed.',
''])

def test_hash_table():
    # note that the first one doesn't go in at all, next goes in once, last goes in twice.
    _setup([
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
'# hashdb-Version: 3.0.0-dev-0',
'# hash-table-command-Version: 3',
'# command_line: ../src/hashdb hash_table temp_1.hdb 0000000000000000',
'1111111111111111	[{"source_list_id":2844319735},{"sources":[{"source_id":1,"file_hash":"","filesize":0,"file_type":"","nonprobative_count":0,"names":[]}]},{"id_offset_pairs":[1,0]}]',
'2222222222222222	[{"source_list_id":2390350426},{"sources":[]},{"id_offset_pairs":[1,0,1,512]}]',
'2222222222222222	',
'# Processing index 2 of 2 completed.',
''])

if __name__=="__main__":
    test_size()
    test_sources()
    test_histogram()
    test_duplicates()
    test_hash_table()

    print("Test Done.")

