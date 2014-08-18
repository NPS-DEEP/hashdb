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
 * Test command interfaces.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "directory_helper.hpp"
#include "../hash_t_selector.h"
#include "commands.hpp"
#include "hashdb_manager.hpp"
#include "file_modes.h"

// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const std::string temp_dir("temp_dir_commands_test.hdb");
static const std::string temp_dir1("temp_dir1_commands_test.hdb");
static const std::string temp_dir2("temp_dir2_commands_test.hdb");
static const std::string temp_dir3("temp_dir3_commands_test.hdb");
static const std::string temp_dir4("temp_dir4_commands_test.hdb");
static const std::string temp_dir5("temp_dir5_commands_test.hdb");
static const std::string temp_dir6("temp_dir6_commands_test.hdb");
static const std::string temp_dir7("temp_dir7_commands_test.hdb");
static const std::string temp_dir8("temp_dir8_commands_test.hdb");
static const char* temp_dfxml_file = "temp_dfxml_out.xml";

static const std::string identified_blocks(HASHDB_TEST_DATADIR "identified_blocks.txt");
static const std::string sample_dfxml4096(HASHDB_TEST_DATADIR "sample_dfxml4096.xml");
static const std::string sample_dfxml512(HASHDB_TEST_DATADIR "sample_dfxml512.xml");

// validate correct size of db
void check_size(const std::string hashdb_dir, size_t size) {
  hashdb_manager_t<hash_t> manager(hashdb_dir, READ_ONLY);
  BOOST_TEST_EQ(manager.map_size(), size);
}

// import/export: import1, export, import2 should retain size of 74
void test_import_export() {
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  remove(temp_dfxml_file);

  // import
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir1);
  commands_t<hash_t>::do_export(temp_dir1, "temp_dfxml_out.xml");
  commands_t<hash_t>::create(settings, temp_dir2);
  commands_t<hash_t>::import("repository2", temp_dfxml_file, temp_dir2);
  check_size(temp_dir2, 74);
}

// database manipulation commands
void test_database_manipulation() {
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  rm_hashdb_dir(temp_dir3);
  rm_hashdb_dir(temp_dir4);
  rm_hashdb_dir(temp_dir5);
  rm_hashdb_dir(temp_dir6);
  rm_hashdb_dir(temp_dir7);
  rm_hashdb_dir(temp_dir8);
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir2);
  commands_t<hash_t>::import("repository2", sample_dfxml4096, temp_dir2);

  // add temp_dir1 to temp_dir2
  std::cout << "add\n";
  commands_t<hash_t>::add(temp_dir1, temp_dir2);
  check_size(temp_dir1, 74);
  check_size(temp_dir2, 2 * 74);
  
  // add_multiple
  std::cout << "add_multiple\n";
  commands_t<hash_t>::create(settings, temp_dir3);
  commands_t<hash_t>::add_multiple(temp_dir1, temp_dir2, temp_dir3);
  check_size(temp_dir3, 2 * 74);

  // intersect
  std::cout << "intersect\n";
  commands_t<hash_t>::create(settings, temp_dir4);
  commands_t<hash_t>::intersect(temp_dir1, temp_dir3, temp_dir4);
  check_size(temp_dir4, 2 * 74);

  // subtract
  std::cout << "subtract\n";
  commands_t<hash_t>::create(settings, temp_dir5);
  commands_t<hash_t>::subtract(temp_dir1, temp_dir4, temp_dir5);
  check_size(temp_dir5, 0);

  // deduplicate
  std::cout << "deduplicate1\n";
  commands_t<hash_t>::create(settings, temp_dir6);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir6);
  commands_t<hash_t>::deduplicate(temp_dir6, temp_dir7);
  check_size(temp_dir7, 74);
  std::cout << "deduplicate2\n";
  commands_t<hash_t>::import("repository2", sample_dfxml4096, temp_dir6);
  commands_t<hash_t>::deduplicate(temp_dir6, temp_dir8);
  // check ability to open existing target
  commands_t<hash_t>::deduplicate(temp_dir6, temp_dir8);
  check_size(temp_dir8, 0);
}

// scan services
void test_scan_services() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir1);

  // scan
  commands_t<hash_t>::scan(temp_dir1, sample_dfxml4096);

  // scan expanded
  commands_t<hash_t>::scan_expanded(temp_dir1, sample_dfxml4096);

  // expand_identified_blocks
  commands_t<hash_t>::expand_identified_blocks(temp_dir1, identified_blocks);

  // server
  // not tested
}

// statistics
void test_statistics() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir1);

  // size
  commands_t<hash_t>::size(temp_dir1);

  // sources
  commands_t<hash_t>::sources(temp_dir1);

  // histogram
  commands_t<hash_t>::histogram(temp_dir1);

  // duplicates
  commands_t<hash_t>::duplicates(temp_dir1, "2");

  // hash_table
  commands_t<hash_t>::hash_table(temp_dir1);
}

// tuning
void test_tuning() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::import("repository1", sample_dfxml4096, temp_dir1);

  // rebuild_bloom
  settings.bloom1_M_hash_size = 20;
  commands_t<hash_t>::rebuild_bloom(settings, temp_dir1);
}

// performance analysis
void test_performance_analysis() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir1);

  // add_random
  // not tested because it requires user input "q" to complete.
  // commands_t<hash_t>::add_random("repo_random", temp_dir1, "100000");

  // scan_random
  // not tested because it takes time to run
  // commands_t<hash_t>::scan_random(temp_dir1);
}

// validate use of multiple repository names
void test_multiple_repository_names() {
  
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // create new hashdb
  hashdb_settings_t settings;
  commands_t<hash_t>::create(settings, temp_dir);

  // test ability to manage multiple repository names
  for (int i=0; i<12; i++) {
    // generate unique repository name
    std::ostringstream ss;
    ss << "test_repository_name_" << i;

    // import
    commands_t<hash_t>::import(ss.str(), sample_dfxml4096, temp_dir);
  }

  // duplicate import should not add elements
  commands_t<hash_t>::import("test_repository_name_0", sample_dfxml4096, temp_dir);

  // validate correct size
  check_size(temp_dir, 74*12);
}

// validate block size control for 4096
void test_block_size_4096() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  remove(temp_dfxml_file);

  // create new hashdb
  hashdb_settings_t settings;
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir2);

  // import
  commands_t<hash_t>::import("test_repository_name", sample_dfxml4096, temp_dir1);
  check_size(temp_dir1, 74);

  // export
  commands_t<hash_t>::do_export(temp_dir1, "temp_dfxml_out.xml");

  // import
  commands_t<hash_t>::import("test_repository_name", "temp_dfxml_out.xml", temp_dir2);
  check_size(temp_dir2, 74);
}

// validate block size control for 512 and max count of 1
void test_block_size_512_and_count_1() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  remove(temp_dfxml_file);

  // create new hashdb
  hashdb_settings_t settings;
  settings.hash_block_size = 512;
  settings.maximum_hash_duplicates  = 1;
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir2);

  // import
  commands_t<hash_t>::import("test_repository_name", sample_dfxml512, temp_dir1);
  commands_t<hash_t>::import("test_repository_name2", sample_dfxml512, temp_dir1);
  check_size(temp_dir1, 24);

  // export
  commands_t<hash_t>::do_export(temp_dir1, "temp_dfxml_out.xml");

  // import
  commands_t<hash_t>::import("test_repository_name", "temp_dfxml_out.xml", temp_dir2);
  check_size(temp_dir1, 24);
}

int cpp_main(int argc, char* argv[]) {
  std::cout << "test import and export\n";
  test_import_export();
  std::cout << "test database manipulation\n";
  test_database_manipulation();
  std::cout << "test scan services\n";
  test_scan_services();
  std::cout << "test statistics\n";
  test_statistics();
  std::cout << "test tuning\n";
  test_tuning();
  std::cout << "test performance analysis\n";
  test_performance_analysis();
  std::cout << "test multiple repository names\n";
  test_multiple_repository_names();
  std::cout << "test block size 4096\n";
  test_block_size_4096();
  std::cout << "test block size 512 and count 1\n";
  test_block_size_512_and_count_1();
  return 0;
}

