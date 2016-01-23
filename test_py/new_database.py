#!/usr/bin/env python3
#
# Test the New Database command group

import os
import helpers as h

db1 = "temp_1.hdb"
settings1 = os.path.join(db1, "settings.json")

# check basic settings
def test_basic_settings():
    # remove existing DB
    h.rm_tempdir(db1)

    # create new DB
    h.hashdb(["create", "-b4", "-s2", "-m100", "-t30:10", db1])

    # validate settings parameters
    lines = h.read_file(settings1)
    h.int_equals(len(lines), 1)
    h.str_equals(lines[0], '{"data_store_version":3, "sector_size":2, "block_size":4, "max_id_offset_pairs":100, "hash_prefix_bits":30, "hash_suffix_bytes":10}')

if __name__=="__main__":
    test_basic_settings()
    print("Test Done.")

