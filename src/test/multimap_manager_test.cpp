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
  typedef std::pair<multimap_iterator_t<T>, bool> multimap_pair_t;
  multimap_pair_t multimap_pair;
  size_t num_erased;
  class multimap_iterator_t<T> multimap_it;

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

/*
  // add duplicate
  to_key(105, key);
  multimap_pair = multimap_manager->emplace(key, 0);
  BOOST_TEST_EQ(multimap_pair.second, false);

  // add new
  to_key(205, key);
  multimap_pair = multimap_manager->emplace(key, 0);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 101);

  // remove entry positive
  to_key(105, key);
  num_erased = multimap_manager->erase(key);
  BOOST_TEST_EQ(num_erased, 1);

  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // remove entry false
  to_key(105, key);
  num_erased = multimap_manager->erase(key);
  BOOST_TEST_EQ(num_erased, 0);

  // check count
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // change entry
  to_key(106, key);
  multimap_pair = multimap_manager->change(key, 60);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // change entry invalid
  to_key(106, key);
  multimap_pair = multimap_manager->change(key, 60);
  BOOST_TEST_EQ(multimap_pair.second, false);

  // check count stayed same
  BOOST_TEST_EQ(multimap_manager->size(), 100);

  // validate multimap manager integrity by looking for keys using find
  to_key(103, key);
  multimap_it = multimap_manager->find(key);
  BOOST_TEST_EQ((*multimap_it).second, 3);
  to_key(203, key);
  multimap_it = multimap_manager->find(key); // should = multimap_manager->end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(multimap_it, multimap_manager->end());
  bool temp = (multimap_it == multimap_manager->end());
  BOOST_TEST_EQ(temp, true);

  // validate multimap manager integrity by looking for keys using has
  to_key(103, key);
  BOOST_TEST_EQ(multimap_manager->has(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(multimap_manager->has(key), false);

  // validate iterator
  multimap_it = multimap_manager->begin();
  class multimap_iterator_t<T> multimap_it2 = multimap_manager->end();
  bool temp_equals = (multimap_it == multimap_it2);
  BOOST_TEST_EQ(temp_equals, false);
  temp_equals = (multimap_it != multimap_it2);
  BOOST_TEST_EQ(temp_equals, true);
  multimap_it++; multimap_it++; multimap_it++; multimap_it++;  // +4
//  not for unordered
//  BOOST_TEST_EQ(multimap_it->second, 4);
//  BOOST_TEST_EQ((*multimap_it).second, 4);

  int i=4;
  for (; multimap_it != multimap_manager->end(); ++multimap_it) {
    i++;
  }
  BOOST_TEST_EQ(i, 100);

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
  BOOST_TEST_EQ(multimap_manager->has(key), true);
  to_key(203, key);
  BOOST_TEST_EQ(multimap_manager->has(key), false);

  // try to edit the RO multimap manager
  to_key(0, key);
  BOOST_TEST_THROWS(multimap_pair = multimap_manager->emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = multimap_manager->erase(key), std::runtime_error);
  BOOST_TEST_THROWS(multimap_pair = multimap_manager->change(key, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

*/
  // end RO tests
  std::cout << "about to delete multimap_manager RO.\n";
  delete multimap_manager;
}

/*
template<typename T>
void run_multimap_manager_tests() {
  typedef std::pair<class T::multimap_const_iterator, bool> multimap_pair_t;

  T* multimap;
  multimap_stats_t multimap_stats;
  multimap_pair_t multimap_pair; 
  size_t num_erased;
  class T::multimap_const_iterator multimap_it;
  class T::multimap_const_iterator end_it;
  class T::multimap_const_iterator_range multimap_it_range;

  // clean up from any previous run
  remove(temp_file);

  // create new multimap
  multimap = new T(temp_file, RW_NEW);

  // populate with 1,000,000 entries
  for (uint64_t n=0; n< 1000000; ++n) {
    multimap_manager->emplace(to_key<T>(n+1000000), n);
  }

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(multimap_stats.count_size, 1000000);
  BOOST_TEST_EQ(multimap_manager->count(), 1000000);

  // add same key, different value
  multimap_pair = multimap_manager->emplace(to_key<T>(1000005), 0);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // add same key, different value
  multimap_pair = multimap_manager->emplace(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // doesn't add same key, same value
  multimap_pair = multimap_manager->emplace(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(multimap_pair.second, false);

  // range operation, 1 key, 1 value
  multimap_it_range = multimap_manager->equal_range(to_key<T>(1000000));
  BOOST_TEST(multimap_it_range.first != multimap_manager->end());
  ++multimap_it_range.first;
  BOOST_TEST(multimap_it_range.first == multimap_it_range.second);

  // range operation, 1 key, 3 values
  multimap_it_range = multimap_manager->equal_range(to_key<T>(1000005));
  BOOST_TEST(multimap_it_range.first != multimap_manager->end());
  BOOST_TEST(multimap_it_range.first != multimap_it_range.second);
  ++multimap_it_range.first;
  ++multimap_it_range.first;
  BOOST_TEST(multimap_it_range.first != multimap_manager->end());
  ++multimap_it_range.first;
  BOOST_TEST(multimap_it_range.first == multimap_it_range.second);

  // range operation, no key
  multimap_it_range = multimap_manager->equal_range(to_key<T>(2000005));
  BOOST_TEST(multimap_it_range.first == multimap_manager->end());
  BOOST_TEST(multimap_it_range.second == multimap_manager->end());

  // count for key
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(2000005)), 0);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000004)), 1);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000005)), 3);

  // find
  multimap_it = multimap_manager->find(to_key<T>(1000005), 0);
  BOOST_TEST(multimap_it != multimap_manager->end());
  multimap_it = multimap_manager->find(to_key<T>(1000005), 1);
  BOOST_TEST(multimap_it != multimap_manager->end());
  multimap_it = multimap_manager->find(to_key<T>(1000005), 5);
  BOOST_TEST(multimap_it != multimap_manager->end());
  multimap_it = multimap_manager->find(to_key<T>(1000005), 6);
  BOOST_TEST(multimap_it == multimap_manager->end());

  // has
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000005), 0), true);
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000005), 1), true);
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000005), 5), true);
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000005), 6), false);

  // erase
  num_erased = multimap_manager->erase(to_key<T>(1000004), 4); // valid
  BOOST_TEST_EQ(num_erased, 1);
  num_erased = multimap_manager->erase(to_key<T>(1000004), 4); // not valid now
  BOOST_TEST_EQ(num_erased, 0);
  num_erased = multimap_manager->erase(to_key<T>(2000004), 4); // never valid
  BOOST_TEST_EQ(num_erased, 0);

  // put back 1000004, 4
  multimap_pair = multimap_manager->emplace(to_key<T>(1000004), 4);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // erase same key multiple values
  num_erased = multimap_manager->erase(to_key<T>(1000005), 0);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000005)), 2);
  num_erased = multimap_manager->erase(to_key<T>(1000005), 1);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000005)), 1);
  num_erased = multimap_manager->erase(to_key<T>(1000005), 5);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000005)), 0);
  num_erased = multimap_manager->erase(to_key<T>(1000005), 6);
  BOOST_TEST_EQ(multimap_manager->count(to_key<T>(1000005)), 0);

  // put back 1000005, 5
  multimap_pair = multimap_manager->emplace(to_key<T>(1000005), 5);
  BOOST_TEST_EQ(multimap_pair.second, true);

  // end RW tests
  delete multimap;

  // ************************************************************
  // RO tests
  // ************************************************************
  multimap = new T(temp_file, READ_ONLY);

  // check count
  BOOST_TEST_EQ(multimap_manager->count(), 1000000);

  // validate multimap integrity by looking for keys
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000003), 3), true);
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(1000003), 4), false);
  BOOST_TEST_EQ(multimap_manager->has(to_key<T>(2000003), 0), false);

  // try to edit the RO multimap
  BOOST_TEST_THROWS(multimap_pair = multimap_manager->emplace(0, 0), std::runtime_error);
  BOOST_TEST_THROWS(num_erased = multimap_manager->erase(0, 0), std::runtime_error);

  // NOTE: btree assert failure if exit without delete.

  // end RO tests
  delete multimap;
}
*/

int cpp_main(int argc, char* argv[]) {

  // multimap tests
//  run_multimap_manager_tests<md5_t>(MULTIMAP_BTREE);
//  run_multimap_manager_tests<sha1_t>(MULTIMAP_BTREE);
//  run_multimap_manager_tests<sha256_t>(MULTIMAP_BTREE);
  run_multimap_manager_tests<md5_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_tests<sha1_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_tests<sha256_t>(MULTIMAP_FLAT_SORTED_VECTOR);
  run_multimap_manager_tests<md5_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_tests<sha1_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_tests<sha256_t>(MULTIMAP_RED_BLACK_TREE);
  run_multimap_manager_tests<md5_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_tests<sha1_t>(MULTIMAP_UNORDERED_HASH);
  run_multimap_manager_tests<sha256_t>(MULTIMAP_UNORDERED_HASH);

  // done
  int status = boost::report_errors();
  std::cout << "multimap_manager_test done.\n";
  return status;
}

