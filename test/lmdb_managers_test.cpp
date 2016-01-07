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
#include "lmdb_source_name_manager.hpp"
#include "lmdb_helper.h"
#include "hashdb_settings.hpp"
#include "hashdb_changes.hpp"
#include "../src_libhashdb/hashdb.hpp"
#include "../src/hex_helper.hpp"
#include "directory_helper.hpp"

typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

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

  // find when empty
  TEST_EQ(manager.find(binary_0), false);

  // add
  TEST_EQ(manager.insert(binary_0), true);
  TEST_EQ(manager.find(binary_0), true);

  // re-add
  TEST_EQ(manager.insert(binary_0), false);
  TEST_EQ(manager.find(binary_0), true);

  // check prefix-suffix split
  TEST_EQ(manager.find(binary_1), false);

  // add more
  TEST_EQ(manager.insert(binary_1), true);
  TEST_EQ(manager.insert(binary_12), true);
  TEST_EQ(manager.insert(binary_13), true);
  TEST_EQ(manager.insert(binary_14), true);
  TEST_EQ(manager.insert(binary_14), false);
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
  std::string non_probative_label;
  uint64_t entropy;
  std::string block_label;
  id_offset_pairs_t id_offset_pairs;

  // write default settings
  hashdb_settings_t settings(3, 512, 512);
  hashdb_settings_store::write_settings(p_hashdb_dir, settings);

  // create new manager
  lmdb_hash_data_manager_t manager(hashdb_dir, RW_NEW);

  // this should assert
  manager.find(binary_0, "npl", 1, "bl", id_offset_pairs);

  // insert
  TEST_EQ(manager.insert_hash_data(binary_0, "npl", 1, "bl"), true);
  manager.find(binary_0, non_probative_label, entropy, block_label,
               id_offset_pairs);
  TEST_EQ(non_probative_label, "npl");
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");

  // change
  TEST_EQ(manager.insert_hash_data(binary_0, "npl2", 2, "bl2"), false);
  manager.find(binary_0, non_probative_label, entropy, block_label,
               id_offset_pairs);
  TEST_EQ(non_probative_label, "npl2");
  TEST_EQ(entropy, 2);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(id_offset_pairs.size(), 0);

  // these should assert
  manager.insert_hash_source(binary_0, 1, 513);
  manager.insert_hash_source(binary_1, 1, 512);

  // add sources
  manager.insert_hash_source(binary_0, 1, 512);
  manager.insert_hash_source(binary_0, 1, 1024);
  
  manager.find(binary_0, non_probative_label, entropy, block_label,
               id_offset_pairs);
  TEST_EQ(non_probative_label, "npl2");
  TEST_EQ(entropy, 2);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(id_offset_pairs.size(), 2);
  TEST_EQ(id_offset_pairs[0].first, 1);
  TEST_EQ(id_offset_pairs[0].second, 512);
  TEST_EQ(id_offset_pairs[1].first, 1);
  TEST_EQ(id_offset_pairs[1].second, 1024);

  // check iterator
  TEST_EQ(manager.insert_hash_data(binary_1, "npl3", 3, "bl3"), true);
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
}

// ************************************************************
// lmdb_source_id_manager
// ************************************************************
void lmdb_source_id_manager() {
  std::pair<bool, uint64_t> pair;

  // create new manager
  lmdb_source_id_manager_t manager(hashdb_dir, RW_NEW);

  pair = manager.find(binary_0);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, "")

  pair = manager.insert(binary_0);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1)

  pair = manager.insert(binary_0);
  TEST_EQ(pair.first, false);
  TEST_EQ(pair.second, 1)

  pair = manager.find(binary_0);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1)
}


/*
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

  pair = manager.insert_start(binary_aa);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);

  pair = manager.insert_start(binary_aa);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);

  pair = manager.insert_start(binary_bb);
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 2);

  manager.insert_stop(binary_aa, 10, 11, 12);
  manager.insert_stop(binary_aa, 10, 11, 12);
  manager.insert_stop(binary_bb, 20, 21, 22);

  hashdb::source_metadata_t data(0,0,0);
  data = manager.find(binary_bb);
  TEST_EQ(data.source_id, 20);
  TEST_EQ(data.filesize, 21);
  TEST_EQ(data.positive_count, 22);

  std::pair<std::string, hashdb::source_metadata_t> pair2("",
                                           hashdb::source_metadata_t(0,0,0));
  pair2 = manager.find_begin();
  TEST_EQ(pair2.first, binary_aa);
  TEST_EQ(pair2.second.source_id, 10);
  TEST_EQ(pair2.second.filesize, 11);
  TEST_EQ(pair2.second.positive_count, 12);
  pair2 = manager.find_next(pair2.first);
  TEST_EQ(pair2.first, binary_bb);
  TEST_EQ(pair2.second.source_id, 20);
  TEST_EQ(pair2.second.filesize, 21);
  TEST_EQ(pair2.second.positive_count, 22);
  pair2 = manager.find_next(pair2.first);
  TEST_EQ(pair2.first, "");
  TEST_EQ(pair2.second.source_id, 0);
  TEST_EQ(pair2.second.filesize, 0);
  TEST_EQ(pair2.second.positive_count, 0);

//  manager.insert_stop(binary_cc,0,0,0); // assert not there yet
//  data = manager.find(binary_cc); // assert not found

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_source_name_manager
// ************************************************************
void lmdb_source_name_manager_test() {

  hashdb::source_names_t names;

  // create new manager
  lmdb_source_name_manager_t manager(hashdb_dir, RW_NEW);
  TEST_EQ(manager.size(), 0);

  manager.find(binary_aa, names);
  TEST_EQ(names.size(), 0);

  manager.insert(binary_aa, "ra1", "fa1");
  manager.insert(binary_aa, "ra2", "fa2");
  manager.insert(binary_bb, "rb", "fb");
  manager.insert(binary_bb, "rb", "fb");

  manager.find(binary_aa, names);
  TEST_EQ(names.size(), 2);
  TEST_EQ(names[0].first, "ra1");
  TEST_EQ(names[0].second, "fa1");
  TEST_EQ(names[1].first, "ra2");
  TEST_EQ(names[1].second, "fa2");

  manager.find(binary_bb, names);
  TEST_EQ(names.size(), 1);
  TEST_EQ(names[0].first, "rb");
  TEST_EQ(names[0].second, "fb");
}
*/

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
/*
  // lmdb_hash_label_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_hash_label_manager_test();

  // lmdb_source_id_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_source_id_manager_test();

  // lmdb_source_metadata_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_source_metadata_manager_test();

  // lmdb_source_name_manager
  make_new_hashdb_dir(hashdb_dir);
  lmdb_source_name_manager_test();
*/

  // done
  std::cout << "lmdb_managers_test Done.\n";
  return 0;
}

