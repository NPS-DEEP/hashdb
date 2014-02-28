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

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "hashdb.hpp"
#include "to_key_helper.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "dfxml/src/hash_t.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";
//static const char temp_hash_store[] = "temp_dir/hash_store";
static const char temp_settings[] = "temp_dir/settings.xml";
static const char temp_bloom_filter_1[] = "temp_dir/bloom_filter_1";
static const char temp_hash_store[] = "temp_dir/hash_store";
static const char temp_hash_duplicates_store[] = "temp_dir/hash_duplicates_store";
/*
static const char temp_source_lookup_store_dat[] = "temp_dir/source_lookup_store.dat";
static const char temp_source_lookup_store_idx1[] = "temp_dir/source_lookup_store.idx1";
static const char temp_source_lookup_store_idx2[] = "temp_dir/source_lookup_store.idx2";
static const char temp_source_repository_name_store_dat[] = "temp_dir/source_repository_name_store.dat";
static const char temp_source_repository_name_store_idx1[] = "temp_dir/source_repository_name_store.idx1";
static const char temp_source_repository_name_store_idx2[] = "temp_dir/source_repository_name_store.idx2";
static const char temp_source_filename_store_dat[] = "temp_dir/source_filename_store.dat";
static const char temp_source_filename_store_idx1[] = "temp_dir/source_filename_store.idx1";
static const char temp_source_filename_store_idx2[] = "temp_dir/source_filename_store.idx2";
*/

template<typename T>
void do_import(std::string hashdigest_type) {
  // clean up from any previous run
  remove(temp_settings);
  remove(temp_bloom_filter_1);
  remove(temp_hash_store);
  remove(temp_hash_duplicates_store);

  // valid hashdigest values
  T k1;
//  T k2;
//  T k3;
  to_key(1, k1);
//  to_key(2, k2);
//  to_key(3, k3);

  // create new database
  hashdb_t hashdb(temp_dir, hashdigest_type, 4096, 20);

  // import some elements
  int status;
  status = hashdb.import(k1, "rep1", "file1", 0);
  BOOST_TEST_EQ(status, 0);
  status = hashdb.import(k1, "rep1", "file1", 4096);
  BOOST_TEST_EQ(status, 0);
  status = hashdb.import(k1, "rep1", "file1", 4097); // invalid block size
  BOOST_TEST_EQ(status, 0);

  // invalid mode
  std::vector<std::pair<uint64_t, T> > input;
  std::vector<std::pair<uint64_t, uint32_t> > output;
  status = hashdb.scan(input, output);
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

