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
#include "lmdb_rw_new.hpp"
#include "lmdb_rw_manager.hpp"
#include "lmdb_ro_manager.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_helper.h"
#include "directory_helper.hpp"
#include "hashdb_settings.hpp"

//static const char hashdb_dir[] = "temp_dir_db_managers_test.hdb";
static const std::string hashdb_dir = "temp_dir_db_managers_test.hdb";
static const std::string hashdb_dir2 = "temp_dir_db_managers_test2.hdb";
static const std::string binary_0(lmdb_helper::hex_to_binary_hash("00"));
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
//static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

static const lmdb_source_data_t source_data1("r2", "fn3", 4, "hash5");
static const lmdb_source_data_t source_data2("rn", "fn", 20, "yy");
static const lmdb_source_data_t source_data3("rn3", "fn3", 0, "");
static const lmdb_source_data_t source_data3b("rn3", "fn3", 3, "h3");
static const lmdb_source_data_t source_data0("r", "f", 1, "");
 
 
void create_db() {

  // use specific settings
  hashdb_settings_t settings;
  settings.maximum_hash_duplicates = 2;
  settings.bloom1_is_used = false;

  // create the DB
  lmdb_rw_new::create(hashdb_dir, settings);
}

void test_change() {
  // insert
  lmdb_rw_manager_t manager(hashdb_dir);
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
  TEST_EQ(manager.changes.hashes_removed, 2);
  TEST_EQ(manager.size(), 2);
  manager.remove_hash(binary_bb);
  TEST_EQ(manager.changes.hashes_not_removed_no_hash, 1);
  manager.insert(binary_bb, source_data1, 4096*2);
  manager.insert(binary_bb, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_inserted, 6);
  TEST_EQ(manager.size(), 4);
  manager.remove(binary_bb, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_removed, 3);
  TEST_EQ(manager.size(), 3);
  manager.remove(binary_bb, source_data1, 4096*2);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 1);
  TEST_EQ(manager.size(), 3);
  manager.remove(binary_bb, source_data1, 4096*3);
  TEST_EQ(manager.changes.hashes_removed, 4);
  TEST_EQ(manager.size(), 2);

  // add source data
  manager.add_source_data(source_data3);
  manager.add_source_data(source_data3b);
  manager.add_source_data(source_data3);

  // add single hash
  manager.insert(binary_cc, source_data1, 4096*4);
}

void test_reader() {
  // read
  lmdb_ro_manager_t manager(hashdb_dir);
  TEST_EQ(manager.size(), 3);
  TEST_EQ(manager.find_count(binary_aa), 2);
  TEST_EQ(manager.find_count(binary_bb), 0);
  TEST_EQ(manager.find_count(binary_cc), 1);
  TEST_EQ(manager.find_count(binary_ff), 0);
  lmdb_hash_it_data_t hash_it;
  lmdb_hash_it_data_t hash_it2;
  hash_it = manager.find_first(binary_aa);
  hash_it2 = manager.find_begin();
  TEST_EQ(hash_it, hash_it2);
  hash_it = manager.find_next(hash_it);
  TEST_NE(hash_it, hash_it2);
  hash_it = manager.find_next(hash_it);
  hash_it2 = manager.find_first(binary_cc);
  TEST_EQ(hash_it, hash_it2);
  TEST_EQ(hash_it.is_valid, true);
  TEST_EQ(hash_it.binary_hash, binary_cc);
  lmdb_source_data_t source_data;
  source_data = manager.find_source(hash_it.source_lookup_index);
  TEST_EQ(source_data.repository_name, "r2");
  TEST_EQ(manager.find_exact(binary_aa, source_data1, 4096*1), true)
  TEST_EQ(manager.find_exact(binary_aa, source_data1, 4096*0), false)
  hash_it = manager.find_next(hash_it);
  TEST_EQ(hash_it.is_valid, false);
  TEST_EQ(hash_it.binary_hash, "");
}

void create_db2() {

  // use specific settings
  hashdb_settings_t settings;
  settings.hash_truncation = 1;

  // create the DB
  lmdb_rw_new::create(hashdb_dir2, settings);
}

void test_zero() {
  lmdb_rw_manager_t rw_manager(hashdb_dir2);
  rw_manager.insert(binary_0, source_data0, 4096*0);
  rw_manager.insert(binary_0, source_data0, 4096*1);
  rw_manager.insert(binary_0, source_data0, 4096*2);
  rw_manager.insert(binary_0, source_data0, 4096*3);
  lmdb_ro_manager_t ro_manager(hashdb_dir2);
  TEST_EQ(ro_manager.find_count(binary_0), 4);
}

int main(int argc, char* argv[]) {

  rm_hashdb_dir(hashdb_dir);
  rm_hashdb_dir(hashdb_dir2);
  create_db();
  test_change();
  test_reader();
  create_db2();
  test_zero();
  std::cout << "db_managers_test Done.\n";
  return 0;
}

