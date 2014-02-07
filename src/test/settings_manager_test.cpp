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
#include "settings.hpp"
#include "settings_manager.hpp"

static const char temp_dir[] = "temp_dir";
static const char temp_settings[] = "temp_dir/settings.xml";
static const char invalid_filename[] = "temp_dir/__invalid_filename";

void run_test() {

  remove(temp_settings);

  settings_t settings;

  // read, write first
  settings.hashdb_version = 1;
  settings_manager_t::write_settings(temp_dir, settings);
  settings = settings_manager_t::read_settings(temp_dir);
  BOOST_TEST_EQ(settings.hashdb_version, 1);

  // read, write second
  settings.hashdb_version = 2;
  settings_manager_t::write_settings(temp_dir, settings);
  settings = settings_manager_t::read_settings(temp_dir);
  BOOST_TEST_EQ(settings.hashdb_version, 2);

  // attempt to read an invalid filename
  BOOST_TEST_THROWS(
        settings = settings_manager_t::read_settings(invalid_filename),
        std::runtime_error);
}

int cpp_main(int argc, char* argv[]) {
  run_test();

  // done
  int status = boost::report_errors();
  return status;
}

