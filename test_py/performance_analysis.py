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
'# Processing index 100 of 100 completed.',
'hash_data_inserted: 100',
'hash_data_metadata_different: 0',
'hash_data_not_inserted_duplicate_source: 0',
'hash_data_not_inserted_invalid_file_offset: 0',
'hash_data_not_inserted_max_id_offset_pairs: 0',
'hash_inserted: 100',
'hash_not_inserted: 0',
'source_data_inserted: 1',
'source_data_different: 0',
'source_data_not_inserted: 0',
'source_id_inserted: 1',
'source_id_not_inserted: 0',
'source_name_inserted: 1',
'source_name_not_inserted: 0',
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
'# Processing index 100 of 100 completed.',
'hash_data_inserted: 100',
'hash_data_metadata_different: 0',
'hash_data_not_inserted_duplicate_source: 0',
'hash_data_not_inserted_invalid_file_offset: 0',
'hash_data_not_inserted_max_id_offset_pairs: 0',
'hash_inserted: 1',
'hash_not_inserted: 99',
'source_data_inserted: 1',
'source_data_different: 0',
'source_data_not_inserted: 0',
'source_id_inserted: 1',
'source_id_not_inserted: 0',
'source_name_inserted: 1',
'source_name_not_inserted: 0',
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

