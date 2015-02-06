// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Test the libhashdb interfaces.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "hashdb.hpp"
#include "directory_helper.hpp"
#include "lmdb_helper.h" // for hex_to_binary_hash

static const std::string hashdb_dir = "temp_dir_libhashdb_test.hdb";
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

void do_test() {
  // clean up from any previous run
  rm_hashdb_dir(hashdb_dir);

  // open reader and writer
  hashdb_t hashdb1;
  hashdb_t hashdb2;

  // input for import
  hashdb_t::import_input_t import_input;
  import_input.push_back(hashdb_t::import_element_t(binary_aa, "rep1", "file1", 0));
  import_input.push_back(hashdb_t::import_element_t(binary_aa, "rep1", "file1", 4096));
  import_input.push_back(hashdb_t::import_element_t(binary_aa, "rep1", "file1", 4097)); // invalid

  // open hashdb1 for import
  std::pair<bool, std::string> import_pair = hashdb1.open_import(hashdb_dir, 4096, 20);
  TEST_EQ(import_pair.first, true);

  // import some elements
  int status;
  status = hashdb1.import(import_input);
  TEST_EQ(status, 0);

  // import metadata
  status = hashdb1.import_metadata("rep1", "file1", 10000, binary_big);
  TEST_EQ(status, 0);
  status = hashdb1.import_metadata("zrep1", "file1", 10000, binary_big);
  TEST_EQ(status, 0);
  status = hashdb1.import_metadata("zrep1", "file1", 10000, binary_big);
  TEST_EQ(status, 0);

  // open hashdb2 for scan
  std::pair<bool, std::string> scan_pair = hashdb2.open_scan(hashdb_dir);
  TEST_EQ(scan_pair.first, true);

  // populate input
  hashdb_t::scan_input_t scan_input;
  scan_input.push_back(binary_aa);
  scan_input.push_back(binary_bb);
  scan_input.push_back(binary_big);

  // perform scan
  hashdb_t::scan_output_t scan_output;
  status = hashdb2.scan(scan_input, scan_output);
  TEST_EQ(status, 0);
  TEST_EQ(scan_output.size(), 1);
  TEST_EQ(scan_output[0].second, 2);
}

int main(int argc, char* argv[]) {
  do_test();

  return 0;
}

