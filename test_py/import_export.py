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
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1",
          "0000000000000000	8899aabbccddeeff	1",
          "0011223344556677	8899aabbccddeeff	2",
          "0011223344556677	ffffffffffffffff	3",
          "1111111111111111	2222222222222222	9",
          "1111111111111111	2222222222222222	9"])
    H.hashdb(["import_tab", "temp_1.hdb", "temp_1.tab"])
    H.hashdb(["export", "temp_1.hdb", "temp_1.json"])

    returned_answer = H.read_file("temp_1.json")
    expected_answer = ["# command: ","# hashdb-Version: ",
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_offsets":["1111111111111111",2,[4096]]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_offsets":["0000000000000000",1,[0],"0011223344556677",2,[0,512]]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_offsets":["0011223344556677",1,[1024]]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_1.tab","temp_1.tab"]}'
]

    H.lines_equals(returned_answer, expected_answer)

# test repository name option
def test_import_tab2():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_1.tab"])
    H.hashdb(["export", "temp_1.hdb", "temp_1.json"])

    returned_answer = H.read_file("temp_1.json")
    expected_answer = ["# command: ","# hashdb-Version: ", \
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_offsets":["0011223344556677",1,[0]]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["r","temp_1.tab"]}'
]
    H.lines_equals(returned_answer, expected_answer)

# test whitelist directory option
def test_import_tab3():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_1.tab"])

    H.rm_tempdir("temp_2.hdb")
    H.rm_tempfile("temp_2.json")
    H.hashdb(["create", "temp_2.hdb"])
    H.make_tempfile("temp_2.tab", [
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0011223344556677	8899aabbccddeeff	1",
          "0000000000000000	8899aabbccddeeff	1",
          "0011223344556677	8899aabbccddeeff	2",
          "0011223344556677	ffffffffffffffff	3",
          "1111111111111111	2222222222222222	9",
          "1111111111111111	2222222222222222	9"])
    H.hashdb(["import_tab", "-w", "temp_1.hdb", "temp_2.hdb", "temp_2.tab"])
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])

    returned_answer = H.read_file("temp_2.json")
    expected_answer = ["# command: ","# hashdb-Version: ", \
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_offsets":["1111111111111111",2,[4096]]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"w","source_offsets":["0000000000000000",1,[0],"0011223344556677",2,[0,512]]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_offsets":["0011223344556677",1,[1024]]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["temp_2.tab","temp_2.tab"]}'
]

    H.lines_equals(returned_answer, expected_answer)

# test preexisting source file detection
def test_import_tab4():
    H.rm_tempdir("temp_1.hdb")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0000000000000000	8888888888888888	1",
          "0000000000000000	8888888888888888	2"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_1.tab"])
    H.make_tempfile("temp_2.tab", [
          "# <file hash> <tab> <block hash> <tab> <index>",
          "0000000000000000	8888888888888888	1",
          "0000000000000000	8888888888888888	2",
          "0000000000000000	8888888888888888	3",
          "1111111111111111	8888888888888888	1",
          "1111111111111111	8888888888888888	2"])
    H.hashdb(["import_tab", "-rr", "temp_1.hdb", "temp_2.tab"])

    H.hashdb(["export", "temp_1.hdb", "temp_1.json"])

    returned_answer = H.read_file("temp_1.json")
    expected_answer = [
"# command: ","# hashdb-Version: ",
'{"block_hash":"8888888888888888","k_entropy":0,"block_label":"","source_offsets":["0000000000000000",2,[0,512],"1111111111111111",2,[0,512]]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["r","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["r","temp_2.tab"]}'
]
    H.lines_equals(returned_answer, expected_answer)

# test import JSON
def test_import_json():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.rm_tempfile("temp_2.json")

    temp1_input = [
'{"block_hash":"2222222222222222","k_entropy":1,"block_label":"bl1","source_offsets":["1111111111111111",2,[4096]]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":2,"block_label":"bl2","source_offsets":["0000000000000000",1,[0],"0011223344556677",2,[0,512]]}',
'{"block_hash":"ffffffffffffffff","k_entropy":3,"block_label":"bl3","source_offsets":["0011223344556677",1,[1024]]}',
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","zero_count":4,"nonprobative_count":5,"name_pairs":["r2","f2"]}',
'{"file_hash":"0011223344556677","filesize":6,"file_type":"fta","zero_count":7,"nonprobative_count":8,"name_pairs":["r1","f1"]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}'
]

    expected_answer = [
"# command: ","# hashdb-Version: ",
'{"block_hash":"2222222222222222","k_entropy":1,"block_label":"bl1","source_offsets":["1111111111111111",2,[4096]]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":2,"block_label":"bl2","source_offsets":["0000000000000000",1,[0],"0011223344556677",2,[0,512]]}',
'{"block_hash":"ffffffffffffffff","k_entropy":3,"block_label":"bl3","source_offsets":["0011223344556677",1,[1024]]}',
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","zero_count":4,"nonprobative_count":5,"name_pairs":["r2","f2"]}',
'{"file_hash":"0011223344556677","filesize":6,"file_type":"fta","zero_count":7,"nonprobative_count":8,"name_pairs":["r1","f1"]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}'
]

    H.make_tempfile("temp_1.json", temp1_input)
    H.hashdb(["create", "temp_1.hdb"])
    H.hashdb(["import", "temp_1.hdb", "temp_1.json"])
    H.hashdb(["export", "temp_1.hdb", "temp_2.json"])

    returned_answer = H.read_file("temp_2.json")
    H.lines_equals(returned_answer, expected_answer)

# test import JSON hash partition range
def test_export_json_hash_partition_range():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.rm_tempfile("temp_2.json")

    temp1_input = [
'{"block_hash":"2222222222222222","k_entropy":1,"block_label":"bl1","source_offsets":["1111111111111111",2,[4096]]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":2,"block_label":"bl2","source_offsets":["0000000000000000",1,[0],"0011223344556677",2,[0,512]]}',
'{"block_hash":"ffffffffffffffff","k_entropy":3,"block_label":"bl3","source_offsets":["0011223344556677",1,[1024]]}',
'{"file_hash":"0000000000000000","filesize":3,"file_type":"ftb","zero_count":4,"nonprobative_count":5,"name_pairs":["r2","f2"]}',
'{"file_hash":"0011223344556677","filesize":6,"file_type":"fta","zero_count":7,"nonprobative_count":8,"name_pairs":["r1","f1"]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}'
]

    expected_answer = [
"# command: ","# hashdb-Version: ",
'{"block_hash":"2222222222222222","k_entropy":1,"block_label":"bl1","source_offsets":["1111111111111111",2,[4096]]}',
'{"file_hash":"1111111111111111","filesize":9,"file_type":"ftc","zero_count":10,"nonprobative_count":11,"name_pairs":["r3","f3"]}'
]

    H.make_tempfile("temp_1.json", temp1_input)
    H.hashdb(["create", "temp_1.hdb"])
    H.hashdb(["import", "temp_1.hdb", "temp_1.json"])
    H.hashdb(["export", "-p", "00:80", "temp_1.hdb", "temp_2.json"])

    returned_answer = H.read_file("temp_2.json")
    H.lines_equals(returned_answer, expected_answer)

def test_ingest():
    H.make_temp_media("temp_1_media")
    H.rm_tempdir("temp_1.hdb")
    H.hashdb(["create", "temp_1.hdb"])
    H.hashdb(["ingest", "temp_1.hdb", "temp_1_media"])
    returned_answer = H.hashdb(["size", "temp_1.hdb"])
    H.lines_equals(returned_answer, [
'{"hash_data_store":4, "hash_store":4, "source_data_store":4, "source_id_store":4, "source_name_store":4}',
''
])
# NOTE: cannot use this because timestamp is always different.
#    returned_answer = H.hashdb(["export", "temp_1.hdb", "-"])
#    H.lines_equals(returned_answer, [
#'# command: ', '# hashdb-Version: ',
#'{"block_hash":"06d2d08afcbd948f6f033d3b249dde4f","k_entropy":4038,"block_label":"H","source_offsets":["63750f9c54bf17c589466d924257a929",1,[512]]}',
#'{"block_hash":"11e6c76f7ae1c4642bd51a4275ead6a2","k_entropy":404,"block_label":"HM","source_offsets":["de3fe827ce5077cc479b93c1146f07d7",1,[0]]}',
#'{"block_hash":"18d8dfaa993bf04fc3257fddc10b7548","k_entropy":220,"block_label":"HM","source_offsets":["40dc7e419c3722c2b0e87e227c51cc4a",1,[0]]}',
#'{"block_hash":"480ca749a0bfacec96579151c8f85e95","k_entropy":404,"block_label":"HM","source_offsets":["3987c7a5bdae192e485f4d813d732891",1,[0]]}',
#'{"file_hash":"3987c7a5bdae192e485f4d813d732891","filesize":21,"file_type":"","zero_count":0,"nonprobative_count":1,"name_pairs":["temp_1_media","temp_1_media-666-zip"]}',
#'{"file_hash":"40dc7e419c3722c2b0e87e227c51cc4a","filesize":12,"file_type":"","zero_count":0,"nonprobative_count":1,"name_pairs":["temp_1_media","temp_1_media-872-gzip"]}',
#'{"file_hash":"63750f9c54bf17c589466d924257a929","filesize":917,"file_type":"","zero_count":1,"nonprobative_count":1,"name_pairs":["temp_1_media","temp_1_media"]}',
#'{"file_hash":"de3fe827ce5077cc479b93c1146f07d7","filesize":21,"file_type":"","zero_count":0,"nonprobative_count":1,"name_pairs":["temp_1_media","temp_1_media-600-zip"]}',
#'# Processing 4 of 4 completed.',
#''
#])

if __name__=="__main__":
    test_import_tab1()
    test_import_tab2()
    test_import_tab3()
    test_import_tab4()
    test_import_json()
    test_export_json_hash_partition_range()
    test_ingest()
    print("Test Done.")

