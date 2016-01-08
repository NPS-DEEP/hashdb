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
static const std::string file_cc(hex_to_binary_hash("cc"));
static const std::string file_dd(hex_to_binary_hash("dd"));
static const std::string block_ee(hex_to_binary_hash("ee"));
static const std::string block_ff(hex_to_binary_hash("ff"));
//static const std::string binary_big(hex_to_binary_hash("0123456789abcdef2123456789abcdef"));
// ************************************************************
// helper
// ************************************************************
std::pair<bool, std::string> create_default_hashdb(
                          const std::string& p_hashdb_dir,
                          const std::string& command_string) {
    return hashdb::create_hashdb(p_hashdb_dir, 512, 512, command_string);
}

// ************************************************************
// misc interfaces
// ************************************************************
void test_is_valid_hashdb() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // no db
  std::pair<bool, std::string> pair;
  pair = hashdb::is_valid_hashdb(hashdb_dir);
  TEST_EQ(pair.first, false);

  // valid db
  pair = create_default_hashdb(hashdb_dir, "test");
  pair = hashdb::is_valid_hashdb(hashdb_dir);
  TEST_EQ(pair.first, true);
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

/*
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
  manager.import_source_data(binary_0, 100, data);
  std::cout << manager.size() << "\n";
  manager.import_source_data(binary_0, 100, data);
  std::cout << manager.size() << "\n";

  // force fail
  //manager.import_source_data(binary_aa, 100, data);
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
  whitelist_manager.import_source_data(binary_0, 100, data);
  std::cout << "whitelist " << whitelist_manager.size() << "\n";

  // DB in hashdb_dir2
  // nothing should be imported
  pair = create_default_hashdb(hashdb_dir2, "create import DB");
  hashdb::import_manager_t manager(hashdb_dir2, hashdb_dir, true, "import");
  manager.import_source_name(binary_0, "rn", "fn");
  manager.import_source_data(binary_0, 100, data);
  std::cout << "db " << manager.size() << "\n";
}

// ************************************************************
// hashdb_scan_manager
// ************************************************************
void scan_setup() {

  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // create new hashdb directory
  std::pair<bool, std::string> pair;
  pair = create_default_hashdb(hashdb_dir, "test_scan_manager.a");
  TEST_EQ(pair.first, true);
  hashdb::import_manager_t manager(hashdb_dir, "", false, "import");

  // import source file_cc with two attribution names and a useless duplicate
  // and source file_dd with one attribution name
  manager.import_source_name(file_cc, "rn1", "fn1");
  manager.import_source_name(file_cc, "rn2", "fn2");
  manager.import_source_name(file_cc, "rn2", "fn2"); // useless duplicate
  manager.import_source_name(file_dd, "rn3", "fn3");

  // import hash data in duplicate into two sources
  hashdb::hash_data_list_t data;
  data.push_back(hashdb::hash_data_t(block_ee, 0, ""));
  data.push_back(hashdb::hash_data_t(block_ee, 512, ""));
  data.push_back(hashdb::hash_data_t(block_ff, 1024, "LABEL"));
  manager.import_source_data(file_cc, 100, data);
  manager.import_source_data(file_dd, 200, data);
}

void scan_test() {
  hashdb::scan_manager_t manager(hashdb_dir);

  // find_id_offset_pairs, no match
  hashdb::id_offset_pairs_t id_offset_pairs;
  manager.find_id_offset_pairs(file_cc, id_offset_pairs); // not a block hash
  TEST_EQ(id_offset_pairs.size(), 0);

  // find_id_offset_pairs, match
  manager.find_id_offset_pairs(block_ee, id_offset_pairs); // block hash
  TEST_EQ(id_offset_pairs.size(), 4);
  TEST_EQ(id_offset_pairs[0].first, 1);
  TEST_EQ(id_offset_pairs[0].second, 0);
  TEST_EQ(id_offset_pairs[1].first, 1);
  TEST_EQ(id_offset_pairs[1].second, 512);
  TEST_EQ(id_offset_pairs[2].first, 2);
  TEST_EQ(id_offset_pairs[2].second, 0);
  TEST_EQ(id_offset_pairs[3].first, 2);
  TEST_EQ(id_offset_pairs[3].second, 512);

  // find_source_names, no match
  hashdb::source_names_t source_names;
  manager.find_source_names(block_ee, source_names); // not a file hash
  TEST_EQ(source_names.size(), 0);

  // find_source_names, match file_cc
  manager.find_source_names(file_cc, source_names); // file hash
  TEST_EQ(source_names.size(), 2);
  TEST_EQ(source_names[0].first, "rn1");
  TEST_EQ(source_names[0].second, "fn1");
  TEST_EQ(source_names[1].first, "rn2");
  TEST_EQ(source_names[1].second, "fn2");

  // find_source_names, match file_dd
  manager.find_source_names(file_dd, source_names); // file hash
  TEST_EQ(source_names.size(), 1);
  TEST_EQ(source_names[0].first, "rn3");
  TEST_EQ(source_names[0].second, "fn3");

  // find_file_binary_hash
  std::string file_binary_hash;
  file_binary_hash = manager.find_file_binary_hash(1);
  TEST_EQ(file_binary_hash, file_cc)
  file_binary_hash = manager.find_file_binary_hash(2);
  TEST_EQ(file_binary_hash, file_dd)
  //file_binary_hash = manager.find_file_binary_hash(0); // fail invalid value

  // hash iterator begin
  std::string binary_hash;
  binary_hash = manager.hash_begin(id_offset_pairs);
  TEST_EQ(binary_hash, block_ee)
  TEST_EQ(id_offset_pairs.size(), 4);
  TEST_EQ(id_offset_pairs[0].first, 1);
  TEST_EQ(id_offset_pairs[0].second, 0);
  TEST_EQ(id_offset_pairs[1].first, 1);
  TEST_EQ(id_offset_pairs[1].second, 512);
  TEST_EQ(id_offset_pairs[2].first, 2);
  TEST_EQ(id_offset_pairs[2].second, 0);
  TEST_EQ(id_offset_pairs[3].first, 2);
  TEST_EQ(id_offset_pairs[3].second, 512);

  // hash iterator next
  binary_hash = manager.hash_next(binary_hash, id_offset_pairs);
  TEST_EQ(binary_hash, block_ff)
  TEST_EQ(id_offset_pairs.size(), 2);
  TEST_EQ(id_offset_pairs[0].first, 1);
  TEST_EQ(id_offset_pairs[0].second, 1024);
  TEST_EQ(id_offset_pairs[1].first, 2);
  TEST_EQ(id_offset_pairs[1].second, 1024);

  // hash iterator end
  binary_hash = manager.hash_next(binary_hash, id_offset_pairs);
  TEST_EQ(binary_hash, "")
  TEST_EQ(id_offset_pairs.size(), 0);

  // source iterator begin
  std::pair<std::string, hashdb::source_metadata_t> pair;
  pair = manager.source_begin();
  TEST_EQ(pair.first, file_cc);
  TEST_EQ(pair.second.source_id, 1);
  TEST_EQ(pair.second.filesize, 100);
  TEST_EQ(pair.second.positive_count, 2);

  // source iterator next
  pair = manager.source_next(pair.first);
  TEST_EQ(pair.first, file_dd);
  TEST_EQ(pair.second.source_id, 2);
  TEST_EQ(pair.second.filesize, 200);
  TEST_EQ(pair.second.positive_count, 2);
}
*/

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // hashdb_create_manager
  test_create_manager();

/*
  // import, no whitelist, skip low entropy
  test_import_manager();

  // import, do not skip low entropy, no whitelist
  test_import_manager2();

  // valid hashdb check
  test_is_valid_hashdb();

  // scan
  scan_setup();
  scan_test();
*/

  // done
  std::cout << "hashdb_managers_test Done.\n";
  return 0;
}

