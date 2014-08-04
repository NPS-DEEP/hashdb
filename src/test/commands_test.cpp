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
#include "../hash_t_selector.h"
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
static const std::string identified_blocks(DATADIR "identified_blocks.txt");
static const std::string sample_dfxml4096(DATADIR "sample_dfxml4096.xml");
static const std::string sample_dfxml512(DATADIR "sample_dfxml512.xml");

// validate correct size of db
void check_size(const std::string hashdb_dir, size_t size) {
  hashdb_manager_t<hash_t> manager(hashdb_dir, READ_ONLY);
  BOOST_TEST_EQ(manager.map_size(), size);
}

// validate all commands in general
void do_test1() {
 
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  rm_hashdb_dir(temp_dir3);
  rm_hashdb_dir(temp_dir4);
  rm_hashdb_dir(temp_dir5);

  // create new hashdbs
  hashdb_settings_t settings;
  settings.bloom1_is_used = false;
  commands_t<hash_t>::create(settings, temp_dir1);
  commands_t<hash_t>::create(settings, temp_dir2);
  commands_t<hash_t>::create(settings, temp_dir3);
  commands_t<hash_t>::create(settings, temp_dir4);
  commands_t<hash_t>::create(settings, temp_dir5);

  // import
  commands_t<hash_t>::import("test_repository_name", sample_dfxml4096, temp_dir1);

  // export
  commands_t<hash_t>::do_export(temp_dir1, "temp_dfxml_out.xml");

  // add
  commands_t<hash_t>::add(temp_dir1, temp_dir1);
  
  // add_multiple
  commands_t<hash_t>::add_multiple(temp_dir1, temp_dir2, temp_dir3);

  // intersect
  commands_t<hash_t>::intersect(temp_dir1, temp_dir3, temp_dir2);

  // subtract
  commands_t<hash_t>::subtract(temp_dir1, temp_dir2, temp_dir3);

  // deduplicate
  commands_t<hash_t>::deduplicate(temp_dir1, temp_dir4);

  // scan
  commands_t<hash_t>::scan(temp_dir5, sample_dfxml4096);

  // scan expanded
  commands_t<hash_t>::scan_expanded(temp_dir5, sample_dfxml4096);

  // expand_identified_blocks
  commands_t<hash_t>::expand_identified_blocks(temp_dir5, identified_blocks);

  // server
  // not tested

  // size
  commands_t<hash_t>::size(temp_dir5);

  // sources
  commands_t<hash_t>::sources(temp_dir5);

  // histogram
  commands_t<hash_t>::histogram(temp_dir5);

  // duplicates
  commands_t<hash_t>::duplicates(temp_dir5, "2");

  // hash_table
  commands_t<hash_t>::hash_table(temp_dir5);

  // rebuild_bloom
  commands_t<hash_t>::rebuild_bloom(settings, temp_dir5);

  // add_random
  commands_t<hash_t>::add_random("repo_random", temp_dir5, "100000");

  // scan_random
  // not tested because it takes time to run
  // commands_t<hash_t>::scan_random(temp_dir5);
}

// validate use of multiple repository names
void do_test2() {
  
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
void do_test3() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);

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
void do_test4() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);

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
  std::cout << "test1\n";
  do_test1();
  std::cout << "test2\n";
  do_test2();
  std::cout << "test3\n";
  do_test3();
  std::cout << "test4\n";
  do_test4();
  return 0;
}

