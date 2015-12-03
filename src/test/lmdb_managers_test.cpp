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
#include "lmdb_hash_manager.hpp"
#include "lmdb_hash_label_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_metadata_manager.hpp"
#include "lmdb_helper.h"
#include "hashdb_settings.hpp"
#include "hashdb_changes.hpp"
#include "file_helper.hpp"
#include "directory_helper.hpp"

static const std::string hashdb_dir = "temp_dir_lmdb_managers_test.hdb";
static const std::string binary_0(lmdb_helper::hex_to_binary_hash("00"));
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
//static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

void make_new_hashdb_dir(std::string p_hashdb_dir) {
  // remove any previous hashdb_dir
  rm_hashdb_dir(p_hashdb_dir);

  // create the hashdb directory
  file_helper::require_no_dir(p_hashdb_dir);
  file_helper::create_new_dir(p_hashdb_dir);

  // write default settings
  hashdb_settings_t settings;
  hashdb_settings_store_t::write_settings(p_hashdb_dir, settings);
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
  hashdb_changes_t changes;
  id_offset_pairs_t pairs;

  // make sure iterator works on an empty DB
  manager.find_begin(pairs);
  TEST_EQ(pairs.size(), 0);


  // list to add
  hash_data_list_t list;

  // two entries with one duplicate element
  list.push_back(hash_data_t(binary_aa, 512, ""));
  list.push_back(hash_data_t(binary_aa, 512, ""));
  list.push_back(hash_data_t(binary_aa, 1024, ""));
  list.push_back(hash_data_t(binary_bb, 2048, ""));
  manager.insert(1, list, changes);
  TEST_EQ(manager.size(), 3);
  TEST_EQ(changes.hashes_not_inserted_duplicate_element, 1)

  // hashes_not_inserted_invalid_sector_size
  list.push_back(hash_data_t(binary_bb, 511, ""));
  TEST_EQ(changes.hashes_not_inserted_invalid_sector_size, 0)
  manager.insert(1, list, changes);
  TEST_EQ(changes.hashes_not_inserted_invalid_sector_size, 1)

  // new source ID
  list.clear();
  list.push_back(hash_data_t(binary_aa, 512, ""));
  manager.insert(2, list, changes);

  // hashes_inserted
  TEST_EQ(changes.hashes_inserted, 4)

  // find
  manager.find(binary_aa, pairs);
  TEST_EQ(pairs.size(), 3);
  TEST_EQ(pairs[0].first, 1)
  TEST_EQ(pairs[0].second, 512)
  TEST_EQ(pairs[1].first, 1)
  TEST_EQ(pairs[1].second, 1024)
  TEST_EQ(pairs[2].first, 2)
  TEST_EQ(pairs[2].second, 512)
  manager.find(binary_bb, pairs);
  TEST_EQ(pairs.size(), 1);
  TEST_EQ(pairs[0].first, 1)
  TEST_EQ(pairs[0].second, 2048)

  // iterator walk
  std::string binary_hash = manager.find_begin(pairs);
  TEST_EQ(pairs.size(), 3);
  binary_hash = manager.find_next(binary_hash, pairs);
  TEST_EQ(pairs.size(), 1);
  binary_hash = manager.find_next(binary_hash, pairs);
  TEST_EQ(pairs.size(), 0);
  TEST_EQ(binary_hash, "");
// force assert: binary_hash = manager.find_next(binary_hash, pairs);
// force assert: binary_hash = manager.find_next(binary_ff, pairs);

  // size
  TEST_EQ(manager.size(), 4);
}

void lmdb_hash_manager_read() {
  lmdb_hash_manager_t manager(hashdb_dir, READ_ONLY);

  // find
  id_offset_pairs_t pairs;
  manager.find(binary_aa, pairs);
  TEST_EQ(pairs.size(), 3);
  manager.find(binary_bb, pairs);
  TEST_EQ(pairs.size(), 1);

  // size
  TEST_EQ(manager.size(), 4);
}

// ************************************************************
// lmdb_hash_label_manager
//
// Note that everything is tested during create rather than from individual
// file mount modes.  That infrastructure is tested only by lmdb_hash_manager.
// ************************************************************
void lmdb_hash_label_manager_test() {

  // create new manager
  lmdb_hash_label_manager_t manager(hashdb_dir, RW_NEW);

  std::string label;
  label = manager.find(binary_aa);
  TEST_EQ(label, "")

  manager.insert(binary_aa, "");
  label = manager.find(binary_aa);
  TEST_EQ(label, "")

  manager.insert(binary_aa, "l1");
  label = manager.find(binary_aa);
  TEST_EQ(label, "l1")

  manager.insert(binary_aa, "l1");
  label = manager.find(binary_aa);
  TEST_EQ(label, "l1")

  manager.insert(binary_aa, "l2");
  label = manager.find(binary_aa);
  TEST_EQ(label, "l1")

  manager.insert(binary_bb, "l2");
  label = manager.find(binary_bb);
  TEST_EQ(label, "l2")

  manager.insert(binary_aa, "l1");
  label = manager.find(binary_aa);
  TEST_EQ(label, "l1")

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_source_id_manager
// ************************************************************
void lmdb_source_id_manager_test() {

  // create new manager
  lmdb_source_id_manager_t manager(hashdb_dir, RW_NEW);

  manager.insert(1, binary_aa);
  manager.insert(2, binary_bb);
//  manager.insert(1, binary_aa); // makes warning to stdout, not checked
  std::string binary_file_hash;
  binary_file_hash = manager.find(1);
  TEST_EQ(binary_file_hash, binary_aa)
  binary_file_hash = manager.find(2);
  TEST_EQ(binary_file_hash, binary_bb)
//  binary_file_hash = manager.find(3); // assert

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_source_metadata_manager
// ************************************************************
void lmdb_source_metadata_manager_test() {

  // create new manager
  lmdb_source_metadata_manager_t manager(hashdb_dir, RW_NEW);

  std::pair<bool, uint64_t> pair;

  pair = manager.insert_begin(binary_aa);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);

  pair = manager.insert_begin(binary_aa);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);

  pair = manager.insert_begin(binary_bb);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 2);

  manager.insert_end(binary_aa, 10, 11, 12);
  manager.insert_end(binary_aa, 10, 11, 12);
  manager.insert_end(binary_bb, 20, 21, 22);

  source_metadata_t data("",0,0,0);
  data = manager.find(binary_bb);
  TEST_EQ(data.file_binary_hash, binary_bb);
  TEST_EQ(data.source_id, 20);
  TEST_EQ(data.filesize, 21);
  TEST_EQ(data.positive_count, 22);

  manager.insert_end(binary_cc,0,0,0); // assert not there yet
//  data = manager.find(binary_cc); // assert not found

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // lmdb_hash_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_hash_manager_create();
  lmdb_hash_manager_write();
  lmdb_hash_manager_read();

  // lmdb_hash_label_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_hash_label_manager_test();

  // lmdb_source_id_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_source_id_manager_test();

  // lmdb_source_metadata_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_source_metadata_manager_test();

  // done
  std::cout << "lmdb_managers_test Done.\n";
  return 0;
}

