#!/usr/bin/env python3
#
# Test tuning

import shutil
import hashdb_helpers as H
import os

db1 = "temp_1.hdb"

# see bloom_filter_manager_test.cpp for actual interface tests.
# This test just checks for whether the bloom filter is enabled.
def test_rebuild_bloom():
    bloom_path = os.path.join(db1, "bloom_filter")

    # create DB
    shutil.rmtree(db1, True)
    H.hashdb(["create", db1])
    H.bool_equals(os.path.exists(bloom_path), True)
    H.hashdb(["rebuild_bloom", db1, "--bloom", "disabled"])
    H.bool_equals(os.path.exists(bloom_path), False)
    H.hashdb(["rebuild_bloom", db1])
    H.bool_equals(os.path.exists(bloom_path), True)


if __name__=="__main__":
    test_rebuild_bloom()
    print("Test Done.")

