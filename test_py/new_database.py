#!/usr/bin/env python3
#
# Test the New Database command group

import os
import helpers as h

db1 = "temp_1.hdb"
settings1 = os.path.join("temp_1.hdb", "settings.json")

# check basic settings
def test_basic_settings():
    # remove existing DB
    h.rm_tempdir("temp_1.hdb")

    # create new DB
    h.hashdb(["create", "-b4", "temp_1.hdb"])

    # validate settings parameters
    lines = h.read_file(settings1)
    h.lines_equals(lines, [
'{"settings_version":4, "block_size":4}'

])

if __name__=="__main__":
    test_basic_settings()
    print("Test Done.")

