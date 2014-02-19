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
#include "map_manager.hpp"
#include "hashdb_map_only_manager.hpp"
#include "hashdb_map_only_iterator.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_map[] = "temp_dir/hash_store";
static const char temp_settings[] = "temp_dir/settings.xml";

template<typename T>
void run_tests() {

  T key;
  std::pair<map_iterator_t<T>, bool> map_action_pair;

  // perform setup inside control block so map_manager resources get released
  {
    // create map manager
    map_manager_t<T> map_manager(temp_dir, RW_NEW, MAP_BTREE);

    // put 1 element into map
    to_key(101, key);
    map_action_pair = map_manager.emplace(key, 1);
  }

  // open hashdb_map_only_manager
  hashdb_map_only_manager_t manager(temp_dir, READ_ONLY);
  hashdb_map_only_iterator_t it = manager.begin();
  hashdb_map_only_iterator_t end_it = manager.end();

  // see that count is correct
  BOOST_TEST_EQ(it->second, 1);

  // verify end function
  ++it;
  BOOST_TEST_EQ((it == end_it), true);
}

int cpp_main(int argc, char* argv[]) {

  hashdb_settings_t settings;
  settings.hashdigest_type = HASHDIGEST_MD5;
  remove(temp_settings);
  hashdb_settings_manager_t::write_settings(std::string(temp_dir), settings);
  run_tests<md5_t>();
  settings.hashdigest_type = HASHDIGEST_SHA1;
  remove(temp_settings);
  hashdb_settings_manager_t::write_settings(std::string(temp_dir), settings);
  run_tests<sha1_t>();
  settings.hashdigest_type = HASHDIGEST_SHA256;
  remove(temp_settings);
  hashdb_settings_manager_t::write_settings(std::string(temp_dir), settings);
  run_tests<sha256_t>();

  // done
  int status = boost::report_errors();
  return status;
}

