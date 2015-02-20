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
#include "unit_test.h"
#include "directory_helper.hpp"
#include "commands.hpp"
#include "file_modes.h"

// for progress tracker
bool quiet_mode;

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

static const std::string identified_blocks("identified_blocks.txt");
static const std::string sample_dfxml4096("sample_dfxml4096.xml");
static const std::string sample_dfxml512("sample_dfxml512.xml");

/*
// get size of db
size_t map_size(const std::string& hashdb_dir) {
  hashdb_manager_t manager(hashdb_dir, READ_ONLY);
  return manager.map_size();
}

// import/export: import1, export, import2 should retain size of 74
void test_import_export() {
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  remove(temp_dfxml_file);

  // import
  commands_t::create(settings, temp_dir1);
  commands_t::import(temp_dir1, sample_dfxml4096, "repository1");
  commands_t::do_export(temp_dir1, "temp_dfxml_out.xml");
  commands_t::create(settings, temp_dir2);
  commands_t::import(temp_dir2, temp_dfxml_file, "repository2");
  TEST_EQ(map_size(temp_dir2), 74);
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
  commands_t::create(settings, temp_dir1);
  commands_t::import(temp_dir1, sample_dfxml4096, "repository1");
  commands_t::create(settings, temp_dir2);
  commands_t::import(temp_dir2, sample_dfxml4096, "repository2");
  commands_t::add_multiple(temp_dir1, temp_dir2, temp_dir3);

  // add
  commands_t::add(temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add(temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add(temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  rm_hashdb_dir(temp_dir4);

  // add_multiple
  std::cout << "add_multiple\n";
  commands_t::add_multiple(temp_dir1, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  commands_t::add_multiple(temp_dir1, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  rm_hashdb_dir(temp_dir4);

  // add_repository
  std::cout << "add_repository\n";
  commands_t::add_repository(temp_dir1, temp_dir4, "invalid_repository");
  TEST_EQ(map_size(temp_dir4), 0);
  commands_t::add_repository(temp_dir1, temp_dir4, "repository1");
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add_repository(temp_dir1, temp_dir4, "repository1");
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add_repository(temp_dir2, temp_dir4, "repository1");
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add_repository(temp_dir2, temp_dir4, "repository2");
  TEST_EQ(map_size(temp_dir4), 2*74);
  rm_hashdb_dir(temp_dir4);

  // intersect
  std::cout << "intersect\n";
  commands_t::intersect(temp_dir1, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 0);
  commands_t::intersect(temp_dir1, temp_dir3, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  rm_hashdb_dir(temp_dir4);

  // intersect hash
  std::cout << "intersect_hash\n";
  commands_t::intersect_hash(temp_dir1, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  commands_t::intersect_hash(temp_dir1, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  rm_hashdb_dir(temp_dir4);

  // subtract
  std::cout << "subtract\n";
  commands_t::subtract(temp_dir3, temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::subtract(temp_dir3, temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add(temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 74);
  commands_t::add(temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  commands_t::subtract(temp_dir3, temp_dir2, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 2*74);
  commands_t::subtract(temp_dir3, temp_dir4, temp_dir5);
  TEST_EQ(map_size(temp_dir5), 0);
  rm_hashdb_dir(temp_dir4);
  rm_hashdb_dir(temp_dir5);

  // subtract_hash
  std::cout << "subtract_hash\n";
  commands_t::subtract_hash(temp_dir3, temp_dir1, temp_dir4);
  TEST_EQ(map_size(temp_dir4), 0);
  rm_hashdb_dir(temp_dir4);

  // deduplicate
  std::cout << "deduplicate1\n";
  commands_t::create(settings, temp_dir6);
  commands_t::import(temp_dir6, sample_dfxml4096, "repository1");
  commands_t::deduplicate(temp_dir6, temp_dir7);
  TEST_EQ(map_size(temp_dir7), 74);
  std::cout << "deduplicate2\n";
  commands_t::import(temp_dir6, sample_dfxml4096, "repository2");
  commands_t::deduplicate(temp_dir6, temp_dir8);
  // check ability to open existing target
  commands_t::deduplicate(temp_dir6, temp_dir8);
  TEST_EQ(map_size(temp_dir8), 0);
}

// scan services
void test_scan_services() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t::create(settings, temp_dir1);
  commands_t::import(temp_dir1, sample_dfxml4096, "repository1");

  // scan
  commands_t::scan(temp_dir1, sample_dfxml4096);
  hash_t k1;
  to_key(1, k1);
  commands_t::scan_hash(temp_dir1, k1.hexdigest());
}

// statistics
void test_statistics() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t::create(settings, temp_dir1);
  commands_t::import(temp_dir1, sample_dfxml4096, "repository1");

  // size
  commands_t::size(temp_dir1);

  // sources
  commands_t::sources(temp_dir1);

  // histogram
  commands_t::histogram(temp_dir1);

  // duplicates
  commands_t::duplicates(temp_dir1, "2");

  // hash_table
  commands_t::hash_table(temp_dir1, "1");
// invalid  commands_t::hash_table(temp_dir1, "2");
}

// tuning
void test_tuning() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t::create(settings, temp_dir1);
  commands_t::import(temp_dir1, sample_dfxml4096, "repository1");

  // rebuild_bloom
  settings.bloom_M_hash_size = 20;
  commands_t::rebuild_bloom(settings, temp_dir1);
}

// performance analysis
void test_performance_analysis() {

  // setup
  hashdb_settings_t settings;
  rm_hashdb_dir(temp_dir1);
  commands_t::create(settings, temp_dir1);

  // add_random
  // not tested because it requires user input "q" to complete.
  // commands_t::add_random("repo_random", temp_dir1, "100000");

  // scan_random
  // not tested because it takes time to run
  // commands_t::scan_random(temp_dir1);
}

// validate use of multiple repository names
void test_multiple_repository_names() {
  
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // create new hashdb
  hashdb_settings_t settings;
  commands_t::create(settings, temp_dir);

  // test ability to manage multiple repository names
  for (int i=0; i<12; i++) {
    // generate unique repository name
    std::ostringstream ss;
    ss << "test_repository_name_" << i;

    // import
    commands_t::import(temp_dir, sample_dfxml4096, ss.str());
  }

  // duplicate import should not add elements
  commands_t::import(temp_dir, sample_dfxml4096, "test_repository_name_0");

  // validate correct size
  TEST_EQ(map_size(temp_dir), 74*12);
}

// validate block size control for 4096
void test_block_size_4096() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);
  rm_hashdb_dir(temp_dir2);
  remove(temp_dfxml_file);

  // create new hashdb
  hashdb_settings_t settings;
  commands_t::create(settings, temp_dir1);
  commands_t::create(settings, temp_dir2);

  // import
  commands_t::import(temp_dir1, sample_dfxml4096, "test_repository_name");
  TEST_EQ(map_size(temp_dir1), 74);

  // export
  commands_t::do_export(temp_dir1, "temp_dfxml_out.xml");

  // import
  commands_t::import(temp_dir2, "temp_dfxml_out.xml", "test_repository_name");
  TEST_EQ(map_size(temp_dir2), 74);
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
  commands_t::create(settings, temp_dir1);
  commands_t::create(settings, temp_dir2);

  // import
  commands_t::import(temp_dir1, sample_dfxml512, "test_repository_name");
  commands_t::import(temp_dir1, sample_dfxml512, "test_repository_name2");
  TEST_EQ(map_size(temp_dir1), 24);

  // export
  commands_t::do_export(temp_dir1, "temp_dfxml_out.xml");

  // import
  commands_t::import(temp_dir2, "temp_dfxml_out.xml", "test_repository_name");
  TEST_EQ(map_size(temp_dir1), 24);
}

// test check for valid hash block size
void test_block_size_0() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir1);

  // create new hashdb
  hashdb_settings_t settings;
  settings.hash_block_size = 0;
  commands_t::create(settings, temp_dir1);

  // import
  commands_t::import(temp_dir1, sample_dfxml4096, "test_repository_name");

  // with hash_block_size=0, total should be every hash, including remainder
  TEST_EQ(map_size(temp_dir1), 75);
}
*/

int main(int argc, char* argv[]) {
/*
  std::cout << "test new database\n";
  std::cout << "  test default block size 4096\n";
  test_block_size_4096();
  std::cout << "  test block size 512 and maximum duplicates count 1\n";
  test_block_size_512_and_count_1();
  std::cout << "  test use of block size 0\n";
  test_block_size_0();
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
*/
  return 0;
}

