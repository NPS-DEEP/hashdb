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
#include "dfxml/src/hash_t.h"
#include "file_modes.h"
#include "multimap_iterator.hpp"
#include "multimap_manager.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_file[] = "temp_dir/hash_duplicates_store";

// provide these for the unordered hash map and multimap
inline std::size_t hash_value(const md5_t& key) {
  return boost::hash_value<unsigned char,16>(key.digest);
}
inline std::size_t hash_value(const sha1_t& key) {
  return boost::hash_value<unsigned char,20>(key.digest);
}
inline std::size_t hash_value(const sha256_t& key) {
  return boost::hash_value<unsigned char,32>(key.digest);
}

// make one of these for each hash type to test
void to_key(uint64_t i, md5_t& key) {
  std::ostringstream ss;
  ss << std::setw(16*2) << std::setfill('0') << std::hex << i;
  key = md5_t::fromhex(ss.str());
}

void to_key(uint64_t i, sha1_t& key) {
  std::ostringstream ss;
  ss << std::setw(20*2) << std::setfill('0') << std::hex << i;
  key = sha1_t::fromhex(ss.str());
}

void to_key(uint64_t i, sha256_t& key) {
  std::ostringstream ss;
  ss << std::setw(32*2) << std::setfill('0') << std::hex << i;
  key = sha256_t::fromhex(ss.str());
}

template<typename T>
void run_multimap_manager_tests(multimap_type_t multimap_type) {

  T key;
  multimap_manager_t<T>* multimap_manager;
  bool did_emplace;
  bool are_equal;
  class multimap_iterator_t<T> multimap_it;
  typedef std::pair<class multimap_iterator_t<T>, class multimap_iterator_t<T> > range_pair_t;
  range_pair_t range_pair;

  // clean up from any previous run
  remove(temp_file);

  // create new multimap manager
  multimap_manager = new multimap_manager_t<T>(temp_dir, RW_NEW, multimap_type);

  // populate with 100 entries
  for (uint64_t n=0; n< 100; ++n) {
    to_key(n+100, key);
    multimap_manager->emplace(key, n);
  }

// force failed test just to see the output
//  BOOST_TEST_EQ(multimap_manager->size(), 101);
  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // add duplicate good
  to_key(105, key);
  did_emplace = multimap_manager->emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, true);

  // add duplicate bad
  to_key(105, key);
  did_emplace = multimap_manager->emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, false);

  // add duplicate bad
  to_key(105, key);
  did_emplace = multimap_manager->emplace(key, 5);
  BOOST_TEST_EQ(did_emplace, false);

  // emplace second value to key
  to_key(205, key);
  did_emplace = multimap_manager->emplace(key, 0);
  BOOST_TEST_EQ(did_emplace, true);

  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 102);

  // check range for key 103 with single entry
  to_key(103, key);
  range_pair = multimap_manager->equal_range(key);
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
  range_pair = multimap_manager->equal_range(key);
  BOOST_TEST_EQ(range_pair.first->second, 5);
  ++range_pair.first;
  BOOST_TEST_EQ(range_pair.first->second, 0);
  ++range_pair.first;

  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);

  // check range for non-existant key 206
  to_key(206, key);
  range_pair = multimap_manager->equal_range(key);
  are_equal=range_pair.first == range_pair.second;
  BOOST_TEST_EQ(are_equal, true);
  
  // check "has"
  to_key(105, key);
  BOOST_TEST_EQ(multimap_manager->has(key, 5), true);
  BOOST_TEST_EQ(multimap_manager->has(key, 0), true);
  BOOST_TEST_EQ(multimap_manager->has(key, 6), false);
  to_key(206, key);
  BOOST_TEST_EQ(multimap_manager->has(key, 0), false);

  // check "has_range"
  to_key(205, key);
  BOOST_TEST_EQ(multimap_manager->has_range(key), true);
  to_key(206, key);
  BOOST_TEST_EQ(multimap_manager->has_range(key), false);

  // erase range
  to_key(205, key);
  BOOST_TEST_EQ(multimap_manager->erase_range(key), true);
  BOOST_TEST_EQ(multimap_manager->erase_range(key), false);
  BOOST_TEST_EQ(multimap_manager->emplace(key, 5), true);
  BOOST_TEST_EQ(multimap_manager->emplace(key, 5), false);

  // erase 110 and 111
  to_key(110, key);
  BOOST_TEST_EQ(multimap_manager->erase(key, 10), true);
  to_key(111, key);
  BOOST_TEST_EQ(multimap_manager->erase_range(key), true);
  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // end RW tests
  delete multimap_manager;

  // ************************************************************
  // RO tests
  // ************************************************************
  multimap_manager = new multimap_manager_t<T>(temp_dir, READ_ONLY, multimap_type);

  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // validate multimap manager integrity by looking for keys
  to_key(103, key);
  BOOST_TEST_EQ(multimap_manager->has_range(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(multimap_manager->has_range(key), false);

  // try to edit the RO multimap manager
  to_key(0, key);
  BOOST_TEST_THROWS(did_emplace = multimap_manager->emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(multimap_manager->erase(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(did_emplace = multimap_manager->erase_range(key), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  std::cout << "about to delete multimap_manager RO.\n";
  delete multimap_manager;
}


int cpp_main(int argc, char* argv[]) {

  // multimap tests
//  run_multimap_manager_tests<md5_t>(MULTIMAP_BTREE);
//  run_multimap_manager_tests<sha1_t>(MULTIMAP_BTREE);
//  run_multimap_manager_tests<sha256_t>(MULTIMAP_BTREE);
std::cout << "tests.a\n";
  run_multimap_manager_tests<md5_t>(MULTIMAP_FLAT_SORTED_VECTOR);
std::cout << "tests.b\n";
  run_multimap_manager_tests<sha1_t>(MULTIMAP_FLAT_SORTED_VECTOR);
std::cout << "tests.c\n";
  run_multimap_manager_tests<sha256_t>(MULTIMAP_FLAT_SORTED_VECTOR);
std::cout << "tests.d\n";
  run_multimap_manager_tests<md5_t>(MULTIMAP_RED_BLACK_TREE);
std::cout << "tests.e\n";
  run_multimap_manager_tests<sha1_t>(MULTIMAP_RED_BLACK_TREE);
std::cout << "tests.f\n";
  run_multimap_manager_tests<sha256_t>(MULTIMAP_RED_BLACK_TREE);
std::cout << "tests.g\n";
  run_multimap_manager_tests<md5_t>(MULTIMAP_UNORDERED_HASH);
std::cout << "tests.h\n";
  run_multimap_manager_tests<sha1_t>(MULTIMAP_UNORDERED_HASH);
std::cout << "tests.i\n";
  run_multimap_manager_tests<sha256_t>(MULTIMAP_UNORDERED_HASH);
std::cout << "tests.j\n";

  // done
  int status = boost::report_errors();
  std::cout << "multimap_manager_test done.\n";
  return status;
}

