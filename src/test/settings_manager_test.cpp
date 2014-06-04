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
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

// Save with a changed value, then read, delete, write, and read,
// then see if changed value is preserved.
void run_test() {

  // clean up from any previous run
  rm_hashdb_dir("temp_dir");

  // create and write settings
  hashdb_settings_t settings;
  settings.hash_block_size = 512;

  // write settings
  hashdb_settings_manager_t::write_settings("temp_dir", settings);

  // read, delete, write, read settings
  settings = hashdb_settings_manager_t::read_settings("temp_dir");
  rm_hashdb_dir("temp_dir");
  hashdb_settings_manager_t::write_settings("temp_dir", settings);
  settings = hashdb_settings_manager_t::read_settings("temp_dir");

  // attempt to read an invalid filename
  BOOST_TEST_EQ(settings.hash_block_size, 512);
}

int cpp_main(int argc, char* argv[]) {
  run_test();

  // done
  int status = boost::report_errors();
  return status;
}

