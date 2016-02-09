#!/usr/bin/env python3
#
# Test performance analysis interfaces.

import helpers as H

db1 = "temp_1.hdb"

def test_random():
    H.rm_tempdir("temp_1.hdb")
    H.hashdb(["create", "temp_1.hdb"])
    lines = H.hashdb(["add_random", "temp_1.hdb", "100"])
    H.lines_equals(lines, [
'Processing index 100 of 100 completed.',
'hashdb changes:',
'    hash_data_data_inserted: 100',
'    hash_data_source_inserted: 100',
'    hash_prefix_inserted: 100',
'    hash_suffix_inserted: 100',
'    source_data_inserted: 1',
'    source_id_inserted: 1',
'    source_name_inserted: 1',
''])
    H.hashdb(["export_json", "temp_1.hdb", "temp_1.json"])

    lines = H.hashdb(["scan_random", "temp_1.hdb", "100"])
    H.lines_equals(lines, [
'# Processing index 100 of 100 completed.',
''])

def test_same():
    H.rm_tempdir("temp_1.hdb")
    H.hashdb(["create", "temp_1.hdb"])
    lines = H.hashdb(["add_same", "temp_1.hdb", "100"])
    H.lines_equals(lines, [
'Processing index 100 of 100 completed.',
'hashdb changes:',
'    hash_data_data_inserted: 1',
'    hash_data_data_same: 99',
'    hash_data_source_inserted: 100',
'    hash_prefix_inserted: 1',
'    hash_suffix_inserted: 1',
'    hash_count_changed: 31',
'    hash_not_changed: 68',
'    source_data_inserted: 1',
'    source_id_inserted: 1',
'    source_name_inserted: 1',
''])
    H.hashdb(["export_json", "temp_1.hdb", "temp_1.json"])

    lines = H.hashdb(["scan_same", "temp_1.hdb", "100"])
    H.lines_equals(lines, [
'# Processing index 100 of 100 completed.',
''])



if __name__=="__main__":
    test_random()
    test_same()
    print("Test Done.")

