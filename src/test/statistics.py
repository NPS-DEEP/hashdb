#!/usr/bin/env python3
#
# Test statistics

import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"
db2 = "temp_2.hdb"
xml1 = "temp_1.xml"

def setup():
    # create DB with 3 entries
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.rm_tempfile(xml1)
    H.write_temp_dfxml_hash(byte_run_hashdigest="00", byte_run_file_offset=1*4096, repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00", byte_run_file_offset=2*4096, repository_name="r1", byte_run_hash_label="H")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="11", byte_run_file_offset=3*4096, byte_run_hash_label="L")
    H.hashdb(["import", db1, "temp_dfxml_hash"])

def test_size():
    lines = H.hashdb(["size", db1])
    #print(*lines, sep='\n')
    H.str_equals(lines[0], 'hash store size: 3')
    H.str_equals(lines[1], 'source store size: 2')
    H.str_equals(lines[2], '')
    H.int_equals(len(lines), 3)

def test_sources():
    lines = H.hashdb(["sources", db1])
    H.str_equals(lines[0], '{"source_id":1,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[1], '{"source_id":2,"repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[2], '')
    H.int_equals(len(lines), 3)

def test_histogram():
    lines = H.hashdb(["histogram", db1])
    H.str_equals(lines[4], '{"total_hashes": 3, "total_distinct_hashes": 1}')
    H.str_equals(lines[5], '{"duplicates":1, "distinct_hashes":1, "total":1}')
    H.str_equals(lines[6], '{"duplicates":2, "distinct_hashes":1, "total":2}')
    H.str_equals(lines[7], '')
    H.int_equals(len(lines), 8)

def test_duplicates():
    lines = H.hashdb(["duplicates", db1, "0"])
    H.str_equals(lines[4], 'No hashes were found with this count.')
    H.int_equals(len(lines), 6)
    lines = H.hashdb(["duplicates", db1, "1"])
    H.str_equals(lines[3], '["11",{"count":1}]')
    H.int_equals(len(lines), 6)
    lines = H.hashdb(["duplicates", db1, "2"])
    H.str_equals(lines[3], '["00",{"count":2}]')
    H.int_equals(len(lines), 6)
    lines = H.hashdb(["duplicates", db1, "3"])
    H.str_equals(lines[4], 'No hashes were found with this count.')
    H.int_equals(len(lines), 6)

def test_hash_table():
    # source_id 0
    lines = H.hashdb(["hash_table", db1, "0"])
    H.str_equals(lines[0], 'The requested source ID is not in the database.')
    H.int_equals(len(lines), 2)

    # source_id 1
    lines = H.hashdb(["hash_table", db1, "1"])
    H.str_equals(lines[3], '# {"source_id":1,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[5], '4096	00	{"count":2}')
    H.str_equals(lines[6], '8192	00	{"count":2}')
    H.int_equals(len(lines), 8)

    # source_id 2
    lines = H.hashdb(["hash_table", db1, "2"])
    H.str_equals(lines[3], '# {"source_id":2,"repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[5], '12288	11	{"count":1}')
    H.int_equals(len(lines), 7)

    # source_id 3
    lines = H.hashdb(["hash_table", db1, "3"])
    H.str_equals(lines[0], 'The requested source ID is not in the database.')
    H.int_equals(len(lines), 2)

def write_empty_identified_blocks():
    tempfile = open("temp_identified_blocks", "w")

def write_full_identified_blocks():
    tempfile = open("temp_identified_blocks", "w")
    tempfile.write('4096	00	{"count":2}\n')
    tempfile.write('8192	00	{"count":2}\n')
    tempfile.write('12288	11	{"count":1}\n')

def write_wrong_identified_blocks():
    tempfile = open("temp_identified_blocks", "w")
    tempfile.write('65536	22	{"count":2}\n')

def test_expand_identified_blocks():
    # test empty file
    write_empty_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", db1, "temp_identified_blocks"])
    H.int_equals(len(lines), 4)

    # test all
    write_full_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '4096	00	[{"count":2},{"source_list_id":2844319735, "sources":[{"source_id":1,"file_offset":4096,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"},{"source_id":1,"file_offset":8192,"label":"H"}]}]')
    H.str_equals(lines[4], '8192	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[5], '12288	11	[{"count":1},{"source_list_id":654825492, "sources":[{"source_id":2,"file_offset":12288,"label":"L","repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}]}]')
    H.int_equals(len(lines), 7)

    # test all with -m0
    write_full_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", "-m0", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '4096	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[4], '8192	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[5], '12288	11	[{"count":1},{"source_list_id":654825492}]')
    H.int_equals(len(lines), 7)

    # test all with -m1
    write_full_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", "-m1", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '4096	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[4], '8192	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[5], '12288	11	[{"count":1},{"source_list_id":654825492, "sources":[{"source_id":2,"file_offset":12288,"label":"L","repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}]}]');
    H.int_equals(len(lines), 7)

    # test all with -m2
    write_full_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", "-m2", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '4096	00	[{"count":2},{"source_list_id":2844319735, "sources":[{"source_id":1,"file_offset":4096,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"},{"source_id":1,"file_offset":8192,"label":"H"}]}]')
    H.str_equals(lines[4], '8192	00	[{"count":2},{"source_list_id":2844319735}]')
    H.str_equals(lines[5], '12288	11	[{"count":1},{"source_list_id":654825492, "sources":[{"source_id":2,"file_offset":12288,"label":"L","repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}]}]')
    H.int_equals(len(lines), 7)

    # test invalid hash value
    write_wrong_identified_blocks()
    lines = H.hashdb(["expand_identified_blocks", db1, "temp_identified_blocks"])
    H.str_equals((lines[3])[:5], 'Error')
    H.int_equals(len(lines), 5)

def test_explain_identified_blocks():
    # test empty file
    write_empty_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '# hashes')
    H.str_equals(lines[4], '# There are no hashes to report.')
    H.str_equals(lines[5], '# sources')
    H.str_equals(lines[6], '# There are no sources to report.')
    H.int_equals(len(lines), 8)

    # test all
    write_full_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '# hashes')
    H.str_equals(lines[4], '["00",{"count":2},[{"source_id":1,"file_offset":4096},{"source_id":1,"file_offset":8192,"label":"H"}]]')
    H.str_equals(lines[5], '["11",{"count":1},[{"source_id":2,"file_offset":12288,"label":"L"}]]')
    H.str_equals(lines[6], '# sources')
    H.str_equals(lines[7], '{"source_id":1,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[8], '{"source_id":2,"repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.int_equals(len(lines), 10)

    # test all with -m0
    write_full_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", "-m0", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '# hashes')
    H.str_equals(lines[4], '# There are no hashes to report.')
    H.str_equals(lines[5], '# sources')
    H.str_equals(lines[6], '# There are no sources to report.')
    H.int_equals(len(lines), 8)

    # test all with -m1
    write_full_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", "-m1", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '# hashes')
    H.str_equals(lines[4], '["11",{"count":1},[{"source_id":2,"file_offset":12288,"label":"L"}]]')
    H.str_equals(lines[5], '# sources')
    H.str_equals(lines[6], '{"source_id":2,"repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.int_equals(len(lines), 8)

    # test all with -m2
    write_full_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", "-m2", db1, "temp_identified_blocks"])
    H.str_equals(lines[3], '# hashes')
    H.str_equals(lines[4], '["00",{"count":2},[{"source_id":1,"file_offset":4096},{"source_id":1,"file_offset":8192,"label":"H"}]]')
    H.str_equals(lines[5], '["11",{"count":1},[{"source_id":2,"file_offset":12288,"label":"L"}]]')
    H.str_equals(lines[6], '# sources')
    H.str_equals(lines[7], '{"source_id":1,"repository_name":"r1","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.str_equals(lines[8], '{"source_id":2,"repository_name":"repositoryname","filename":"file1","file_hashdigest":"ff112233445566778899aabbccddeeff"}')
    H.int_equals(len(lines), 10)

    # test invalid hash value
    write_wrong_identified_blocks()
    lines = H.hashdb(["explain_identified_blocks", db1, "temp_identified_blocks"])
    H.str_equals((lines[3])[:5], 'Error')
    H.str_equals(lines[4], '# hashes')
    H.str_equals(lines[5], '# There are no hashes to report.')
    H.str_equals(lines[6], '# sources')
    H.str_equals(lines[7], '# There are no sources to report.')
    H.int_equals(len(lines), 9)


if __name__=="__main__":
    setup()
    test_size()
    test_sources()
    test_histogram()
    test_duplicates()
    test_hash_table()
    test_expand_identified_blocks()
    test_explain_identified_blocks()
    print("Test Done.")

