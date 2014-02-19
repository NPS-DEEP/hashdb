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
 * Test the map_multimap_manager and map_multimap_iterator.
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
#include "file_modes.h"
#include "map_multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "dfxml/src/hash_t.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";
static const char temp_settings[] = "temp_dir/settings.xml";
//static const char temp_hash_store[] = "temp_dir/hash_store";
static const char temp_bloom_filter_1[] = "temp_dir/bloom_filter_1";

template<typename T>
void rw_new_tests(map_type_t map_type, multimap_type_t multimap_type) {

  T k1;
  T k2;
  T k3;
  to_key(1, k1);
  to_key(2, k2);
  to_key(3, k3);
  const uint64_t v1 = 111;
  const uint64_t v2 = 222;
  const uint64_t v3 = 333;

  map_multimap_iterator_t<T> map_multimap_it;

  // clean up from any previous run
//  remove(temp_hash_store);
  remove(temp_settings);
  remove(temp_bloom_filter_1);

  // create working settings
  hashdb_settings_t settings;
  settings.map_type = MAP_BTREE;
  settings.multimap_type = MULTIMAP_BTREE;
  hashdb_settings_manager_t::write_settings(temp_dir, settings);

  // changes_t
  hashdb_changes_t changes;

  // open manager as new
  map_multimap_manager_t<T> manager(temp_dir, RW_NEW);

  // check initial size
  BOOST_TEST_EQ(manager.map_size(), 0);
  BOOST_TEST_EQ(manager.multimap_size(), 0);

  // check initial iterator
  BOOST_TEST_EQ((manager.begin() == manager.end()), true);

  // populate with test data
  // checks: emplace, changes.hashes_inserted
  manager.emplace(k1, v1, 0, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 1);

  manager.emplace(k2, v2, 0, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 2);
  manager.emplace(k2, v2, 0, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_duplicate_element, 1); // map
  manager.emplace(k2, v3, 0, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 3);
  manager.emplace(k2, v3, 0, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_duplicate_element, 2); // multimap

  manager.emplace(k3, v1, 0, changes);
  manager.emplace(k3, v2, 0, changes);
  manager.emplace(k3, v3, 0, changes);

  BOOST_TEST_EQ(manager.map_size(), 3);
  BOOST_TEST_EQ(manager.multimap_size(), 5);

  // check iterator, may correctly fail for unorderd map types
  map_multimap_iterator_t<T> it;
  map_multimap_iterator_t<T> end_it;
  it = manager.begin();
  end_it = manager.end();
  BOOST_TEST_EQ(it->second, 111);
  ++it; ++it; ++it; ++it; ++it; ++it;
  BOOST_TEST_EQ((it == end_it), true);

  // remove element
  manager.remove(k1, v1, changes);  // from map
  BOOST_TEST_EQ(changes.hashes_removed, 1);
  manager.remove(k1, v1, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 1);
  manager.remove(k2, v1, changes);  // multimap
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 2);
  manager.remove(k2, v2, changes);  // multimap
  BOOST_TEST_EQ(changes.hashes_removed, 2);

  // remove key
  manager.remove_key(k1, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_hash, 1);
  manager.remove_key(k2, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 3);
  manager.remove_key(k2, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_hash, 2);
  manager.remove_key(k3, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 6);

  // check ending size
  BOOST_TEST_EQ(manager.map_size(), 0);
  BOOST_TEST_EQ(manager.multimap_size(), 0);

  // check ending iterator
  BOOST_TEST_EQ((manager.begin() == manager.end()), true);
}

int cpp_main(int argc, char* argv[]) {

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

  // map tests
  rw_new_tests<md5_t>(MAP_BTREE, MULTIMAP_BTREE);
  rw_new_tests<sha1_t>(MAP_BTREE, MULTIMAP_BTREE);
  rw_new_tests<sha256_t>(MAP_BTREE, MULTIMAP_BTREE);

  rw_new_tests<md5_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  rw_new_tests<sha1_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  rw_new_tests<sha256_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);

  rw_new_tests<md5_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  rw_new_tests<sha1_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  rw_new_tests<sha256_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);

  rw_new_tests<md5_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  rw_new_tests<sha1_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  rw_new_tests<sha256_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);

  // done
  int status = boost::report_errors();
  return status;
}

