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
#include <iostream>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "hashdb_types.h"

static const char temp_file[] = "temp_file";

template<typename T>
void run_map_tests() {
  typedef std::pair<class T::map_const_iterator, bool> map_pair_t;

  T* map;
  map_stats_t map_stats;
  map_pair_t map_pair; 
  size_t num_erased;
  class T::map_const_iterator map_it;

  // clean up from any previous run
  remove(temp_file);

  // create new map
  map = new T(temp_file, RW_NEW);

  // populate with 1,000,000 entries
  for (uint64_t i=0; i< 1000000; ++i) {
    map->emplace(i+1000000, i);
  }

// force failed test just to see the output
//BOOST_TEST_EQ(map_stats.count_size, 1000001);

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // add duplicate
  map_pair = map->emplace(1000005, 0);
  BOOST_TEST_EQ(map_pair.second, false);

  // add new
  map_pair = map->emplace(2000005, 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000001);

  // remove entry positive
  num_erased = map->erase(1000005);
  BOOST_TEST_EQ(num_erased, 1);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // remove entry false
  num_erased = map->erase(1000005);
  BOOST_TEST_EQ(num_erased, 0);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // change entry
  map_pair = map->change(1000006, 60);
  BOOST_TEST_EQ(map_pair.second, true);

  // change entry invalid
  map_pair = map->change(6000006, 60);
  BOOST_TEST_EQ(map_pair.second, false);

  // check count stayed same
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // validate map integrity by looking for keys using find
  map_it = map->find(1000003);
  BOOST_TEST_EQ(map_it->second, 3);
  map_it = map->find(2000003); // should = map->end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(map_it, map->end());
  bool temp = (map_it == map->end());
  BOOST_TEST_EQ(temp, true);

  // validate map integrity by looking for keys using has
  BOOST_TEST_EQ(map->has(1000003), true);
  BOOST_TEST_EQ(map->has(2000003), false);

  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new T(temp_file, READ_ONLY);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map->has(1000003), true);
  BOOST_TEST_EQ(map->has(2000003), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(map_pair = map->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map->erase(0), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map->change(0, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map;
}

template<typename T>
void run_multimap_tests() {
  typedef std::pair<class T::map_const_iterator, bool> map_pair_t;

  T* map;
  map_stats_t map_stats;
  map_pair_t map_pair; 
  size_t num_erased;
  class T::map_const_iterator map_it;
  class T::map_const_iterator_range map_it_range;
  bool has;

  // clean up from any previous run
  remove(temp_file);

  // create new map
  map = new T(temp_file, RW_NEW);

  // populate with 1,000,000 entries
  for (uint64_t i=0; i< 1000000; ++i) {
    map->emplace(i+1000000, i);
  }

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // add same key, different value
  map_pair = map->emplace(1000005, 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // add same key, different value
  map_pair = map->emplace(1000005, 1);
  BOOST_TEST_EQ(map_pair.second, true);

  // doesn't add same key, same value
  map_pair = map->emplace(1000005, 1);
  BOOST_TEST_EQ(map_pair.second, false);

  // range operation, 1 key, 1 value
  map_it_range = map->equal_range(1000000);
  BOOST_TEST_EQ(map_it_range.first != map->>end());
  BOOST_TEST_EQ(map_it_range.first == map_it_range.second);

  // range operation, 1 key, 3 values
  map_it_range = map->equal_range(1000005);
  BOOST_TEST_EQ(map_it_range.first != map->>end());
  BOOST_TEST_EQ(map_it_range.first != map_it_range.second);
  ++map_it_range.first;
  ++map_it_range.first;
  BOOST_TEST_EQ(map_it_range.first != map->>end());
  BOOST_TEST_EQ(map_it_range.first == map_it_range.second);

  // range operation, no key
  map_it_range = map->equal_range(2000005);
  BOOST_TEST_EQ(map_it_range.first == map->>end());
  BOOST_TEST_EQ(map_it_range.second == map->>end());

  // count for key
  BOOST_TEST_EQ(map->count(2000005), 0);
  BOOST_TEST_EQ(map->count(1000004), 1);
  BOOST_TEST_EQ(map->count(1000005), 3);

  // find
  map_it = map->find(1000005, 0);
  BOOST_TEST_EQ(map_it != map->end());
  map_it = map->find(1000005, 1);
  BOOST_TEST_EQ(map_it != map->end());
  map_it = map->find(1000005, 5);
  BOOST_TEST_EQ(map_it != map->end());
  map_it = map->find(1000005, 6);
  BOOST_TEST_EQ(map_it != map=>end());

  // has
  BOOST_TEST_EQ(map->has(1000005, 0), true);
  BOOST_TEST_EQ(map->has(1000005, 1), true);
  BOOST_TEST_EQ(map->has(1000005, 5), true);
  BOOST_TEST_EQ(map->has(1000005, 6), false);

  // erase
  num_erased = erase(1000004, 4); // valid
  BOOST_TEST_EQ(num_erased, 1);
  num_erased = erase(1000004, 4); // not valid now
  BOOST_TEST_EQ(num_erased, 0);
  num_erased = erase(2000004, 4); // never valid
  BOOST_TEST_EQ(num_erased, 0);

  // erase same key multiple values
  num_erased = erase(1000005, 0);
  BOOST_TEST_EQ(map->count(1000005), 2);
  num_erased = erase(1000005, 1);
  BOOST_TEST_EQ(map->count(1000005), 1);
  num_erased = erase(1000005, 5);
  BOOST_TEST_EQ(map->count(1000005), 0);
  num_erased = erase(1000005, 6);
  BOOST_TEST_EQ(map->count(1000005), 0);

  // put back 1000005, 5
  map_pair = map->emplace(1000005, 5);
  BOOST_TEST_EQ(map_pair.second, true);



  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new T(temp_file, READ_ONLY);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map->has(1000003), true);
  BOOST_TEST_EQ(map->has(2000003), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(map_pair = map->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map->erase(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map->change(0, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete map;
}

int cpp_main(int argc, char* argv[]) {
  // red-black-tree map
  run_map_tests<class map_red_black_tree_t<uint64_t, uint64_t> >();

  // unordered hash map
  run_map_tests<class map_unordered_hash_t<uint64_t, uint64_t> >();

  // flat sorted vector map
  run_map_tests<class map_flat_sorted_vector_t<uint64_t, uint64_t> >();

  // btree map
  run_map_tests<class map_btree_t<uint64_t, uint64_t> >();

  // red-black-tree multimap
  run_multimap_tests<class multimap_red_black_tree_t<uint64_t, uint64_t> >();

  // done
  int status = boost::report_errors();
  return status;
}

