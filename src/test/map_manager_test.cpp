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
 * Test the map manager
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "map_manager.hpp"
#include "file_modes.h"
#include "dfxml/src/hash_t.h"
#include "map_iterator.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_file[] = "temp_dir/hash_store";

template<typename T>
void run_map_manager_rw_tests(map_type_t map_type) {

  T key;
  typedef std::pair<map_iterator_t<T>, bool> map_pair_t;
  map_pair_t map_pair;
  bool did_erase;
  class map_iterator_t<T> map_it;

  // clean up from any previous run
  remove(temp_file);

  // create new map manager
  map_manager_t<T> map_manager(temp_dir, RW_NEW, map_type);

  // populate with 100 entries
  for (uint64_t n=0; n< 100; ++n) {
    to_key(n+100, key);
    map_manager.emplace(key, n);
  }

// force failed test just to see the output
//  BOOST_TEST_EQ(map_manager.size(), 101);

  // ************************************************************
  // RW tests
  // ************************************************************
  // check count
  BOOST_TEST_EQ(map_manager.size(), 100);

  // add duplicate
  to_key(105, key);
  map_pair = map_manager.emplace(key, 0);
  BOOST_TEST_EQ(map_pair.second, false);

  // add new
  to_key(205, key);
  map_pair = map_manager.emplace(key, 0);
  BOOST_TEST_EQ(map_pair.second, true);

  // check count
  BOOST_TEST_EQ(map_manager.size(), 101);

  // remove entry positive
  to_key(105, key);
  did_erase = map_manager.erase(key);
  BOOST_TEST_EQ(did_erase, true);

  // check count
  BOOST_TEST_EQ(map_manager.size(), 100);

  // remove entry false
  to_key(105, key);
  did_erase = map_manager.erase(key);
  BOOST_TEST_EQ(did_erase, false);

  // check count
  BOOST_TEST_EQ(map_manager.size(), 100);

  // change entry
  to_key(106, key);
  map_pair = map_manager.change(key, 60);
  BOOST_TEST_EQ(map_pair.second, true);

  // change entry invalid
  to_key(106, key);
  map_pair = map_manager.change(key, 60);
  BOOST_TEST_EQ(map_pair.second, false);

  // check count stayed same
  BOOST_TEST_EQ(map_manager.size(), 100);

  // validate map manager integrity by looking for keys using find
  to_key(103, key);
  map_it = map_manager.find(key);
  BOOST_TEST_EQ((*map_it).second, 3);
  to_key(203, key);
  map_it = map_manager.find(key); // should = map_manager.end()

  // compiler can't handle this, so use simpler alternative.
  // BOOST_TEST_EQ(map_it, map_manager.end());
  bool temp = (map_it == map_manager.end());
  BOOST_TEST_EQ(temp, true);

  // validate map manager integrity by looking for keys using find_count
  to_key(103, key);
  BOOST_TEST_EQ(map_manager.find_count(key), 1);
  to_key(203, key);
  BOOST_TEST_EQ(map_manager.find_count(key), 0);

  // validate iterator
  map_it = map_manager.begin();
  class map_iterator_t<T> map_it2 = map_manager.end();
  bool temp_equals = (map_it == map_it2);
  BOOST_TEST_EQ(temp_equals, false);
  temp_equals = (map_it != map_it2);
  BOOST_TEST_EQ(temp_equals, true);
  map_it++; map_it++; map_it++; map_it++;  // +4
/* not for unordered
  BOOST_TEST_EQ(map_it->second, 4);
  BOOST_TEST_EQ((*map_it).second, 4);
*/
  int i=4;
  for (; map_it != map_manager.end(); ++map_it) {
    i++;
  }
  BOOST_TEST_EQ(i, 100);
}

template<typename T>
void run_map_manager_ro_tests(map_type_t map_type) {

  T key;
  typedef std::pair<map_iterator_t<T>, bool> map_pair_t;
  map_pair_t map_pair;
  class map_iterator_t<T> map_it;

  // ************************************************************
  // RO tests
  // ************************************************************
  // create new map manager
  map_manager_t<T> map_manager(temp_dir, READ_ONLY, map_type);

  // check count
  BOOST_TEST_EQ(map_manager.size(), 100);

  // validate map manager integrity by looking for keys
  to_key(103, key);
  BOOST_TEST_EQ(map_manager.find_count(key), 1);
  to_key(203, key);
  BOOST_TEST_EQ(map_manager.find_count(key), 0);

  // try to edit the RO map manager
  to_key(0, key);
  BOOST_TEST_THROWS(map_pair = map_manager.emplace(key, 0), std::runtime_error);
  BOOST_TEST_THROWS(map_manager.erase(key), std::runtime_error);
  BOOST_TEST_THROWS(map_pair = map_manager.change(key, 0), std::runtime_error);
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
  // map tests
  run_map_manager_rw_tests<md5_t>(MAP_BTREE);
  run_map_manager_ro_tests<md5_t>(MAP_BTREE);
  run_map_manager_rw_tests<sha1_t>(MAP_BTREE);
  run_map_manager_ro_tests<sha1_t>(MAP_BTREE);
  run_map_manager_rw_tests<sha256_t>(MAP_BTREE);
  run_map_manager_ro_tests<sha256_t>(MAP_BTREE);
  run_map_manager_rw_tests<md5_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_ro_tests<md5_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_rw_tests<sha1_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_ro_tests<sha1_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_rw_tests<sha256_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_ro_tests<sha256_t>(MAP_FLAT_SORTED_VECTOR);
  run_map_manager_rw_tests<md5_t>(MAP_RED_BLACK_TREE);
  run_map_manager_ro_tests<md5_t>(MAP_RED_BLACK_TREE);
  run_map_manager_rw_tests<sha1_t>(MAP_RED_BLACK_TREE);
  run_map_manager_ro_tests<sha1_t>(MAP_RED_BLACK_TREE);
  run_map_manager_rw_tests<sha256_t>(MAP_RED_BLACK_TREE);
  run_map_manager_ro_tests<sha256_t>(MAP_RED_BLACK_TREE);
  run_map_manager_rw_tests<md5_t>(MAP_UNORDERED_HASH);
  run_map_manager_ro_tests<md5_t>(MAP_UNORDERED_HASH);
  run_map_manager_rw_tests<sha1_t>(MAP_UNORDERED_HASH);
  run_map_manager_ro_tests<sha1_t>(MAP_UNORDERED_HASH);
  run_map_manager_rw_tests<sha256_t>(MAP_UNORDERED_HASH);
  run_map_manager_ro_tests<sha256_t>(MAP_UNORDERED_HASH);

  // done
  int status = boost::report_errors();
  return status;
}

