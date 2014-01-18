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
#include "file_modes.h"
#include "dfxml/src/hash_t.h"

static const char temp_file[] = "temp_file";

// make one of these for each hash type to test
md5_t to_key(uint64_t i) {
  std::ostringstream ss;
  ss << std::setw(32) << std::setfill('0') << std::hex << i;
  std::string temp = ss.str();
  return md5_t::fromhex(temp);
}

// provide this for the unordered hash map and multimap
inline std::size_t hash_value(const md5_t& md5) {
  return boost::hash_value<unsigned char,16>(md5.digest);
}

template<typename T>
void run_map_tests() {
  typedef std::pair<class T::map_const_iterator, bool> map_pair_t;

  T* map;
  map_pair_t map_pair; 
  size_t num_erased;
  class T::map_const_iterator map_it;

  // clean up from any previous run
  remove(temp_file);

  // create new map
  map = new T(temp_file, RW_NEW);

  // populate with 1,000,000 entries
  for (uint64_t n=0; n< 1000000; ++n) {
    map->emplace(to_key(n+1000000), n);
  }

// force failed test just to see the output
//BOOST_TEST_EQ(map->size(), 1000001);

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // add duplicate
  map_pair = map->emplace(to_key(1000005), 0);
  BOOST_TEST_EQ(map_pair.second, false);

  // add new
  map_pair = map->emplace(to_key(2000005), 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // check count
  BOOST_TEST_EQ(map->size(), 1000001);

  // remove entry positive
  num_erased = map->erase(to_key(1000005));
  BOOST_TEST_EQ(num_erased, 1);

  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // remove entry false
  num_erased = map->erase(to_key(1000005));
  BOOST_TEST_EQ(num_erased, 0);

  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // change entry
  map_pair = map->change(to_key(1000006), 60);
  BOOST_TEST_EQ(map_pair.second, true);

  // change entry invalid
  map_pair = map->change(to_key(1000006), 60);
  BOOST_TEST_EQ(map_pair.second, false);

  // check count stayed same
  BOOST_TEST_EQ(map->size(), 1000000);

  // validate map integrity by looking for keys using find
  map_it = map->find(to_key(1000003));
  BOOST_TEST_EQ(map_it->second, 3);
  map_it = map->find(to_key(2000003)); // should = map->end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(map_it, map->end());
  bool temp = (map_it == map->end());
  BOOST_TEST_EQ(temp, true);

  // validate map integrity by looking for keys using has
  BOOST_TEST_EQ(map->has(to_key(1000003)), true);
  BOOST_TEST_EQ(map->has(to_key(2000003)), false);

  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new T(temp_file, READ_ONLY);

  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map->has(to_key(1000003)), true);
  BOOST_TEST_EQ(map->has(to_key(2000003)), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(map_pair = map->emplace(to_key(0), 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map->erase(to_key(0)), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map->change(to_key(0), 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map;
}

template<typename T>
void run_multimap_tests() {
  typedef std::pair<class T::map_const_iterator, bool> map_pair_t;

  T* map;
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
    map->emplace(to_key(n+1000000), n);
  }

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // add same key, different value
  map_pair = map->emplace(to_key(1000005), 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // add same key, different value
  map_pair = map->emplace(to_key(1000005), 1);
  BOOST_TEST_EQ(map_pair.second, true);

  // doesn't add same key, same value
  map_pair = map->emplace(to_key(1000005), 1);
  BOOST_TEST_EQ(map_pair.second, false);

  // range operation, 1 key, 1 value
  map_it_range = map->equal_range(to_key(1000000));
  BOOST_TEST(map_it_range.first != map->end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, 1 key, 3 values
  map_it_range = map->equal_range(to_key(1000005));
  BOOST_TEST(map_it_range.first != map->end());
  BOOST_TEST(map_it_range.first != map_it_range.second);
  ++map_it_range.first;
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first != map->end());
  ++map_it_range.first;
  BOOST_TEST(map_it_range.first == map_it_range.second);

  // range operation, no key
  map_it_range = map->equal_range(to_key(2000005));
  BOOST_TEST(map_it_range.first == map->end());
  BOOST_TEST(map_it_range.second == map->end());

  // count for key
  BOOST_TEST_EQ(map->count(to_key(2000005)), 0);
  BOOST_TEST_EQ(map->count(to_key(1000004)), 1);
  BOOST_TEST_EQ(map->count(to_key(1000005)), 3);

  // find
  map_it = map->find(to_key(1000005), 0);
  BOOST_TEST(map_it != map->end());
  map_it = map->find(to_key(1000005), 1);
  BOOST_TEST(map_it != map->end());
  map_it = map->find(to_key(1000005), 5);
  BOOST_TEST(map_it != map->end());
  map_it = map->find(to_key(1000005), 6);
  BOOST_TEST(map_it == map->end());

  // has
  BOOST_TEST_EQ(map->has(to_key(1000005), 0), true);
  BOOST_TEST_EQ(map->has(to_key(1000005), 1), true);
  BOOST_TEST_EQ(map->has(to_key(1000005), 5), true);
  BOOST_TEST_EQ(map->has(to_key(1000005), 6), false);

  // erase
  num_erased = map->erase(to_key(1000004), 4); // valid
  BOOST_TEST_EQ(num_erased, 1);
  num_erased = map->erase(to_key(1000004), 4); // not valid now
  BOOST_TEST_EQ(num_erased, 0);
  num_erased = map->erase(to_key(2000004), 4); // never valid
  BOOST_TEST_EQ(num_erased, 0);

  // put back 1000004, 4
  map_pair = map->emplace(to_key(1000004), 4);
  BOOST_TEST_EQ(map_pair.second, true);

  // erase same key multiple values
  num_erased = map->erase(to_key(1000005), 0);
  BOOST_TEST_EQ(map->count(to_key(1000005)), 2);
  num_erased = map->erase(to_key(1000005), 1);
  BOOST_TEST_EQ(map->count(to_key(1000005)), 1);
  num_erased = map->erase(to_key(1000005), 5);
  BOOST_TEST_EQ(map->count(to_key(1000005)), 0);
  num_erased = map->erase(to_key(1000005), 6);
  BOOST_TEST_EQ(map->count(to_key(1000005)), 0);

  // put back 1000005, 5
  map_pair = map->emplace(to_key(1000005), 5);
  BOOST_TEST_EQ(map_pair.second, true);

  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new T(temp_file, READ_ONLY);

  // check count
  BOOST_TEST_EQ(map->size(), 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map->has(to_key(1000003), 3), true);
  BOOST_TEST_EQ(map->has(to_key(1000003), 4), false);
  BOOST_TEST_EQ(map->has(to_key(2000003), 0), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(map_pair = map->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map->erase(0, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map;
}

//  typedef uint64_t my_key_t;
  typedef md5_t my_key_t;
  typedef uint64_t val_t;

int cpp_main(int argc, char* argv[]) {
  // ************************************************************
  // map
  // ************************************************************
  // red-black-tree map
  run_map_tests<class map_red_black_tree_t<my_key_t, val_t> >();

  // unordered hash map
  run_map_tests<class map_unordered_hash_t<my_key_t, val_t> >();

  // flat sorted vector map
  run_map_tests<class map_flat_sorted_vector_t<my_key_t, val_t> >();

  // btree map
  run_map_tests<class map_btree_t<my_key_t, val_t> >();

  // ************************************************************
  // multimap
  // ************************************************************
  // red-black-tree multimap
  run_multimap_tests<class multimap_red_black_tree_t<my_key_t, val_t> >();

  // unordered hash multimap
  run_multimap_tests<class multimap_unordered_hash_t<my_key_t, val_t> >();

  // flat sorted vector multimap
  run_multimap_tests<class multimap_flat_sorted_vector_t<my_key_t, val_t> >();

  // btree multimap
  run_multimap_tests<class multimap_btree_t<my_key_t, val_t> >();

  // ************************************************************
  // done
  // ************************************************************
  // done
  int status = boost::report_errors();
  return status;
}

