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
 * Test the hashdb iterator
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "dfxml/src/hash_t.h"
#include "file_modes.h"
#include "map_types.h"
#include "multimap_types.h"
#include "map_manager.hpp"
#include "multimap_manager.hpp"
#include "hashdb_iterator.hpp"
#include "source_lookup_encoding.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_map[] = "temp_dir/temp_map";
static const char temp_multimap[] = "temp_dir/temp_multimap";

template<typename T>
void run_rw_tests(map_type_t map_type, multimap_type_t multimap_type) {

  T key;
  uint64_t pay;
  hashdb_iterator_t<T> it;
  hashdb_iterator_t<T> it_end;
  bool is_done;

  // clean up from any previous run
  remove(temp_map);
  remove(temp_multimap);

  // create map manager
  map_manager_t<T> map_manager(temp_map, RW_NEW, map_type);

  // create multimap manager
  multimap_manager_t<T> multimap_manager(temp_multimap, RW_NEW, multimap_type);

  // put 1 element into map
  to_key(101, key);
  map_manager.emplace(key, 1);

  // walk map of 1 element
  it = hashdb_iterator_t<T>(&map_manager, &multimap_manager, false);
  it_end = hashdb_iterator_t<T>(&map_manager, &multimap_manager, true);
  
  BOOST_TEST_EQ(it->second, 1);
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, false);
  ++it;
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, true);

  // have element in map forward to element in multimap
  to_key(101, key);
  pay = source_lookup_encoding::get_source_lookup_encoding(2);
  map_manager.emplace(key, pay);
  multimap_manager.emplace(key, 201);

  // walk multimap of 1 element
  it = hashdb_iterator_t<T>(&map_manager, &multimap_manager, false);
  it_end = hashdb_iterator_t<T>(&map_manager, &multimap_manager, true);
  
  BOOST_TEST_EQ(it->second, 201);
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, false);
  ++it;
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, true);
}

template<typename T>
void run_ro_tests(map_type_t map_type, multimap_type_t multimap_type) {
  // no action
}

int cpp_main(int argc, char* argv[]) {

  run_rw_tests<md5_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_ro_tests<md5_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_rw_tests<md5_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_ro_tests<md5_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_rw_tests<md5_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_ro_tests<md5_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_rw_tests<md5_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  run_ro_tests<md5_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  run_rw_tests<sha1_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_ro_tests<sha1_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_rw_tests<sha1_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_ro_tests<sha1_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_rw_tests<sha1_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_ro_tests<sha1_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_rw_tests<sha1_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  run_ro_tests<sha1_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  run_rw_tests<sha256_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_ro_tests<sha256_t>(MAP_BTREE, MULTIMAP_BTREE);
  run_rw_tests<sha256_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_ro_tests<sha256_t>(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  run_rw_tests<sha256_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_ro_tests<sha256_t>(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  run_rw_tests<sha256_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  run_ro_tests<sha256_t>(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);

  // done
  int status = boost::report_errors();
  std::cout << "multimap_manager_test done.\n";
  return status;
}

