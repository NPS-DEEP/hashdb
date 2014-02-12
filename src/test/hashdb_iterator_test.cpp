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
 * Basically, make it compile and work over one element.
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
#include "map_types.h"
#include "multimap_types.h"
#include "map_manager.hpp"
#include "multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "hashdb_iterator.hpp"
#include "source_lookup_index_manager.hpp"
#include "hashdb_element.hpp"
#include "hashdb_element_lookup.hpp"
#include "settings.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_map[] = "temp_dir/hash_store";
static const char temp_multimap[] = "temp_dir/hash_duplicates_store";

template<typename T>
void run_tests() {

  T key;
  std::pair<map_iterator_t<T>, bool> map_action_pair;

  // clean up from any previous run
  remove(temp_map);
  remove(temp_multimap);

  // create map manager
  map_manager_t<T> map_manager(temp_dir, RW_NEW, MAP_BTREE);

  // create multimap manager
  multimap_manager_t<T> multimap_manager(temp_dir, RW_NEW, MULTIMAP_BTREE);

  // create resources so iterator works
  source_lookup_index_manager_t source_lookup_index_manager(temp_dir, RW_NEW);
  settings_t settings;
  hashdb_element_lookup_t hashdb_element_lookup(&source_lookup_index_manager,
                                                &settings);

  // put 1 element into map
  to_key(101, key);
  map_action_pair = map_manager.emplace(key, 1);

  // put something for "1" in the source lookup index manager
  source_lookup_index_manager.insert("rep1", "file1");

  // create map_multimap iterator
  map_multimap_iterator_t<T> map_multimap_it =
           map_multimap_iterator_t<T>(&map_manager, &multimap_manager, false);
  map_multimap_iterator_t<T> map_multimap_end_it =
           map_multimap_iterator_t<T>(&map_manager, &multimap_manager, true);

  // create hashdb_iterator from map_multimap iterator
  hashdb_iterator_t hashdb_it(map_multimap_it, hashdb_element_lookup);
  hashdb_iterator_t hashdb_end_it(map_multimap_end_it, hashdb_element_lookup);

  // validate iterator value
// too difficult, test later  BOOST_TEST_EQ(hashdb_it->repository_name, "rep1");
  ++hashdb_it;
  BOOST_TEST_EQ((hashdb_it == hashdb_end_it), true);
}

int cpp_main(int argc, char* argv[]) {

  run_tests<md5_t>();
  run_tests<sha1_t>();
  run_tests<sha256_t>();

  // done
  int status = boost::report_errors();
  return status;
}

