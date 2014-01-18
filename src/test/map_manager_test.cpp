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
 * Test the maps and multimaps.
 * Map and multimap tests are linked together here to ensure that the
 * compiler can handle them all at once.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "map_manager.hpp"
#include "file_modes.h"
#include "dfxml/src/hash_t.h"
#include "map_iterator.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_file[] = "temp_dir/temp_file";

// make one of these for each hash type to test
void to_key(uint64_t i, md5_t key) {
  std::ostringstream ss;
  ss << std::setw(16*2) << std::setfill('0') << std::hex << i;
  key = md5_t::fromhex(ss.str());
}

void to_key(uint64_t i, sha1_t key) {
  std::ostringstream ss;
  ss << std::setw(20*2) << std::setfill('0') << std::hex << i;
  key = sha1_t::fromhex(ss.str());
}

void to_key(uint64_t i, sha256_t key) {
  std::ostringstream ss;
  ss << std::setw(32*2) << std::setfill('0') << std::hex << i;
  key = sha256_t::fromhex(ss.str());
}

template<typename T>
void run_map_tests() {

  T key;
  map_manager_t* map_manager;
/*
  std::pair<map_manager_iterator_md5_t, bool> map_pair_t
  map_pair_t map_pair
*/
  size_t num_erased;

  // clean up from any previous run
  remove(temp_file);

  // create new map manager
  map_manager = new map_manager_t(temp_dir, RW_NEW, MAP_BTREE, HASH_ALGORITHM_MD5);

  // populate with 100 entries
  for (uint64_t n=0; n< 100; ++n) {
    to_key(n+100, key);
/*
    map_manager->emplace(key, n);
*/
  }

// force failed test just to see the output
//BOOST_TEST_EQ(map_stats.count_size, 101);

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map_manager->size(), 100);

/*
  // add duplicate
  to_key(105, key);
  map_pair = map_manager->emplace(key, 0);
  BOOST_TEST_EQ(map_pair.second, false);

  // add new
  to_key(205, key);
  map_pair = map_manager->emplace(key, 0);
  BOOST_TEST_EQ(map_pair.second, true);
*/

  // check count
  BOOST_TEST_EQ(map_manager->size(), 101);

  // remove entry positive
  to_key(105, key);
  num_erased = map_manager->erase(key);
  BOOST_TEST_EQ(num_erased, 1);

  // check count
  BOOST_TEST_EQ(map_manager->size(), 100);

  // remove entry false
  to_key(105, key);
  num_erased = map_manager->erase(key);
  BOOST_TEST_EQ(num_erased, 0);

  // check count
  BOOST_TEST_EQ(map_manager->size(), 100);

/*
  // change entry
  to_key(106, key);
  map_pair = map_manager->change(key, 60);
  BOOST_TEST_EQ(map_pair.second, true);

  // change entry invalid
  to_key(106, key);
  map_pair = map_manager->change(key, 60);
  BOOST_TEST_EQ(map_pair.second, false);
*/

  // check count stayed same
  BOOST_TEST_EQ(map_manager->size(), 100);

/*
  // validate map manager integrity by looking for keys using find
  to_key(103, key);
  map_it = map_manager->find(key);
  BOOST_TEST_EQ(map_it->second, 3);
  to_key(203, key);
  map_it = map_manager->find(key); // should = map_manager->end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(map_it, map_manager->end());
  bool temp = (map_it == map_manager->end());
  BOOST_TEST_EQ(temp, true);
*/

  // validate map manager integrity by looking for keys using has
  to_key(103, key);
  BOOST_TEST_EQ(map_manager->has(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(map_manager->has(key), false);

  // end RW tests
  delete map_manager;

  // ************************************************************
  // RO tests
  // ************************************************************
  map_manager = new map_manager_t(temp_dir, READ_ONLY, MAP_BTREE, HASH_ALGORITHM_MD5);

  // check count
  BOOST_TEST_EQ(map_manager->size(), 100);

  // validate map manager integrity by looking for keys
  to_key(103, key);
  BOOST_TEST_EQ(map_manager->has(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(map_manager->has(key), false);

  // try to edit the RO map manager
/*
  to_key(0, key);
  BOOST_TEST_THROWS(map_pair = map_manager->emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map_manager->erase(key), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map_manager->change(key, 0), std::runtime_error);
*/

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map_manager;
}

/*
template<typename T>
void run_multimap_tests() {
  typedef std::pair<class T::map_const_iterator, bool> map_pair_t;

  T* map;
  map_stats_t map_stats;
  map_pair_t map_pair; 
  size_t num_erased;
  class T::map_const_iterator map_it;
  class T::map_const_iterator end_it;
  class T::map_const_iterator_range map_it_range;

  // clean up from any previous run
  remove(temp_file);

  // create new map
  map = new T(temp_file, RW_NEW);

  // populate with 1,000,000 entries
  for (uint64_t n=0; n< 1000000; ++n) {
    map_manager->emplace(to_key<T>(n+1000000), n);
  }

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map_stats.count_size, 1000000);
  BOOST_TEST_EQ(map_manager->count(), 1000000);

  // add same key, different value
  map_pair = map_manager->emplace(to_key<T>(1000005), 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // add same key, different value
  map_pair = map_manager->emplace(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(map_pair.second, true);

  // doesn't add same key, same value
  map_pair = map_manager->emplace(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(map_pair.second, false);

  // range operation, 1 key, 1 value
  map_it_range = map_manager->equal_range(to_key<T>(1000000));
  BOOST_TEST(map_it_range.first != map_manager->end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, 1 key, 3 values
  map_it_range = map_manager->equal_range(to_key<T>(1000005));
  BOOST_TEST(map_it_range.first != map_manager->end());
  BOOST_TEST(map_it_range.first != map_it_range.second);
  ++map_it_range.first;
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first != map_manager->end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, no key
  map_it_range = map_manager->equal_range(to_key<T>(2000005));
  BOOST_TEST(map_it_range.first == map_manager->end());
  BOOST_TEST(map_it_range.second == map_manager->end());

  // count for key
  BOOST_TEST_EQ(map_manager->count(to_key<T>(2000005)), 0);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000004)), 1);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000005)), 3);

  // find
  map_it = map_manager->find(to_key<T>(1000005), 0);
  BOOST_TEST(map_it != map_manager->end());
  map_it = map_manager->find(to_key<T>(1000005), 1);
  BOOST_TEST(map_it != map_manager->end());
  map_it = map_manager->find(to_key<T>(1000005), 5);
  BOOST_TEST(map_it != map_manager->end());
  map_it = map_manager->find(to_key<T>(1000005), 6);
  BOOST_TEST(map_it == map_manager->end());

  // has
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000005), 0), true);
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000005), 1), true);
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000005), 5), true);
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000005), 6), false);

  // erase
  num_erased = map_manager->erase(to_key<T>(1000004), 4); // valid
  BOOST_TEST_EQ(num_erased, 1);
  num_erased = map_manager->erase(to_key<T>(1000004), 4); // not valid now
  BOOST_TEST_EQ(num_erased, 0);
  num_erased = map_manager->erase(to_key<T>(2000004), 4); // never valid
  BOOST_TEST_EQ(num_erased, 0);

  // put back 1000004, 4
  map_pair = map_manager->emplace(to_key<T>(1000004), 4);
  BOOST_TEST_EQ(map_pair.second, true);

  // erase same key multiple values
  num_erased = map_manager->erase(to_key<T>(1000005), 0);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000005)), 2);
  num_erased = map_manager->erase(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000005)), 1);
  num_erased = map_manager->erase(to_key<T>(1000005), 5);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000005)), 0);
  num_erased = map_manager->erase(to_key<T>(1000005), 6);
  BOOST_TEST_EQ(map_manager->count(to_key<T>(1000005)), 0);

  // put back 1000005, 5
  map_pair = map_manager->emplace(to_key<T>(1000005), 5);
  BOOST_TEST_EQ(map_pair.second, true);

  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new T(temp_file, READ_ONLY);

  // check count
  BOOST_TEST_EQ(map_manager->count(), 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000003), 3), true);
  BOOST_TEST_EQ(map_manager->has(to_key<T>(1000003), 4), false);
  BOOST_TEST_EQ(map_manager->has(to_key<T>(2000003), 0), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(map_pair = map_manager->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map_manager->erase(0, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map;
}
*/

void run_temp_test() {

  map_manager_t* map_manager;
  map_manager = new map_manager_t(
                          temp_dir, RW_NEW, MAP_BTREE, HASH_ALGORITHM_MD5);

  map_iterator_md5_t map_iterator_md5(MAP_BTREE, false,
                                    map_manager->map_btree_md5,
                                    map_manager->map_flat_sorted_vector_md5,
                                    map_manager->map_red_black_tree_md5,
                                    map_manager->map_unordered_hash_md5);

  delete map_manager;
}
  

int cpp_main(int argc, char* argv[]) {

  // map tests
  run_temp_test();
//  run_map_tests<md5_t>();
//  run_map_tests<sha1_t>();

  // done
  int status = boost::report_errors();
  return status;
}

