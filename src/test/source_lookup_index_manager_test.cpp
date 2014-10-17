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
 * Test the bloom filter manager
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "directory_helper.hpp"
#include "source_lookup_index_manager.hpp"
#include "file_modes.h"
#include <sys/stat.h> // makedir

static const char temp_dir[] = "temp_dir_source_lookup_index_manager_test";

void run_test() {

  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  source_lookup_index_manager_t manager(temp_dir, RW_NEW);
  std::pair<bool, uint64_t> pair_bool_64;

  // check that the empty iterator is functional
  BOOST_TEST_EQ(manager.begin() == manager.end(), true);

  // add first source
  pair_bool_64 = manager.insert("rep_a", "file_a");
  BOOST_TEST_EQ(pair_bool_64.first, true);
  BOOST_TEST_EQ(pair_bool_64.second, 1);

  // attempt to add a duplicate
  pair_bool_64 = manager.insert("rep_a", "file_a");
  BOOST_TEST_EQ(pair_bool_64.first, false);
  BOOST_TEST_EQ(pair_bool_64.second, 1);  // existing index

  // add
  pair_bool_64 = manager.insert("rep_a", "file_b");
  BOOST_TEST_EQ(pair_bool_64.first, true);
  BOOST_TEST_EQ(pair_bool_64.second, 2);

  // add
  pair_bool_64 = manager.insert("rep_b", "file_a");
  BOOST_TEST_EQ(pair_bool_64.first, true);
  BOOST_TEST_EQ(pair_bool_64.second, 3);

  // add
  pair_bool_64 = manager.insert("rep_b", "file_b");
  BOOST_TEST_EQ(pair_bool_64.first, true);
  BOOST_TEST_EQ(pair_bool_64.second, 4);

  // add
  pair_bool_64 = manager.insert("rep_b", "file_b");
  BOOST_TEST_EQ(pair_bool_64.first, false);
  BOOST_TEST_EQ(pair_bool_64.second, 4);

  // find index
  pair_bool_64 = manager.find("rep_a", "file_b");
  BOOST_TEST_EQ(pair_bool_64.first, true);
  BOOST_TEST_EQ(pair_bool_64.second, 2);

  // find index
  pair_bool_64 = manager.find("rep_a", "file_c");
  BOOST_TEST_EQ(pair_bool_64.first, false);
  BOOST_TEST_EQ(pair_bool_64.second, 0);

  // find index
  pair_bool_64 = manager.find("rep_c", "file_a");
  BOOST_TEST_EQ(pair_bool_64.first, false);
  BOOST_TEST_EQ(pair_bool_64.second, 0);

  // find from index
  std::pair<bool, std::pair<std::string, std::string> > pair_string;
  pair_string = manager.find(1);
  BOOST_TEST_EQ(pair_string.second.first, "rep_a");
  BOOST_TEST_EQ(pair_string.second.second, "file_a");

  // find from index
  pair_string = manager.find(2);
  BOOST_TEST_EQ(pair_string.second.first, "rep_a");
  BOOST_TEST_EQ(pair_string.second.second, "file_b");

  // find from index
  pair_string = manager.find(3);
  BOOST_TEST_EQ(pair_string.second.first, "rep_b");
  BOOST_TEST_EQ(pair_string.second.second, "file_a");

  // check the iterator with values available
  source_lookup_index_manager_t::source_lookup_index_iterator_t it = manager.begin();
  pair_string = manager.find(it->key);
  BOOST_TEST_EQ(pair_string.second.first, "rep_a");
  BOOST_TEST_EQ(pair_string.second.second, "file_a");
  ++it; ++it; ++it; ++it;
}

int cpp_main(int argc, char* argv[]) {

  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  run_test();

  // done
  int status = boost::report_errors();
  return status;
}

