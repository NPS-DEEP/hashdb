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
#include "lmdb_hash_manager.hpp"
#include "lmdb_helper.h"
#include "hashdb_settings.hpp"
#include "hashdb_changes.hpp"
#include "file_helper.hpp"
#include "directory_helper.hpp"

//static const char hashdb_dir[] = "temp_dir_db_managers_test.hdb";
static const std::string hashdb_dir = "temp_dir_db_managers_test.hdb";
static const std::string hashdb_dir2 = "temp_dir_db_managers_test2.hdb";
static const std::string binary_0(lmdb_helper::hex_to_binary_hash("00"));
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
//static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

void test_create() {
  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // create the hashdb directory
  file_helper::require_no_dir(hashdb_dir);
  file_helper::create_new_dir(hashdb_dir);

  // write the settings
  hashdb_settings_t settings;
  hashdb_settings_store_t::write_settings(hashdb_dir, settings);

  // create new manager
  lmdb_hash_manager_t manager(hashdb_dir, RW_NEW);
}

void test_write() {
  lmdb_hash_manager_t manager(hashdb_dir, RW_MODIFY);
  hashdb_changes_t changes;

  // list to add
  hash_data_list_t list;

  // two entries with one duplicate element
  list.push_back(hash_data_t(binary_aa, 512, ""));
  list.push_back(hash_data_t(binary_aa, 512, ""));
  list.push_back(hash_data_t(binary_bb, 512, ""));
  manager.insert(1, list, changes);
  TEST_EQ(manager.size(), 2);
  TEST_EQ(changes.hashes_not_inserted_duplicate_element, 1)

  // hashes_not_inserted_invalid_sector_size
  list.push_back(hash_data_t(binary_bb, 511, ""));
  TEST_EQ(changes.hashes_not_inserted_invalid_sector_size, 0)
  manager.insert(1, list, changes);
  TEST_EQ(changes.hashes_not_inserted_invalid_sector_size, 1)

  // hashes_inserted
  TEST_EQ(changes.hashes_inserted, 2)

/*
  // find
  id_offset_pairs_t pairs = find(binary_aa);
  TEST_EQ(pairs.size(), 1)
  id_offset_pairs_t pairs = find(binary_bb);
  TEST_EQ(pairs.size(), 1)
*/

  // size
}

void test_read() {
  lmdb_hash_manager_t manager(hashdb_dir, READ_ONLY);

/*
  // find
  id_offset_pairs_t pairs = find(binary_aa);
  TEST_EQ(pairs.size(), 1)
  id_offset_pairs_t pairs = find(binary_bb);
  TEST_EQ(pairs.size(), 1)
*/

  // size
  TEST_EQ(manager.size(), 2)
}

int main(int argc, char* argv[]) {

  rm_hashdb_dir(hashdb_dir);
  rm_hashdb_dir(hashdb_dir2);
  test_create();
  test_write();
  test_read();
  std::cout << "db_managers_test Done.\n";
  return 0;
}

