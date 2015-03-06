#!/usr/bin/env python3
#
# Test performance analysis interface

import shutil
import hashdb_helpers as H

db1 = "temp_1.hdb"

def test_add_random():

    # check for correct size and default repository name
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.hashdb(["add_random", db1, "1"])
    sizes = H.parse_sizes(H.hashdb(["size", db1]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    lines = H.hashdb(["sources", db1])
    #print(*lines, sep='\n')
    H.str_equals(lines[0], '{"source_id":1,"repository_name":"repository_temp_1.hdb","filename":"add_random"}');
    H.int_equals(len(lines), 2)

    # check for alternate repository name
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.hashdb(["add_random", "-rr1", db1, "1"])
    sizes = H.parse_sizes(H.hashdb(["size", db1]))
    H.int_equals(sizes['hash_store_size'],1)
    H.int_equals(sizes['source_store_size'],1)
    lines = H.hashdb(["sources", db1])
    H.str_equals(lines[0], '{"source_id":1,"repository_name":"r1","filename":"add_random"}');
    H.int_equals(len(lines), 2)

def test_scan_random():
    # nothing found
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    lines = H.hashdb(["scan_random", db1, "1000000"])
    H.str_equals(lines[0], 'Scanning random hash 1000000 of 1000000')
    H.int_equals(len(lines), 2)

    # something found
    shutil.rmtree(db1, True)
    H.hashdb(["create", "-t1", db1])
    H.hashdb(["add_random", db1, "256"])
    lines = H.hashdb(["scan_random", db1, "1"])
    print(*lines, sep='\n')
    H.str_equals((lines[0])[:11], 'Match found')

if __name__=="__main__":
    test_add_random()
    test_scan_random()
    print("Test Done.")

