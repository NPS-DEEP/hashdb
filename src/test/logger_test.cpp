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
 * Test the hashdb logger.
 * That it runs is a sufficient test,
 * but log.xml may also be inspected.
 */

#include <config.h>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "directory_helper.hpp"
#include "logger.hpp"
#include "hashdb_changes.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"

static const char temp_dir[] = "temp_dir_logger_test";
static const char temp_log[] = "temp_dir_logger_test/log.xml";

void run_test() {

  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);
  hashdb_settings_t settings;
  hashdb_settings_store_t::write_settings(temp_dir, settings);


  hashdb_changes_t changes;

  // check basic usability
  logger_t logger(temp_dir, "logger test");
  logger.add_timestamp("my_timestamp");
  logger.add_hashdb_settings(settings);
  logger.add_hashdb_changes(changes);
  logger.add("add_by_itself", 3);

  // check that logger is not usable once closed
  logger.close();
  BOOST_TEST_THROWS(logger.add_timestamp("already closed"), std::runtime_error);
  BOOST_TEST_THROWS(logger.close(), std::runtime_error);
}

int cpp_main(int argc, char* argv[]) {

  run_test();

  // done
  std::cout << "Logger test completed, inspect log.xml if desired.\n";
  return 0;
}

