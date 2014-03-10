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
 * Test the multimap manager
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
#include "multimap_iterator.hpp"
#include "multimap_manager.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_file[] = "temp_dir/hash_duplicates_store";

template<typename T>
void run_multimap_manager_rw_tests(multimap_type_t multimap_type) {

  // clean up from any previous run
  remove(temp_file);

  T key;
  multimap_manager_t<T> multimap_manager(temp_dir, RW_NEW, multimap_type);
  bool did_emplace;
  bool are_equal;
  class multimap_iterator_t<T> multimap_it;
  typedef std::pair<class multimap_iterator_t<T>, class multimap_iterator_t<T> > range_pair_t;
  range_pair_t range_pair;

  // populate with 100 entries
  for (uint64_t n=0; n< 100; ++n) {
    to_key(n+100, key);
    multimap_manager.emplace(key, n);
  }

// force failed test just to see the output
//  BOOST_TEST_EQ(multimap_manager.size(), 101);
  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(multimap_manager.size(), 100);

  // add duplicate good
  to_key(105, key);
  did_emplace = multimap_manager.emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, true);

  // add duplicate bad
  to_key(105, key);
  did_emplace = multimap_manager.emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, false);

  // add duplicate bad
  to_key(105, key);
  did_emplace = multimap_manager.emplace(key, 5);
  BOOST_TEST_EQ(did_emplace, false);

  // emplace second value to key
  to_key(205, key);
  did_emplace = multimap_manager.emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, true);

  // check count
  BOOST_TEST_EQ(multimap_manager.size(), 102);

  // check range for key 103 with single entry
  to_key(103, key);
  range_pair = multimap_manager.equal_range(key);
  BOOST_TEST_EQ(range_pair.first->second, 3);
  ++range_pair.first;
  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);

  // check range for key 203 with no entry
  to_key(203, key);
  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);

  // check range for key 105
  to_key(105, key);
  range_pair = multimap_manager.equal_range(key);
  BOOST_TEST_EQ(range_pair.first->second, 5);
  ++range_pair.first;
  BOOST_TEST_EQ(range_pair.first->second, 0);
  ++range_pair.first;

  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);

  // check range for non-existant key 206
  to_key(206, key);
  range_pair = multimap_manager.equal_range(key);
  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);
  
  // check "has"
  to_key(105, key);
  BOOST_TEST_EQ(multimap_manager.has(key, 5), true);
  BOOST_TEST_EQ(multimap_manager.has(key, 0), true);
  BOOST_TEST_EQ(multimap_manager.has(key, 6), false);
  to_key(206, key);
  BOOST_TEST_EQ(multimap_manager.has(key, 0), false);

  // check "has_range"
  to_key(205, key);
  BOOST_TEST_EQ(multimap_manager.has_range(key), true);
  to_key(206, key);
  BOOST_TEST_EQ(multimap_manager.has_range(key), false);

  // erase range
  to_key(205, key);
  BOOST_TEST_EQ(multimap_manager.erase_range(key), true);
  BOOST_TEST_EQ(multimap_manager.erase_range(key), false);
  BOOST_TEST_EQ(multimap_manager.emplace(key, 5), true);
  BOOST_TEST_EQ(multimap_manager.emplace(key, 5), false);

  // erase 110 and 111
  to_key(110, key);
  BOOST_TEST_EQ(multimap_manager.erase(key, 10), true);
  to_key(111, key);
  BOOST_TEST_EQ(multimap_manager.erase_range(key), true);
  // check count
  BOOST_TEST_EQ(multimap_manager.size(), 100);
}

template<typename T>
void run_multimap_manager_ro_tests(multimap_type_t multimap_type) {

  T key;
  multimap_manager_t<T> multimap_manager(temp_dir, READ_ONLY, multimap_type);
  class multimap_iterator_t<T> multimap_it;
  typedef std::pair<class multimap_iterator_t<T>, class multimap_iterator_t<T> > range_pair_t;
  range_pair_t range_pair;

  // ************************************************************
  // RO tests
  // ************************************************************

  // check count
  BOOST_TEST_EQ(multimap_manager.size(), 100);

  // validate multimap manager integrity by looking for keys
  to_key(103, key);
  BOOST_TEST_EQ(multimap_manager.has_range(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(multimap_manager.has_range(key), false);

  // try to edit the RO multimap manager
  to_key(0, key);
  BOOST_TEST_THROWS(multimap_manager.emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(multimap_manager.erase(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(multimap_manager.erase_range(key), std::runtime_error);
}


int cpp_main(int argc, char* argv[]) {

  // if temp_dir does not exist, create it
  if (access(temp_dir, F_OK) != 0) {
#ifdef WIN32
    mkdir(temp_dir);
#else
    mkdir(temp_dir,0777);
#endif
  }

  // multimap tests
  run_multimap_manager_rw_tests<md5_t>(MULTIMAP_BTREE);
  run_multimap_manager_ro_tests<md5_t>(MULTIMAP_BTREE);
  run_multimap_manager_rw_tests<sha1_t>(MULTIMAP_BTREE);
  run_multimap_manager_ro_tests<sha1_t>(MULTIMAP_BTREE);
  run_multimap_manager_rw_tests<sha256_t>(MULTIMAP_BTREE);
  run_multimap_manager_ro_tests<sha256_t>(MULTIMAP_BTREE);
  run_multimap_manager_rw_tests<md5_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_ro_tests<md5_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_rw_tests<sha1_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_ro_tests<sha1_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_rw_tests<sha256_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_ro_tests<sha256_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_rw_tests<md5_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_ro_tests<md5_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_rw_tests<sha1_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_ro_tests<sha1_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_rw_tests<sha256_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_ro_tests<sha256_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_rw_tests<md5_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_ro_tests<md5_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_rw_tests<sha1_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_ro_tests<sha1_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_rw_tests<sha256_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_ro_tests<sha256_t>(MULTIMAP_UNORDERED_HASH);

  // done
  int status = boost::report_errors();
  return status;
}

