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
 * Test the LMDB data managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "lmdb_hash_data_manager.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_source_data_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include "lmdb_helper.h"
#include "lmdb_changes.hpp"
#include "hashdb_settings.hpp"
#include "../src_libhashdb/hashdb.hpp"
#include "../src/hex_helper.hpp"
#include "directory_helper.hpp"

typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

typedef std::pair<std::string, std::string> source_name_t;
typedef std::set<source_name_t>             source_names_t;

static const std::string hashdb_dir = "temp_dir_lmdb_managers_test.hdb";
static const std::string binary_0(hex_to_binary_hash(
                                  "00000000000000000000000000000000"));
static const std::string binary_1(hex_to_binary_hash(
                                  "00000000000000000000000000000001"));
static const std::string binary_12(hex_to_binary_hash(
                                  "10000000000000000000000000000002"));
static const std::string binary_13(hex_to_binary_hash(
                                  "10000000000000000000000000000003"));
static const std::string binary_14(hex_to_binary_hash(
                                  "10000000000000000000000000000004"));
static const std::string binary_15(hex_to_binary_hash(
                                  "10000000000000000000000000000005"));
static const std::string binary_26(hex_to_binary_hash(
                                  "20000000000000000000000000000006"));

void make_new_hashdb_dir(std::string p_hashdb_dir) {
  // remove any previous hashdb_dir
  rm_hashdb_dir(p_hashdb_dir);

  // create the hashdb directory
  require_no_dir(p_hashdb_dir);
  create_new_dir(p_hashdb_dir);
}

// ************************************************************
// lmdb_hash_manager
// ************************************************************
void lmdb_hash_manager_create() {

  // create new manager
  lmdb_hash_manager_t manager(hashdb_dir, RW_NEW);
}

void lmdb_hash_manager_write() {
  lmdb_hash_manager_t manager(hashdb_dir, RW_MODIFY);
  lmdb_changes_t changes;

  // find when empty
  TEST_EQ(manager.find(binary_0), false);

  // add
  TEST_EQ(manager.insert(binary_0, changes), true);
  TEST_EQ(changes.hash, 1);
  TEST_EQ(manager.find(binary_0), true);

  // re-add
  TEST_EQ(manager.insert(binary_0, changes), false);
  TEST_EQ(changes.hash_false, 1);
  TEST_EQ(manager.find(binary_0), true);

  // check prefix-suffix split
  TEST_EQ(manager.find(binary_1), false);

  // add more
  TEST_EQ(manager.insert(binary_1, changes), true);
  TEST_EQ(manager.insert(binary_12, changes), true);
  TEST_EQ(manager.insert(binary_13, changes), true);
  TEST_EQ(manager.insert(binary_14, changes), true);
  TEST_EQ(manager.insert(binary_14, changes), false);
  TEST_EQ(manager.find(binary_0), true);
  TEST_EQ(manager.find(binary_1), true);
  TEST_EQ(manager.find(binary_12), true);
  TEST_EQ(manager.find(binary_13), true);
  TEST_EQ(manager.find(binary_14), true);
  TEST_EQ(manager.find(binary_15), false);
  TEST_EQ(manager.find(binary_26), false);

  // size
  TEST_EQ(manager.size(), 2);
}

void lmdb_hash_manager_read() {
  lmdb_hash_manager_t manager(hashdb_dir, READ_ONLY);

  // find
  TEST_EQ(manager.find(binary_0), true);
  TEST_EQ(manager.find(binary_1), true);
  TEST_EQ(manager.find(binary_12), true);
  TEST_EQ(manager.find(binary_13), true);
  TEST_EQ(manager.find(binary_14), true);
  TEST_EQ(manager.find(binary_15), false);
  TEST_EQ(manager.find(binary_26), false);
  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_hash_data_manager
// ************************************************************
void lmdb_hash_data_manager() {

  // variables
  std::pair<bool, std::string> settings_pair;
  std::string low_entropy_label;
  uint64_t entropy;
  std::string block_label;
  id_offset_pairs_t id_offset_pairs;
  lmdb_changes_t changes;

  // write default settings
  hashdb_settings_t settings(3, 512, 512);
  settings_pair = hashdb_settings_store::write_settings(hashdb_dir, settings);
  TEST_EQ(settings_pair.first, true);

  // create new manager
  lmdb_hash_data_manager_t manager(hashdb_dir, RW_NEW);

  // insert
  TEST_EQ(manager.insert_hash_data(binary_0, "lel", 1, "bl", changes), true);
  TEST_EQ(changes.hash_data, 1);
  TEST_EQ(manager.find(binary_0, low_entropy_label, entropy, block_label,
               id_offset_pairs), true);
  TEST_EQ(low_entropy_label, "lel");
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");

  // not there
  TEST_EQ(manager.find(binary_1, low_entropy_label, entropy, block_label,
               id_offset_pairs), false);

  // no change
  TEST_EQ(manager.insert_hash_data(binary_0, "lel2", 2, "bl2", changes), false);
  TEST_EQ(changes.hash_data_false, 1);
  TEST_EQ(changes.hash_data_different, 1);
  TEST_EQ(manager.find(binary_0, low_entropy_label, entropy, block_label,
               id_offset_pairs), true);
  TEST_EQ(low_entropy_label, "lel");
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");
  TEST_EQ(id_offset_pairs.size(), 0);

  // these should assert
  //manager.insert_hash_source(binary_0, 1, 513, changes);
  //manager.insert_hash_source(binary_1, 1, 512, changes);

  // add sources
  manager.insert_hash_source(binary_0, 1, 512, changes);
  TEST_EQ(changes.hash_source, 1);
  manager.insert_hash_source(binary_0, 1, 512, changes);
  TEST_EQ(changes.hash_source_false, 1);
  manager.insert_hash_source(binary_0, 1, 1024, changes);
  
  TEST_EQ(manager.find(binary_0, low_entropy_label, entropy, block_label,
               id_offset_pairs), true);
  TEST_EQ(low_entropy_label, "lel");
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");
  TEST_EQ(id_offset_pairs.size(), 2);
  id_offset_pairs_t::const_iterator it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);
  ++it;
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 1024);

  // check iterator
  TEST_EQ(manager.insert_hash_data(binary_1, "lel3", 3, "bl3", changes), true);
  std::pair<bool, std::string> pair;
  pair = manager.find_begin();
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, binary_0)
  pair = manager.find_next(pair.second);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, binary_1)
  pair = manager.find_next(pair.second);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, "")

  // size
  TEST_EQ(manager.size(), 2);

  // MAX_ID_OFFSET_PAIRS
  // NOTE: to test, change hardcoded value in lmdb_hash_data_manager.hpp to 2
  //TEST_EQ(manager.insert_hash_data(binary_0, "lel", 1, "bl", changes), false);
  //manager.insert_hash_source(binary_0, 1, 4096, changes);
  //TEST_EQ(changes.hash_source_max, 1);
}

// ************************************************************
// lmdb_source_id_manager
// ************************************************************
void lmdb_source_id_manager() {
  std::pair<bool, uint64_t> pair;
  lmdb_changes_t changes;

  // create new manager
  lmdb_source_id_manager_t manager(hashdb_dir, RW_NEW);

  pair = manager.find(binary_0);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, 0)

  pair = manager.insert(binary_0, changes);
  TEST_EQ(changes.source_file_hash, 1);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1)

  pair = manager.insert(binary_0, changes);
  TEST_EQ(changes.source_file_hash_false, 1);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, 1)

  pair = manager.find(binary_0);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1)
}

// ************************************************************
// lmdb_source_data_manager
// ************************************************************
void lmdb_source_data_manager() {
  lmdb_changes_t changes;

  // variables
  std::string file_binary_hash;
  uint64_t filesize;
  std::string file_type;
  uint64_t low_entropy_count;

  // create new manager
  lmdb_source_data_manager_t manager(hashdb_dir, RW_NEW);

  // this should assert
  //manager.find(1, file_binary_hash, filesize, file_type, low_entropy_count);

  // insert
  TEST_EQ(manager.insert(1, "fbh", 2, "ft", 3, changes), true);
  TEST_EQ(changes.source_data, 1);
  manager.find(1, file_binary_hash, filesize, file_type, low_entropy_count);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(low_entropy_count, 3);

  // no change
  TEST_EQ(manager.insert(1, "fbh2", 22, "ft2", 32, changes), false);
  TEST_EQ(changes.source_data_false, 1);
  TEST_EQ(changes.source_data_different, 1);
  manager.find(1, file_binary_hash, filesize, file_type, low_entropy_count);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(low_entropy_count, 3);

  // insert second
  TEST_EQ(manager.insert(0, "", 0, "", 0, changes), true);
  manager.find(0, file_binary_hash, filesize, file_type, low_entropy_count);
  TEST_EQ(file_binary_hash, "");
  TEST_EQ(filesize, 0);
  TEST_EQ(file_type, "");
  TEST_EQ(low_entropy_count, 0);

  // make sure 1 is still in place
  manager.find(1, file_binary_hash, filesize, file_type, low_entropy_count);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(low_entropy_count, 3);

  // iterator
  std::pair<bool, uint64_t> pair;
  pair = manager.find_begin();
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 0);
  pair = manager.find_next(pair.second);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);
  pair = manager.find_next(pair.second);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, 0);

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_source_name_manager
// ************************************************************
void lmdb_source_name_manager() {

  // variables
  std::pair<bool, uint64_t> pair;
  source_names_t source_names;
  lmdb_changes_t changes;

  // create new manager
  lmdb_source_name_manager_t manager(hashdb_dir, RW_NEW);

  // this should assert
  //manager.find(1, source_names);

  // insert, find
  TEST_EQ(manager.insert(1, "rn", "fn", changes), true);
  TEST_EQ(changes.source_name, 1);
  TEST_EQ(manager.insert(1, "rn", "fn", changes), false);
  TEST_EQ(changes.source_name_false, 1);
  TEST_EQ(manager.insert(1, "rn2", "fn2", changes), true);
  manager.find(1, source_names);
  source_names_t::const_iterator it = source_names.begin();
  TEST_EQ(it->first, "rn");
  TEST_EQ(it->second, "fn");
  ++it;
  TEST_EQ(it->first, "rn2");
  TEST_EQ(it->second, "fn2");

  // size
  TEST_EQ(manager.size(), 1);
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  make_new_hashdb_dir(hashdb_dir);

  // lmdb_hash_manager
  lmdb_hash_manager_create();
  lmdb_hash_manager_write();
  lmdb_hash_manager_read();

  // lmdb_hash_data_manager
  lmdb_hash_data_manager();

  // source ID manager
  lmdb_source_id_manager();

  // source data manager
  lmdb_source_data_manager();

  // source name manager
  lmdb_source_name_manager();

  // done
  std::cout << "lmdb_managers_test Done.\n";
  return 0;
}

