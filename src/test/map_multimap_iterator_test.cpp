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
 * Test the map_multimap iterator
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "../hash_t_selector.h"
#include "file_modes.h"
#include "map_manager.hpp"
#include "multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "source_lookup_encoding.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_map[] = "temp_dir/hash_store";
static const char temp_multimap[] = "temp_dir/hash_duplicates_store";

template<typename T>
void run_rw_tests() {

  T key;
  uint64_t pay;
  map_multimap_iterator_t<T> it;
  map_multimap_iterator_t<T> it_end;
  std::pair<typename map_manager_t<T>::map_iterator_t, bool> map_action_pair;
  bool is_done;

  // clean up from any previous run
  remove(temp_map);
  remove(temp_multimap);

  // create map manager
  map_manager_t<T> map_manager(temp_dir, RW_NEW);

  // create multimap manager
  multimap_manager_t<T> multimap_manager(temp_dir, RW_NEW);

  // put 1 element into map
  to_key(101, key);
  map_action_pair = map_manager.emplace(key, 1);
  BOOST_TEST_EQ(map_action_pair.second, true);
  map_action_pair = map_manager.emplace(key, 1);
  BOOST_TEST_EQ(map_action_pair.second, false);

  // walk map of 1 element
  it = map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_manager.begin());
  it_end = map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_manager.end());
  BOOST_TEST_EQ(it->second, 1);
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, false);
  ++it;
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, true);

  // have element in map forward to element in multimap
  to_key(101, key);
  pay = source_lookup_encoding::get_source_lookup_encoding(2);
  map_action_pair = map_manager.change(key, pay);
  BOOST_TEST_EQ(map_action_pair.second, true);
  multimap_manager.emplace(key, 201);

  // walk multimap of 1 element
  it = map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_manager.begin());
  it_end = map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_manager.end());
  
  BOOST_TEST_EQ(it->second, 201);
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, false);
  ++it;
  is_done = (it == it_end);
  BOOST_TEST_EQ(is_done, true);
}

template<typename T>
void run_ro_tests() {
  // no action defined for RO tests
}

int cpp_main(int argc, char* argv[]) {

  make_dir_if_not_there(temp_dir);

  run_rw_tests<hash_t>();
  run_ro_tests<hash_t>();

  // done
  int status = boost::report_errors();
  return status;
}

