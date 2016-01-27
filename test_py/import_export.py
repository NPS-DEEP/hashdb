#!/usr/bin/env python3
#
# Test the Import Export command group

import helpers as H

def test_import_tab():
    H.rm_tempdir("temp_1.hdb")
    H.rm_tempfile("temp_1.json")
    H.hashdb(["create", "temp_1.hdb"])
    H.make_tempfile("temp_1.tab", [
          "# comment"
          "0011223344556677	8899aabbccddeeff	1",
          "0011223344556677	ffffffffffffffff	2",
          "1111111111111111	2222222222222222	10"])
    H.hashdb(["import_tab", "temp_1.hdb", "temp_1.tab"])
    H.hashdb(["export_json", "temp_1.hdb", "temp_1.json"])

    returned_answer = read_file("temp_1.json")
    expected_answer = ["#","#","text"]
    H.list_equals(returned_answer, expected_answer)


if __name__=="__main__":
    test_import_tab()
    print("Test Done.")

