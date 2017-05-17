#!/usr/bin/env python3
#
# Test the Scan command group

import helpers as H

def test_size():
    # hash stores
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0011223344556677", "source_sub_counts":["0000000000000000", 1]}',
'{"block_hash":"00112233556677", "source_sub_counts":["0000000000000000", 1]}'])
    expected_answer = [
'{"hash_data_store":2, "hash_store":2, "source_data_store":1, "source_id_store":1, "source_name_store":0}',
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
'{"block_hash":"0000000000000000", "source_sub_counts":[]}',
'{"block_hash":"1111111111111111", "source_sub_counts":["0000000000000000", 1]}',
'{"block_hash":"2222222222222222", "source_sub_counts":["0000000000000000", 2]}'])

    returned_answer = H.hashdb(["histogram", "temp_1.hdb"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'{"total_hashes": 3, "total_distinct_hashes": 1}',
'{"duplicates":1, "distinct_hashes":1, "total":1}',
'{"duplicates":2, "distinct_hashes":1, "total":2}',
'# Processing 2 of 2 completed.',
''])

def test_duplicates():
    # hash 0... doesn't go in at all.
    # hash 1... has one source with one pair.
    # hash 2... has one source with two pairs.
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0000000000000000", "source_sub_counts":[]}',
'{"block_hash":"1111111111111111", "source_sub_counts":["0000000000000000", 1]}',
'{"block_hash":"2222222222222222", "source_sub_counts":["0000000000000000", 2]}'])

    # zero
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "0"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'No hashes were found with this count.',
'# Processing 2 of 2 completed.',
''])

    # one
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "1"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'1111111111111111	{"block_hash":"1111111111111111","k_entropy":0,"block_label":"","count":1,"source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_sub_counts":["0000000000000000",1]}',
'# Processing 2 of 2 completed.',
''])

    # two
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "2"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'2222222222222222	{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","count":2,"source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_sub_counts":["0000000000000000",2]}',
'# Processing 2 of 2 completed.',
''])

    # three
    returned_answer = H.hashdb(["duplicates", "temp_1.hdb", "3"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'No hashes were found with this count.',
'# Processing 2 of 2 completed.',
''])

def test_hash_table():
    # note that the first hash doesn't go in at all, next goes in once, last goes in twice.
    H.make_hashdb("temp_1.hdb", [
'{"block_hash":"0000000000000000", "source_sub_counts":[]}',
'{"block_hash":"1111111111111111", "source_sub_counts":["0000000000000000", 1]}',
'{"block_hash":"2222222222222222", "source_sub_counts":["0000000000000000", 2]}'])

    # no match
    returned_answer = H.hashdb(["hash_table", "temp_1.hdb", "0011223344556677"])
    H.lines_equals(returned_answer, [
'There is no source with this file hash',
''])

    # two matches
    returned_answer = H.hashdb(["hash_table", "temp_1.hdb", "0000000000000000"])
    H.lines_equals(returned_answer, [
'# command: ',
'# hashdb-Version: ',
'1111111111111111	{"block_hash":"1111111111111111","k_entropy":0,"block_label":"","count":1,"source_list_id":1696784233,"sources":[{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_sub_counts":["0000000000000000",1]}',
'2222222222222222	{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","count":2,"source_list_id":1696784233,"sources":[],"source_sub_counts":["0000000000000000",2]}',
'# Processing 2 of 2 completed.',
''])

def test_media():
    # create media to read
    H.make_temp_media("temp_1_media")

    # read embedded filename in zip header
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "630", "13"])
    H.lines_equals(returned_answer, ["temp_0_file_1"])

    # read zip 1
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "600-zip-0", "50"])
    H.lines_equals(returned_answer, ["temp_0_file_1 content"])

    # read zip 2
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "666-zip-0", "50"])
    H.lines_equals(returned_answer, ["temp_0_file_2 content"])

    # read gzip
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "872-gzip-0", "50"])
    H.lines_equals(returned_answer, ["gzip content"])

    # read partial zip 1
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "600-zip-3", "10"])
    H.lines_equals(returned_answer, ["p_0_file_1"])

    # read partial gzip
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "872-gzip-3", "5"])
    H.lines_equals(returned_answer, ["p con"])

    # read out of range
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "1000000000", "50"])
    H.lines_equals(returned_answer, [""])

    # read out of range zip 1
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "600-zip-100", "50"])
    H.lines_equals(returned_answer, [""])

    # read out of range gzip
    returned_answer = H.hashdb(["read_media", "temp_1_media",
                                "872-gzip-100", "50"])
    H.lines_equals(returned_answer, [""])

    # read media size
    returned_answer = H.hashdb(["read_media_size", "temp_1_media"])
    H.lines_equals(returned_answer, ["917", ""])

if __name__=="__main__":
    test_size()
    test_sources()
    test_histogram()
    test_duplicates()
    test_hash_table()
    test_media()

    print("Test Done.")

