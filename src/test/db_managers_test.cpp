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
 * Test the db managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "lmdb_change_manager.hpp"
#include "lmdb_read_manager.hpp"
#include "lmdb_hash_it_data.hpp"
#include "directory_helper.hpp"
#include "hashdb_directory_manager.hpp"
//#include "file_modes.h"

//zz// file modes:
//zz// READ_ONLY, RW_NEW, RW_MODIFY

//static const char temp_dir[] = "temp_dir_db_test.hdb";
static const std::string temp_dir = "temp_dir_db_test.hdb";
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

static const lmdb_source_data_t source_data1("r2", "fn3", "fsz4", "hash5");
static const lmdb_source_data_t source_data2("rn", "fn", "20", "yy");
static const lmdb_source_data_t source_data3("rn3", "fn3", "", "");
static const lmdb_source_data_t source_data3b("rn3", "fn3", "sz3", "h3");
 
 
void create_db() {
  // create the hashdb directory
  hashdb_directory_manager_t::create_new_hashdb_dir(hashdb_dir);

  // write specific settings
  settings_t settings;
  settings.maximum_hash_duplicates = 2;
  settings.bloom1_is_used = false;
  hashdb_settings_store_t::write_settings(hashdb_dir, settings_t());

  // create the new stores
  lmdb_hash_store_t(hashdb_dir, RW_NEW);
  lmdb_name_store_t(hashdb_dir, RW_NEW);
  lmdb_source_store_t(hashdb_dir, RW_NEW);
}

void test_change() {
  // insert
  lmdb_change_manager_t manager("temp_db_managers_test.hdb");
  manager.insert(binary_aa, source_data1, 4096*1);
  TEST_EQ(manager.changes.hashes_inserted, 1);
  manager.insert(binary_aa, source_data1, 4095);
  TEST_EQ(manager.changes.hashes_not_inserted_invalid_byte_alignment, 1);
  manager.insert(binary_aa, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_inserted, 2);
  manager.insert(binary_aa, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_not_inserted_exceeds_max_duplicates, 1);
  TEST_EQ(manager.changes.hashes_inserted, 2);
  manager.insert(binary_aa, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_not_inserted_duplicate_element, 1);
  TEST_EQ(manager.size(), 2);

  // remove
  TEST_EQ(manager.changes.hashes_inserted, 2);
  manager.insert(binary_bb, source_data1, 4096*2);
  manager.insert(binary_bb, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_inserted, 4);
  TEST_EQ(manager.size(), 4);
  manager.remove_hash(binary_bb);
  TEST_EQ(manager.size(), 2);
  manager.remove_hash(binary_bb);
  TEST_EQ(manager.changes.hashes_not_removed_no_hash, 1);
  manager.insert(binary_bb, source_data1, 4096*2);
  manager.insert(binary_bb, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_inserted, 6);
  TEST_EQ(manager.size(), 4);
  manager.remove(binary_bb, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_removed, 1);
  TEST_EQ(manager.size(), 3);
  manager.remove(binary_bb, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 1);
  TEST_EQ(manager.size(), 3);
  manager.remove(binary_bb, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_removed, 2);
  TEST_EQ(manager.size(), 2);

  // add source data
  manager.add_source_data(data3);
  manager.add_source_data(data3b);
  manager.add_source_data(data3);
}

void test_reader() {
  // TBD
}

int main(int argc, char* argv[]) {

  rm_hashdb_dir(temp_dir);
  create_db();
  test_change();
  test_reader();
  std::cout << "db_managers_test Done.\n";
  return 0;
}

