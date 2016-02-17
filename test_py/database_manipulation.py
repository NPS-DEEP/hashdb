#!/usr/bin/env python3
#
# Test database maniplation commands

import shutil
import helpers as H

json_db1 = [
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"temp_1.tab","filename":"temp_1.tab"}]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"temp_1.tab","filename":"temp_1.tab"}]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"temp_1.tab","filename":"temp_1.tab"},{"repository_name":"second_temp_1.tab", "filename":"second_temp_1.tab"}]}',
'{"block_hash":"2222222222222222","entropy":0,"block_label":"","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":0,"block_label":"","source_offset_pairs":["0011223344556677",0,"0011223344556677",512,"0000000000000000",0]}',
'{"block_hash":"ffffffffffffffff","entropy":0,"block_label":"","source_offset_pairs":["0011223344556677",1024]}']

json1 = [
'#', '#',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"second_temp_1.tab","filename":"second_temp_1.tab"},{"repository_name":"temp_1.tab","filename":"temp_1.tab"}]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"temp_1.tab","filename":"temp_1.tab"}]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","nonprobative_count":0,"names":[{"repository_name":"temp_1.tab","filename":"temp_1.tab"}]}',
'{"block_hash":"2222222222222222","entropy":0,"block_label":"","source_offset_pairs":["1111111111111111",4096]}',
'{"block_hash":"8899aabbccddeeff","entropy":0,"block_label":"","source_offset_pairs":["0000000000000000",0,"0011223344556677",0,"0011223344556677",512]}',
'{"block_hash":"ffffffffffffffff","entropy":0,"block_label":"","source_offset_pairs":["0011223344556677",1024]}']

def test_add():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json1)
    H.rm_tempdir("temp_2.hdb")

    # add to new temp_2.hdb
    H.hashdb(["add", "temp_1.hdb", "temp_2.hdb"])

    # temp_2.hdb should match
    H.hashdb(["export_json", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, json1)

    # add to existing temp_2.hdb
    H.hashdb(["add", "temp_1.hdb", "temp_2.hdb"])

    # temp_2.hdb should match
    H.hashdb(["export_json", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, json1)

def test_add_multiple():
    json_db1 = [
'{"file_hash":"11","filesize":1,"file_type":"ft1","nonprobative_count":111,"names":[{"repository_name":"rn1","filename":"fn1"}]}',
'{"block_hash":"11111111","entropy":101,"block_label":"bl1","source_offset_pairs":["11",1024]}']
    json_db2 = [
'{"file_hash":"22","filesize":2,"file_type":"ft2","nonprobative_count":222,"names":[{"repository_name":"rn2","filename":"fn2"}]}',
'{"block_hash":"22222222","entropy":202,"block_label":"bl2","source_offset_pairs":["22",1024]}']
    json3_back = [
'#','#',
'{"file_hash":"11","filesize":1,"file_type":"ft1","nonprobative_count":111,"names":[{"repository_name":"rn1","filename":"fn1"}]}',
'{"file_hash":"22","filesize":2,"file_type":"ft2","nonprobative_count":222,"names":[{"repository_name":"rn2","filename":"fn2"}]}',
'{"block_hash":"11111111","entropy":101,"block_label":"bl1","source_offset_pairs":["11",1024]}',
'{"block_hash":"22222222","entropy":202,"block_label":"bl2","source_offset_pairs":["22",1024]}']


    # create DBs
    H.make_hashdb("temp_1.hdb", json_db1)
    H.make_hashdb("temp_2.hdb", json_db2)
    H.rm_tempdir("temp_3.hdb")

    # add 1 and 2 into 3
    H.hashdb(["add_multiple", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])

    # check temp_3.hdb
    H.hashdb(["export_json", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, json3_back)

def test_add_repository():
    # hash with correct repository name is added
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["add_repository", db1, db2, "r1"])
    sizes = H.parse_sizes(H.hashdb(["size", db2]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    H.hashdb(["export", db2, xml1])
    H.dfxml_hash_equals(repository_name="r1")

def test_intersect():
    # db1 with a,b and db2 with b,c intersect to db3 with just b
    # using same hash and different repository name
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    shutil.rmtree(db3, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.hashdb(["create", db2])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r3")
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.hashdb(["intersect", db1, db2, db3])
    sizes = H.parse_sizes(H.hashdb(["size", db3]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    H.hashdb(["export", db3, xml1])
    H.dfxml_hash_equals(repository_name="r2")

def test_intersect_hash():
    # db1 with a,b and db2 with b,c intersect to db3 with just b
    # using different hash
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    shutil.rmtree(db3, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.hashdb(["create", db2])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00112233445566778899aabbccddeef1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00112233445566778899aabbccddeef2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00112233445566778899aabbccddeef3")
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.hashdb(["intersect_hash", db1, db2, db3])
    sizes = H.parse_sizes(H.hashdb(["size", db3]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    H.hashdb(["export", db3, xml1])
    H.dfxml_hash_equals(byte_run_hashdigest="00112233445566778899aabbccddeef2")

def test_subtract():
    # db1 - db2 -> db3 where source must match
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    shutil.rmtree(db3, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.hashdb(["create", db2])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["subtract", db1, db2, db3])
    sizes = H.parse_sizes(H.hashdb(["size", db3]))
    H.int_equals(sizes['hash_store_size'],1)
    H.hashdb(["export", db3, xml1])
    H.dfxml_hash_equals(repository_name="r2")

def test_subtract_hash():
    # db1 - db2 -> db3 where hash must match
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    shutil.rmtree(db3, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.hashdb(["create", db2])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.hashdb(["subtract_hash", db1, db2, db3])
    sizes = H.parse_sizes(H.hashdb(["size", db3]))
    H.int_equals(sizes['hash_store_size'],1)
    H.hashdb(["export", db1, xml1])
    H.dfxml_hash_equals(byte_run_hashdigest="00")

def test_subtract_repository():
    # hash with correct repository name is added
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["subtract_repository", db1, db2, "r1"])
    sizes = H.parse_sizes(H.hashdb(["size", db2]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    H.hashdb(["export", db2, xml1])
    H.dfxml_hash_equals(repository_name="r2")

def test_deduplicate():
    # hash with correct repository name is added
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    H.rm_tempfile(xml1)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.write_temp_dfxml_hash(byte_run_hashdigest="00")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["deduplicate", db1, db2])
    sizes = H.parse_sizes(H.hashdb(["size", db2]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    H.hashdb(["export", db2, xml1])
    H.dfxml_hash_equals(byte_run_hashdigest="00")

if __name__=="__main__":
    test_add()
    test_add_multiple()
    test_add_repository()
    test_intersect()
    test_intersect_hash()
    test_subtract()
    test_subtract_hash()
    test_subtract_repository()
    test_deduplicate()
    print("Test Done.")

