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
 * Test the bi_store module.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "directory_helper.hpp"
#include "bi_data_types.hpp"
#include "bi_store.hpp"
#include "file_modes.h"
#include <sys/stat.h> // makedir

typedef bi_store_t<bi_data_64_pair_t> source_lookup_store_t;

static const char temp_dir[] = "temp_dir_bi_store_test";
static const std::string temp_store = "temp_dir_bi_store_test/source_lookup_store";

void run_setup() {
  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  source_lookup_store_t store(temp_store, RW_NEW);
  store.insert_value(std::pair<uint64_t,uint64_t>(2,3));
}

void run_test() {

  source_lookup_store_t store1(temp_store, READ_ONLY);
  source_lookup_store_t store2(temp_store, READ_ONLY);

  std::pair<uint64_t, uint64_t> value_pair;
  store1.get_value(1, value_pair);
  BOOST_TEST_EQ(value_pair.first, 2);
  store2.get_value(1, value_pair);
  BOOST_TEST_EQ(value_pair.first, 2);
}

int cpp_main(int argc, char* argv[]) {

  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  run_setup();
  run_test();

  // done
  int status = boost::report_errors();
  return status;
}

