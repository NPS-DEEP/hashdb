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
 * Test the red-black tree map.
 */

#include <config.h>
#include "map_red_black_tree.hpp"
#include <iostream>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "hashdb_types.h"
#include "map_red_black_tree.hpp"

int cpp_main(int argc, char* argv[]) {
  // red-black tree
  //  typedef map_red_black_tree_t<uint8_t[16], uint64_t> red_black_tree_t;
  typedef map_red_black_tree_t<uint64_t, uint64_t> red_black_tree_t;
  typedef std::pair<red_black_tree_t::map_const_iterator, bool> rbtree_pair_t;

  red_black_tree_t* map;
  map_stats_t map_stats;
  rbtree_pair_t rbtree_pair; 
  size_t num_erased;
  red_black_tree_t::map_const_iterator rbtree_it;

  // clean up from any previous run
  remove("temp_rbtree");

  // create new map
  map = new red_black_tree_t("temp_rbtree", RW_NEW);

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

  // add duplicate
  rbtree_pair = map->emplace(1000005, 0);
  BOOST_TEST_EQ(rbtree_pair.second, false);

  // add new
  rbtree_pair = map->emplace(2000005, 0);
  BOOST_TEST_EQ(rbtree_pair.second, true);

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
  rbtree_pair = map->change(1000006, 60);
  BOOST_TEST_EQ(rbtree_pair.second, true);

  // change entry invalid
  rbtree_pair = map->change(6000006, 60);
  BOOST_TEST_EQ(rbtree_pair.second, false);

  // check count stayed same
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // validate map integrity by looking for keys using find
  rbtree_it = map->find(1000003);
  BOOST_TEST_EQ(rbtree_it->second, 3);
  rbtree_it = map->find(2000003); // should = map->end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(rbtree_it, map->end());
  bool temp = (rbtree_it == map->end());
  BOOST_TEST_EQ(temp, true);

  // validate map integrity by looking for keys using has
  BOOST_TEST_EQ(map->has(1000003), true);
  BOOST_TEST_EQ(map->has(2000003), false);

  // end RW tests
  delete map;

  // ************************************************************
  // RO tests
  // ************************************************************
  map = new red_black_tree_t("temp_rbtree", READ_ONLY);

  // check count
  map_stats = map->get_map_stats();
  BOOST_TEST_EQ(map_stats.count_size, 1000000);

  // validate map integrity by looking for keys
  BOOST_TEST_EQ(map->has(1000003), true);
  BOOST_TEST_EQ(map->has(2000003), false);

  // try to edit the RO map
  BOOST_TEST_THROWS(rbtree_pair = map->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = map->erase(0), std::runtime_error);
  BOOST_TEST_THROWS(rbtree_pair = map->change(0, 0), std::runtime_error);

  // ************************************************************
  // done
  // ************************************************************
  int status = boost::report_errors();
  return status;
}

