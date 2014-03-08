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
#include "map_red_black_tree.hpp"
#include "map_unordered_hash.hpp"
#include "map_flat_sorted_vector.hpp"
#include "map_btree.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_btree.hpp"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "file_modes.h"
#include "dfxml/src/hash_t.h"

static const char temp_file[] = "temp_file";

template<typename T, typename K>
void run_map_rw_tests() {
  typedef std::pair<typename T::map_const_iterator_t, bool> map_pair_t;
  K key;

  // clean up from any previous run
  remove(temp_file);

  T map(temp_file, RW_NEW);

  map_pair_t map_pair; 
  size_t num_erased;
  typename T::map_const_iterator_t map_it;

  // populate with 1,000,000 entries
  for (uint64_t n=0; n< 1000000; ++n) {
    to_key(n+1000000, key);
    map.emplace(key, n);
  }

// force failed test just to see the output
//BOOST_TEST_EQ(map.size(), 1000001);

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // add duplicate
  to_key(1000005, key);
std::cout << "map_and_multimap_test.run_map_rw_tests.a\n";
  map_pair = map.emplace(key, 0);
std::cout << "map_and_multimap_test.run_map_rw_tests.b\n";
  BOOST_TEST_EQ(map_pair.second, false);

  // add new
  to_key(2000005, key);
  map_pair = map.emplace(key, 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // check count
  BOOST_TEST_EQ(map.size(), 1000001);

  // remove entry positive
  to_key(1000005, key);
  num_erased = map.erase(key);
  BOOST_TEST_EQ(num_erased, 1);

  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // remove entry false
  to_key(1000005, key);
  num_erased = map.erase(key);
  BOOST_TEST_EQ(num_erased, 0);

  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // change entry
  to_key(1000006, key);
  map_pair = map.change(key, 60);
  BOOST_TEST_EQ(map_pair.second, true);

  // change entry invalid
  to_key(1000006, key);
  map_pair = map.change(key, 60);
  BOOST_TEST_EQ(map_pair.second, false);

  // check count stayed same
  BOOST_TEST_EQ(map.size(), 1000000);

  // validate map integrity by looking for keys using find
  to_key(1000003, key);
  map_it = map.find(key);
  BOOST_TEST_EQ(map_it->second, 3);
  to_key(2000003, key);
  map_it = map.find(key); // should = map.end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(map_it, map.end());
  bool temp = (map_it == map.end());
  BOOST_TEST_EQ(temp, true);

  // validate map integrity by looking for keys using has
  to_key(1000003, key);
  BOOST_TEST_EQ(map.find_count(key), 1);
  to_key(2000003, key);
  BOOST_TEST_EQ(map.find_count(key), 0);
}

template<typename T, typename K>
void run_map_ro_tests() {
//std::cout << "run_map_ro_tests\n";

  typedef std::pair<typename T::map_const_iterator_t, bool> map_pair_t;
  // ************************************************************
  // RO tests
  // ************************************************************
  map_pair_t map_pair; 
  typename T::map_const_iterator_t map_it;
  K key;

  T map(temp_file, READ_ONLY);

  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // validate map integrity by looking for keys
  to_key(1000003, key);
  BOOST_TEST_EQ(map.find_count(key), 1);
  to_key(2000003, key);
  BOOST_TEST_EQ(map.find_count(key), 0);

  // try to edit the RO map
  to_key(0, key);
  BOOST_TEST_THROWS(map_pair = map.emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(map.erase(key), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map.change(key, 0), std::runtime_error);
}

template<typename T, typename K>
void run_multimap_rw_tests() {
//std::cout << "run_multimap_rw_tests\n";
  typedef std::pair<typename T::map_const_iterator_t, bool> map_pair_t;

  // clean up from any previous run
  remove(temp_file);

  T map(temp_file, RW_NEW);
  K key;
  map_pair_t map_pair; 
  bool did_erase;
  typename T::map_const_iterator_t map_it;
  typename T::map_const_iterator_t end_it;
  typename T::map_const_iterator_range_t map_it_range;
  bool did_emplace;

  // populate with 1,000,000 entries
  for (uint64_t n=0; n< 1000000; ++n) {
    to_key(n+1000000, key);
    map.emplace(key, n);
  }

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // add same key, different value
  to_key(1000005, key);
  did_emplace = map.emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, true);

  // add same key, different value
  to_key(1000005, key);
  did_emplace = map.emplace(key, 1);
  BOOST_TEST_EQ(did_emplace, true);

  // doesn't add same key, same value
  to_key(1000005, key);
  did_emplace = map.emplace(key, 1);
  BOOST_TEST_EQ(did_emplace, false);

  // range operation, 1 key, 1 value
  to_key(1000000, key);
  map_it_range = map.equal_range(key);
  BOOST_TEST(map_it_range.first != map.end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, 1 key, 3 values
  to_key(1000005, key);
  map_it_range = map.equal_range(key);
  BOOST_TEST(map_it_range.first != map.end());
  BOOST_TEST(map_it_range.first != map_it_range.second);
  ++map_it_range.first;
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first != map.end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, no key
  to_key(2000005, key);
  map_it_range = map.equal_range(key);
  BOOST_TEST(map_it_range.first == map.end());
  BOOST_TEST(map_it_range.second == map.end());

  // count for key
  to_key(2000005, key);
  BOOST_TEST_EQ(map.count(key), 0);
  to_key(1000004, key);
  BOOST_TEST_EQ(map.count(key), 1);
  to_key(1000005, key);
  BOOST_TEST_EQ(map.count(key), 3);

  // find
  to_key(1000005, key);
  map_it = map.find(key, 0);
  BOOST_TEST(map_it != map.end());
  map_it = map.find(key, 1);
  BOOST_TEST(map_it != map.end());
  map_it = map.find(key, 5);
  BOOST_TEST(map_it != map.end());
  map_it = map.find(key, 6);
  BOOST_TEST(map_it == map.end());

  // has
  to_key(1000005, key);
  BOOST_TEST_EQ(map.has(key, 0), true);
  BOOST_TEST_EQ(map.has(key, 1), true);
  BOOST_TEST_EQ(map.has(key, 5), true);
  BOOST_TEST_EQ(map.has(key, 6), false);

  // erase
  to_key(1000004, key);
  did_erase = map.erase(key, 4); // valid
  BOOST_TEST_EQ(did_erase, true);
  to_key(1000004, key);
  did_erase = map.erase(key, 4); // not valid now
  BOOST_TEST_EQ(did_erase, false);
  to_key(2000004, key);
  did_erase = map.erase(key, 4); // never valid
  BOOST_TEST_EQ(did_erase, false);

  // put back 1000004, 4
  to_key(1000004, key);
  did_emplace = map.emplace(key, 4);
  BOOST_TEST_EQ(did_emplace, true);

  // erase same key multiple values
  to_key(1000005, key);
  did_erase = map.erase(key, 0);
  BOOST_TEST_EQ(map.count(key), 2);
  did_erase = map.erase(key, 1);
  BOOST_TEST_EQ(map.count(key), 1);
  did_erase = map.erase(key, 5);
  BOOST_TEST_EQ(map.count(key), 0);
  did_erase = map.erase(key, 6);
  BOOST_TEST_EQ(map.count(key), 0);

  // put back 1000005, 5
  to_key(1000005, key);
  did_emplace = map.emplace(key, 5);
  BOOST_TEST_EQ(did_emplace, true);
}

template<typename T, typename K>
void run_multimap_ro_tests() {
//std::cout << "run_multimap_ro_tests\n";
  typedef std::pair<typename T::map_const_iterator_t, bool> map_pair_t;
  // ************************************************************
  // RO tests
  // ************************************************************
  T map(temp_file, READ_ONLY);
  K key;

  map_pair_t map_pair; 
  typename T::map_const_iterator_t map_it;
  typename T::map_const_iterator_t end_it;
  typename T::map_const_iterator_range_t map_it_range;

  // check count
  BOOST_TEST_EQ(map.size(), 1000000);

  // validate map integrity by looking for keys
  to_key(1000003, key);
  BOOST_TEST_EQ(map.has(key, 3), true);
  to_key(1000003, key);
  BOOST_TEST_EQ(map.has(key, 4), false);
  to_key(2000003, key);
  BOOST_TEST_EQ(map.has(key, 0), false);

  // try to edit the RO map
  to_key(0, key);
  BOOST_TEST_THROWS(map.emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(map.erase(key, 0), std::runtime_error);
}

//  typedef uint64_t my_key_t;
  typedef md5_t my_key_t;
  typedef uint64_t val_t;

int cpp_main(int argc, char* argv[]) {
  // ************************************************************
  // map
  // ************************************************************
  // red-black-tree map
  run_map_rw_tests<class map_red_black_tree_t<my_key_t, val_t>, my_key_t >();
  run_map_ro_tests<class map_red_black_tree_t<my_key_t, val_t>, my_key_t >();

  // unordered hash map
  run_map_rw_tests<class map_unordered_hash_t<my_key_t, val_t>, my_key_t >();
  run_map_ro_tests<class map_unordered_hash_t<my_key_t, val_t>, my_key_t >();

  // flat sorted vector map
  run_map_rw_tests<class map_flat_sorted_vector_t<my_key_t, val_t>, my_key_t >();
  run_map_ro_tests<class map_flat_sorted_vector_t<my_key_t, val_t>, my_key_t >();

  // btree map
  run_map_rw_tests<class map_btree_t<my_key_t, val_t>, my_key_t >();
  run_map_ro_tests<class map_btree_t<my_key_t, val_t>, my_key_t >();

  // ************************************************************
  // multimap
  // ************************************************************
  // red-black-tree multimap
  run_multimap_rw_tests<class multimap_red_black_tree_t<my_key_t, val_t>, my_key_t >();
  run_multimap_ro_tests<class multimap_red_black_tree_t<my_key_t, val_t>, my_key_t >();

  // unordered hash multimap
  run_multimap_rw_tests<class multimap_unordered_hash_t<my_key_t, val_t>, my_key_t >();
  run_multimap_ro_tests<class multimap_unordered_hash_t<my_key_t, val_t>, my_key_t >();

  // flat sorted vector multimap
  run_multimap_rw_tests<class multimap_flat_sorted_vector_t<my_key_t, val_t>, my_key_t >();
  run_multimap_ro_tests<class multimap_flat_sorted_vector_t<my_key_t, val_t>, my_key_t >();

  // btree multimap
  run_multimap_rw_tests<class multimap_btree_t<my_key_t, val_t>, my_key_t >();
  run_multimap_ro_tests<class multimap_btree_t<my_key_t, val_t>, my_key_t >();

  // ************************************************************
  // done
  // ************************************************************
  // done
  int status = boost::report_errors();
  return status;
}

