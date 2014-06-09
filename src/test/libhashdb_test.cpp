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
 * Test the map_multimap_manager.
 */

#include "directory_helper.hpp"
#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "hashdb.hpp"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "../hash_t_selector.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";

typedef hashdb_t__<hash_t> hashdb_t;

void do_import() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // valid hashdigest values
  hash_t k1;
  to_key(0, k1);

  // input for import
  hashdb_t::import_input_t import_input;
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 0));
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 4096));
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 4097)); // invalid

  // create new database
  hashdb_t hashdb(temp_dir, 4096, 20);

  // import some elements
  int status;
  status = hashdb.import(import_input);

  // invalid mode
  hashdb_t::scan_input_t scan_input;
  hashdb_t::scan_output_t scan_output;
  std::cout << "may emit scan mode error here.\n";
  status = hashdb.scan(scan_input, scan_output);
  BOOST_TEST_NE(status, 0);
}

void do_scan() {

  // valid hashdigest values
  hash_t k1;
  hash_t k2;
  to_key(0, k1);
  to_key(0, k2);

  // open to scan
  hashdb_t hashdb(temp_dir);

  hashdb_t::scan_input_t input;
  hashdb_t::scan_output_t output;

  // populate input
  input.push_back(k1);
  input.push_back(k2);

  // perform scan
  hashdb.scan(input, output);
  BOOST_TEST_EQ(output.size(), 2);
  BOOST_TEST_EQ(output[0].first, 0);
  BOOST_TEST_EQ(output[0].second, 2);
  BOOST_TEST_EQ(output[1].first, 1);
  BOOST_TEST_EQ(output[0].second, 2);
}

int cpp_main(int argc, char* argv[]) {
  do_import();
  do_scan();

  // done
  int status = boost::report_errors();
  return status;
}

