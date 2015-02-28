#!/usr/bin/env python3
#
# Test database maniplation commands

import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"
db2 = "temp_2.hdb"
db3 = "temp_3.hdb"
xml1 = "temp_1.xml"

def test_add():
    # one hash in db1 is added to db2
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash()
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["add", db1, db2])
    sizes = H.parse_sizes(H.hashdb(["size", db2]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)

def test_add_multiple():
    # hash from db1 and db2 result in two hashes in db3
    shutil.rmtree(db1, True)
    shutil.rmtree(db2, True)
    shutil.rmtree(db3, True)
    H.hashdb(["create", db1])
    H.write_temp_dfxml_hash(repository_name="r1")
    H.hashdb(["import", db1, "temp_dfxml_hash"])
    H.hashdb(["create", db2])
    H.write_temp_dfxml_hash(repository_name="r2")
    H.hashdb(["import", db2, "temp_dfxml_hash"])
    H.hashdb(["add_multiple", db1, db2, db3])
    sizes = H.parse_sizes(H.hashdb(["size", db3]))
    H.int_equals(sizes['hash_store_size'],2)
    H.int_equals(sizes['source_store_size'],2)

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
    H.hashdb(["export", db1, xml1])
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

def test_intersect():
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

if __name__=="__main__":
    test_add()
    test_add_multiple()
    test_add_repository()
    test_intersect()
    print("Test Done.")

