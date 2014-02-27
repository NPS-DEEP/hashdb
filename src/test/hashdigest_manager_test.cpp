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
 * Test the hashdigest_manager.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "map_types.h"
#include "multimap_types.h"
#include "hashdigest_types.h"
#include "file_modes.h"
#include "hashdb_manager.hpp"
#include "hashdigest_iterator.hpp"
#include "hashdigest_manager.hpp"
#include "dfxml/src/hash_t.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";
static const char temp_settings[] = "temp_dir/settings.xml";
static const char temp_bloom_filter_1[] = "temp_dir/bloom_filter_1";
static const char temp_hash_store[] = "temp_dir/hash_store";
static const char temp_hash_duplicates_store[] = "temp_dir/hash_duplicates_store";

void write_settings(hashdigest_type_t hashdigest_type,
                    map_type_t map_type,
                    multimap_type_t multimap_type) {
  // clean up from any previous run
  remove(temp_settings);
  remove(temp_bloom_filter_1);
  remove(temp_hash_store);
  remove(temp_hash_duplicates_store);

  // create working settings
  hashdb_settings_t settings;
  settings.hashdigest_type = hashdigest_type;
  settings.map_type = map_type;
  settings.multimap_type = multimap_type;
  hashdb_settings_manager_t::write_settings(temp_dir, settings);
}

template<typename T>
void build_hashdb(hashdigest_type_t hashdigest_type,
                  map_type_t map_type,
                  multimap_type_t multimap_type) {

  // open new hashdb manager
  hashdb_manager_t manager(temp_dir, RW_NEW);

  // make element to add
  T k1;
  T k3;
  to_key(1, k1);
  hashdigest_t d1(k1);
  hashdb_element_t element(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);

  // create working changes object
  hashdb_changes_t changes;

  // insert element
  manager.insert(element, changes);
}

void do_test() {

  // open hashdigest manager
  hashdigest_manager_t manager(temp_dir, READ_ONLY);
  hashdigest_iterator_t it = manager.begin();

  // check iterator
  BOOST_TEST_EQ(source_lookup_encoding::get_count(it->second), 1);
  ++it;
  BOOST_TEST_EQ((it == manager.end()), true);
}

int cpp_main(int argc, char* argv[]) {
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH

  write_settings(HASHDIGEST_MD5, MAP_BTREE, MULTIMAP_BTREE);
  build_hashdb<md5_t>(HASHDIGEST_MD5, MAP_BTREE, MULTIMAP_BTREE);
  do_test();
  write_settings(HASHDIGEST_SHA1, MAP_BTREE, MULTIMAP_BTREE);
  build_hashdb<sha1_t>(HASHDIGEST_SHA1, MAP_BTREE, MULTIMAP_BTREE);
  do_test();
  write_settings(HASHDIGEST_SHA256, MAP_BTREE, MULTIMAP_BTREE);
  build_hashdb<sha256_t>(HASHDIGEST_SHA256, MAP_BTREE, MULTIMAP_BTREE);
  do_test();

  // done
  int status = boost::report_errors();
  return status;
}

