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
#include "dfxml/src/hash_t.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";

template<typename T>
void do_import(std::string hashdigest_type) {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // valid hashdigest values
  T k1;
//  T k2;
//  T k3;
  to_key(1, k1);
//  to_key(2, k2);
//  to_key(3, k3);

  // input for import
  std::vector<hashdb_t::import_element_t<T> > import_input;
  import_input.push_back(hashdb_t::import_element_t<T>(k1, "rep1", "file1", 0));
  import_input.push_back(hashdb_t::import_element_t<T>(k1, "rep1", "file1", 4096));
  import_input.push_back(hashdb_t::import_element_t<T>(k1, "rep1", "file1", 4097)); // invalid

  // create new database
  hashdb_t hashdb(temp_dir, hashdigest_type, 4096, 20);

  // import some elements
  int status;
  status = hashdb.import(import_input);

  // invalid mode
  std::vector<std::pair<uint64_t, T> > scan_input;
  std::vector<std::pair<uint64_t, uint32_t> > scan_output;
  status = hashdb.scan(scan_input, scan_output);
  BOOST_TEST_NE(status, 0);
}

template<typename T>
void do_scan() {

  // valid hashdigest values
  T k1;
  T k2;
 // T k3;
  to_key(1, k1);
  to_key(2, k2);
 // to_key(3, k3);

  // open to scan
  hashdb_t hashdb(temp_dir);

  std::vector<std::pair<uint64_t, T> > input;
  std::vector<std::pair<uint64_t, uint32_t> > output;

  // populate input
  input.push_back(std::pair<uint64_t, T>(1, k1));
  input.push_back(std::pair<uint64_t, T>(2, k2));

  // perform scan
  hashdb.scan(input, output);
  BOOST_TEST_EQ(output[0].first, 1);
  BOOST_TEST_EQ(output[0].second, 2);
  BOOST_TEST_EQ(output.size(), 1);
}

int cpp_main(int argc, char* argv[]) {
  do_import<md5_t>("MD5");
  do_scan<md5_t>();
  do_import<sha1_t>("SHA1");
  do_scan<sha1_t>();
  do_import<sha256_t>("SHA256");
  do_scan<sha256_t>();

  // done
  int status = boost::report_errors();
  return status;
}

