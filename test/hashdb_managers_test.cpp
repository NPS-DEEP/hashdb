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
 * Test the hashdb managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "directory_helper.hpp"
#include "hex_helper.hpp"
#include "../src_libhashdb/hashdb.hpp"

static const std::string hashdb_dir = "temp_dir_hashdb_managers_test.hdb";
static const std::string hashdb_dir2 = "temp_dir_hashdb_managers_test2.hdb";
static const std::string binary_0(hex_to_binary_hash("00"));
static const std::string binary_aa(hex_to_binary_hash("aa"));
static const std::string binary_bb(hex_to_binary_hash("bb"));
static const std::string binary_cc(hex_to_binary_hash("cc"));
static const std::string binary_ff(hex_to_binary_hash("ff"));
//static const std::string binary_big(hex_to_binary_hash("0123456789abcdef2123456789abcdef"));
// ************************************************************
// helper
// ************************************************************
std::pair<bool, std::string> create_default_hashdb(
                          const std::string& p_hashdb_dir,
                          const std::string& command_string) {
    return hashdb::create_hashdb(p_hashdb_dir,
                                 512, 512, true, 28, 3,
                                 command_string);
}

// ************************************************************
// hashdb_create_manager
// ************************************************************
void test_create_manager() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // create the hashdb directory
  std::pair<bool, std::string> pair;
  pair = create_default_hashdb(hashdb_dir, "test_create_manager.a");
  TEST_EQ(pair.first, true);
  pair = create_default_hashdb(hashdb_dir, "test_create_manager.b");
  TEST_EQ(pair.first, false);
}

// ************************************************************
// hashdb_import_manager
// ************************************************************
// no whitelist, skip low entropy
void test_import_manager() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // create new hashdb directory
  std::pair<bool, std::string> pair;
  pair = create_default_hashdb(hashdb_dir, "test_import_manager.a");
  TEST_EQ(pair.first, true);

  hashdb::import_manager_t manager(hashdb_dir, "", true,
                                   "test_import_manager.b");

  // import data
  // LABEL should not be imported.
  // Note: this is a coarse functional test.  Inspect the history log
  // if desired.  See Python tests for fuller testing.
  hashdb::hash_data_list_t data;
  data.push_back(hashdb::hash_data_t(binary_aa, 0, ""));
  data.push_back(hashdb::hash_data_t(binary_aa, 512, ""));
  data.push_back(hashdb::hash_data_t(binary_bb, 1024, "LABEL"));
  manager.import_source_name(binary_0, "repository0", "file0");
  manager.import_source_hashes(binary_0, 100, data);
  std::cout << manager.size() << "\n";
  manager.import_source_hashes(binary_0, 100, data);
  std::cout << manager.size() << "\n";

  // force fail
  //manager.import_source_hashes(binary_aa, 100, data);
}

// whitelist, import low entropy
void test_import_manager2() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);
  rm_hashdb_dir(hashdb_dir2);

  // data
  hashdb::hash_data_list_t data;
  data.push_back(hashdb::hash_data_t(binary_aa, 0, ""));
  data.push_back(hashdb::hash_data_t(binary_aa, 512, ""));
  data.push_back(hashdb::hash_data_t(binary_bb, 1024, "LABEL"));

  // whitelist DB in hashdb_dir
  // LABEL should be imported
  std::pair<bool, std::string> pair;
  pair = create_default_hashdb(hashdb_dir, "create whitelist DB");
  hashdb::import_manager_t whitelist_manager(hashdb_dir, "", false,
                                             "import whitelist DB");
  whitelist_manager.import_source_name(binary_0, "w_repository0", "w_file0");
  whitelist_manager.import_source_hashes(binary_0, 100, data);
  std::cout << "whitelist " << whitelist_manager.size() << "\n";

  // DB in hashdb_dir2
  // nothing should be imported
  pair = create_default_hashdb(hashdb_dir2, "create import DB");
  hashdb::import_manager_t manager(hashdb_dir2, hashdb_dir, true, "import");
  manager.import_source_name(binary_0, "rn", "fn");
  manager.import_source_hashes(binary_0, 100, data);
  std::cout << "db " << manager.size() << "\n";
}

void bloom_setup() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);
  rm_hashdb_dir(hashdb_dir2);

  // data
  hashdb::hash_data_list_t data;
  data.push_back(hashdb::hash_data_t(binary_aa, 0, ""));
  data.push_back(hashdb::hash_data_t(binary_aa, 512, ""));
  data.push_back(hashdb::hash_data_t(binary_bb, 1024, "LABEL"));

  // add data
  std::pair<bool, std::string> pair;
  pair = create_default_hashdb(hashdb_dir, "create DB");
  hashdb::import_manager_t manager(hashdb_dir, "", false, "import");
  manager.import_source_name(binary_0, "rn", "fn");
  manager.import_source_hashes(binary_0, 100, data);
}

void bloom_test() {
  // rebuild Bloom on non-existent hashdb
  std::pair<bool, std::string> pair;

  // rebuild Bloom, off
  pair = hashdb::rebuild_bloom(hashdb_dir, false, 2, 20, "rebuild_1");
  TEST_EQ(pair.first, true);

  // rebuild Bloom, on
  pair = hashdb::rebuild_bloom(hashdb_dir, false, 2, 20, "rebuild_2");
  TEST_EQ(pair.first, true);

  // non-existent DB fails
  //pair = hashdb::rebuild_bloom(hashdb_dir2, false, 2, 20, "rebuild_3");
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // hashdb_create_manager
  test_create_manager();

  // import, no whitelist, skip low entropy
  test_import_manager();

  // import, do not skip low entropy, no whitelist
  test_import_manager2();

  // Bloom filter
  bloom_setup();
  bloom_test();

  // done
  std::cout << "hashdb_managers_test Done.\n";
  return 0;
}

