// Author:  Bruce Allen
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
#include "source_id_offsets.hpp"
#include "../src_libhashdb/hashdb.hpp"
#include "directory_helper.hpp"

typedef std::pair<std::string, std::string> source_name_t;
typedef std::set<source_name_t>             source_names_t;

static const std::string hashdb_dir = "temp_dir_lmdb_managers_test.hdb";
static const std::string binary_0(hashdb::hex_to_bin(
                                  "00000000000000000000000000000000"));
static const std::string binary_1(hashdb::hex_to_bin(
                                  "00000000000000000000000000000001"));
static const std::string binary_2(hashdb::hex_to_bin(
                                  "00000000000000000000000000000002"));
static const std::string binary_10(hashdb::hex_to_bin(
                                  "10000000000000000000000000000000"));
static const std::string binary_11(hashdb::hex_to_bin(
                                  "10000000000000000000000000000001"));
static const std::string binary_12(hashdb::hex_to_bin(
                                  "10000000000000000000000000000002"));
static const std::string binary_13(hashdb::hex_to_bin(
                                  "10000000000000000000000000000003"));
static const std::string binary_14(hashdb::hex_to_bin(
                                  "10000000000000000000000000000004"));
static const std::string binary_15(hashdb::hex_to_bin(
                                  "10000000000000000000000000000005"));
static const std::string binary_26(hashdb::hex_to_bin(
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
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 28, 3);
}

// run after create
void lmdb_hash_manager_write() {
  hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_MODIFY, 28, 3);
  hashdb::lmdb_changes_t changes;

  // find when empty
  TEST_EQ(manager.find(binary_0), 0);

  // add
  manager.insert(binary_0, 1, changes);
  TEST_EQ(changes.hash_prefix_inserted, 1);
  TEST_EQ(changes.hash_suffix_inserted, 1);
  TEST_EQ(changes.hash_count_changed, 0);
  TEST_EQ(changes.hash_not_changed, 0);
  TEST_EQ(manager.find(binary_0), 1);

  // re-add same
  manager.insert(binary_0, 1, changes);
  TEST_EQ(changes.hash_prefix_inserted, 1);
  TEST_EQ(changes.hash_suffix_inserted, 1);
  TEST_EQ(changes.hash_count_changed, 0);
  TEST_EQ(changes.hash_not_changed, 1);
  TEST_EQ(manager.find(binary_0), 1);

  // change count
  manager.insert(binary_0, 2, changes);
  TEST_EQ(changes.hash_prefix_inserted, 1);
  TEST_EQ(changes.hash_suffix_inserted, 1);
  TEST_EQ(changes.hash_count_changed, 1);
  TEST_EQ(changes.hash_not_changed, 1);
  TEST_EQ(manager.find(binary_0), 2);

  // check prefix-suffix split
  TEST_EQ(manager.find(binary_1), 0);

  // add more
  manager.insert(binary_1, 1, changes);
  manager.insert(binary_12, 1, changes);
  manager.insert(binary_13, 1, changes);
  manager.insert(binary_14, 1, changes);
  TEST_EQ(manager.find(binary_0), 2);
  TEST_EQ(manager.find(binary_1), 1);
  TEST_EQ(manager.find(binary_12), 1);
  TEST_EQ(manager.find(binary_13), 1);
  TEST_EQ(manager.find(binary_14), 1);
  TEST_EQ(manager.find(binary_15), 0);
  TEST_EQ(manager.find(binary_26), 0);

  // size
  TEST_EQ(manager.size(), 2);
}

// run after read
void lmdb_hash_manager_read() {
  hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::READ_ONLY, 28, 3);

  // find
  TEST_EQ(manager.find(binary_0), 2);
  TEST_EQ(manager.find(binary_1), 1);
  TEST_EQ(manager.find(binary_12), 1);
  TEST_EQ(manager.find(binary_13), 1);
  TEST_EQ(manager.find(binary_14), 1);
  TEST_EQ(manager.find(binary_15), 0);
  TEST_EQ(manager.find(binary_26), 0);
  // size
  TEST_EQ(manager.size(), 2);
}

// test with various values for prefix bits and suffix bytes
// Also: enable the DEBUG flag in lmdb_helper.h and observe proper compressed
// encodings.
void lmdb_hash_manager_settings() {
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_changes_t changes;

  {
  } { // 1 prefix bit, no suffix
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 0);
    manager.insert(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);

    TEST_EQ(manager.find(hashdb::hex_to_bin("00000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("7fffffffffffffffffffffffffffffff")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("80000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff")),1);

  } { // demonstrate that the db is cleared
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 0);
    TEST_EQ(manager.size(), 0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("00000000000000000000000000000000")),0);

  } { // 1 prefix bit, no suffix, demonstrate adding 0 instead of 1
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 0);
    manager.insert(hashdb::hex_to_bin("00000000000000000000000000000000"), 1, changes);

    TEST_EQ(manager.find(hashdb::hex_to_bin("00000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("80000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("7fffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("8fffffffffffffffffffffffffffffff")),0);

  } { // 2 prefix bits, no suffix
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 2, 0);

    manager.insert(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("cfffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("c0000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("40000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("80000000000000000000000000000000")),0);

  } { // 1 prefix bit, 1 suffix byte
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 1);

    manager.insert(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);

    TEST_EQ(manager.find(hashdb::hex_to_bin("00000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("800000000000000000000000000000ff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("700000000000000000000000000000ff")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("800000000000000000000000000000fe")),0);

  } { // 9 prefix bits, 2 suffix bytes
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 9, 2);

    manager.insert(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ffff000000000000000000000000ffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ff80000000000000000000000000ffff")),1);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ff00000000000000000000000000ffff")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ff800000000000000000000000007fff")),0);
    TEST_EQ(manager.find(hashdb::hex_to_bin("ff80000000000000000000000000ff7f")),0);
  }
}

// test corner-case values for count
void lmdb_hash_manager_count() {
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 28, 3);
  hashdb::lmdb_changes_t changes;
  manager.insert(binary_0, 1494, changes);
  TEST_EQ(manager.find(binary_0), 1370);
  manager.insert(binary_0, 1495, changes);
  TEST_EQ(manager.find(binary_0), 1495);
}


// ************************************************************
// lmdb_hash_data_manager
// ************************************************************
// test empty and no action conditions
void lmdb_hash_data_manager_empty() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 4, 3);

  // binary_0 not there, all fields reset
  entropy=1.0;
  block_label="bl",
  count=1;
  source_id_offsets.insert(hashdb::source_id_offset_t());
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), false);
  TEST_FLOAT_EQ(entropy, 0.0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_offsets.size(), 0);

  // Attempt to insert an empty key.  A warning is sent to stderr.
  TEST_EQ(manager.insert("", 1.0, "bl", 1, 2, file_offsets, changes), 0);

  // set up file_offsets with an invalid and valid value
  file_offsets.insert(513);
  file_offsets.insert(1024);

  // attempt to insert with invalid sub_count.  A warning is sent to stderr.
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 1, file_offsets, changes), 0);

  // attempt to insert with an invalid file_offset.  warning sent to stderr.
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 2, file_offsets, changes), 0);
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), false);
  TEST_FLOAT_EQ(entropy, 0.0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_offsets.size(), 0);

  // iterator
  TEST_EQ(manager.first_hash(), "");

}

// test Type 1
void lmdb_hash_data_manager_type1() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 4, 3);

  // set up file_offsets
  file_offsets.insert(512*2);

  // test insert_new_type1()
  // Insert offset 512*2 with sub_count=2.
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 2, file_offsets, changes), 2);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.0);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 2);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->source_id, 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 1);
  TEST_EQ(*(source_id_offsets.begin()->file_offsets.begin()), 512*2);

  // add offset 512*1
  file_offsets.insert(512*1); // 512*1 will go in front of 512*2

  // test insert_update_type1()
  // Insert 512*1, re-insert 512*2, use sub_count 4, and change data
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 1, 4, file_offsets, changes), 6);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 6);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->source_id, 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 2);
  TEST_EQ(*(source_id_offsets.begin()->file_offsets.begin()), 512*1);
  TEST_EQ(*(source_id_offsets.begin()->file_offsets.rbegin()), 512*2);

  // Insert 512*3, which will fit, and 512*4, which will not fit max_sub_count=3
  file_offsets.clear();
  file_offsets.insert(512*3);
  file_offsets.insert(512*4);
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 1, 2, file_offsets, changes), 8);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 3);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 8);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 3);
  TEST_EQ(*(source_id_offsets.begin()->file_offsets.rbegin()), 512*3);

  // Insert 512*3, 512*4 again.
  // 512*3 will be detected as present, 512*4 will not.
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 1, 2, file_offsets, changes), 10);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 3);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 10);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 3);
}

// test Type 1 with max_count=0, max_sub_count=1
void lmdb_hash_data_manager_type1_0_1() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 1, 0);

  // set up file_offsets
  file_offsets.insert(1024);

  // Insert offset 1024 with sub_count=2.
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 2, file_offsets, changes), 2);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 0);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.0);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 2);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->source_id, 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 0);

  // re-insert 1024 with sub_count=4 and change data
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 1, 4, file_offsets, changes), 6);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 6);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->source_id, 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 0);
}

// test Type 1 with max_count=1, max_sub_count=0
void lmdb_hash_data_manager_type1_1_0() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 1, 0);

  // set up file_offsets
  file_offsets.insert(1024);

  // Insert offset 1024 with sub_count=2.
  TEST_EQ(manager.insert(binary_0, 0.0, "", 1, 2, file_offsets, changes), 2);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_offset_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 0);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 0.0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 2);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(source_id_offsets.begin()->source_id, 1);
  TEST_EQ(source_id_offsets.begin()->file_offsets.size(), 0);
}

// test Type 2 and Type 3
void lmdb_hash_data_manager_type2_and_type3() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;
  hashdb::source_id_offsets_t::const_iterator it;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 1);

  // set up file_offsets
  file_offsets.insert(512*1);
  file_offsets.insert(512*2);

  // set up Type 1
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 2, file_offsets, changes), 2);

  // set up file_offsets
  file_offsets.clear();
  file_offsets.insert(512*3);
  file_offsets.insert(512*4);

  // test insert_new_type2() and max_sub_count
  // Insert to new source_id=2, use sub_count 4, and change data
  // 512*4 will not fit because max_sub_count=3
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 2, 4, file_offsets, changes), 6);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 6);
  TEST_EQ(source_id_offsets.size(), 2);
  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);

  // set up file_offsets
  file_offsets.clear();
  file_offsets.insert(512*5);

  // test insert_new_type3() and max_count
  // Insert to new source_id=3, use sub_count 1, and change data
  // 512*5 will not fit because max_sub_count=2
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 1, file_offsets, changes), 7);
  TEST_EQ(changes.hash_data_source_inserted, 3);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 2);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.0);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 7);
  TEST_EQ(source_id_offsets.size(), 3);

  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 1);
  TEST_EQ(it->file_offsets.size(), 0);

  // test insert_update_type3() and max_count
  // Insert to existing source_id=2, use sub_count 1, and change data
  // 512*5 will not fit because max_sub_count=2
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 2, 1, file_offsets, changes), 8);
  TEST_EQ(changes.hash_data_source_inserted, 3);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 3);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 8);
  TEST_EQ(source_id_offsets.size(), 3);

  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 5);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 1);
  TEST_EQ(it->file_offsets.size(), 0);
}

void lmdb_hash_data_manager_count_and_iterator() {

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 1);

  // set up file_offsets
  std::set<uint64_t> file_offsets;
  file_offsets.insert(512*1);
  file_offsets.insert(512*2);

  // count in Type 1
  hashdb::lmdb_changes_t changes;
  manager.insert(binary_1, 0.0, "", 1, 10, file_offsets, changes);

  // count in Type 2
  manager.insert(binary_2, 0.0, "", 2, 5, file_offsets, changes);
  manager.insert(binary_2, 0.0, "", 3, 15, file_offsets, changes);

  // find_count
  TEST_EQ(manager.find_count(binary_0), 0);
  TEST_EQ(manager.find_count(binary_1), 10);
  TEST_EQ(manager.find_count(binary_2), 20);

  // iterator
  std::string block_hash = manager.first_hash();
  TEST_EQ(block_hash, binary_1);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, binary_2);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, "");
}

// ************************************************************
// lmdb_source_id_manager
// ************************************************************
void lmdb_source_id_manager() {
  // resources
  hashdb::lmdb_changes_t changes;
  bool did_find;
  bool did_insert;
  uint64_t source_id;
  std::string file_binary_hash;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_source_id_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // iterator when empty
  file_binary_hash = manager.first_source();
  TEST_EQ(file_binary_hash, "");

  // search when empty
  did_find = manager.find(binary_0, source_id);
  TEST_EQ(did_find, false);
  TEST_EQ(source_id, 0)

  // add items
  did_insert = manager.insert(binary_0, changes, source_id);
  TEST_EQ(did_insert, true);
  TEST_EQ(source_id, 1);
  TEST_EQ(changes.source_id_inserted, 1);
  TEST_EQ(changes.source_id_already_present, 0);

  did_insert = manager.insert(binary_0, changes, source_id);
  TEST_EQ(did_insert, false);
  TEST_EQ(source_id, 1);
  TEST_EQ(changes.source_id_inserted, 1);
  TEST_EQ(changes.source_id_already_present, 1);

  did_find = manager.find(binary_0, source_id);
  TEST_EQ(did_find, true);
  TEST_EQ(source_id, 1)

  // iterator
  did_insert = manager.insert(binary_2, changes, source_id);
  TEST_EQ(source_id, 2);
  did_insert = manager.insert(binary_1, changes, source_id);
  TEST_EQ(source_id, 3);
  file_binary_hash = manager.first_source();
  TEST_EQ(file_binary_hash, binary_0);
  file_binary_hash = manager.next_source(file_binary_hash);
  TEST_EQ(file_binary_hash, binary_1);
  file_binary_hash = manager.next_source(file_binary_hash);
  TEST_EQ(file_binary_hash, binary_2);
  file_binary_hash = manager.next_source(file_binary_hash);
  TEST_EQ(file_binary_hash, "");

  // allow empty request
  file_binary_hash = manager.next_source(file_binary_hash);
  TEST_EQ(file_binary_hash, "")

  // allow invalid request
  file_binary_hash = manager.next_source(binary_26);
  TEST_EQ(file_binary_hash, "")

}

// ************************************************************
// lmdb_source_data_manager
// ************************************************************
void lmdb_source_data_manager() {
  hashdb::lmdb_changes_t changes;

  // variables
  std::string file_binary_hash;
  uint64_t filesize;
  std::string file_type;
  uint64_t zero_count;
  uint64_t nonprobative_count;
  bool found;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_source_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // no source ID
  found =
    manager.find(1, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(found, false);

  // insert
  manager.insert(1, "fbh", 2, "ft", 1, 3, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 0);
  TEST_EQ(changes.source_data_same, 0);
  found =
    manager.find(1, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(found, true);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(zero_count, 1);
  TEST_EQ(nonprobative_count, 3);

  // insert same
  manager.insert(1, "fbh", 2, "ft", 1, 3, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 0);
  TEST_EQ(changes.source_data_same, 1);
  found =
    manager.find(1, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(found, true);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(zero_count, 1);
  TEST_EQ(nonprobative_count, 3);

  // change
  manager.insert(1, "fbh2", 22, "ft2", 31, 32, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 1);
  TEST_EQ(changes.source_data_same, 1);
  manager.find(1, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(file_binary_hash, "fbh2");
  TEST_EQ(filesize, 22);
  TEST_EQ(file_type, "ft2");
  TEST_EQ(zero_count, 31);
  TEST_EQ(nonprobative_count, 32);

  // insert second
  manager.insert(0, "", 0, "", 0, 0, changes);
  manager.find(0, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(file_binary_hash, "");
  TEST_EQ(filesize, 0);
  TEST_EQ(file_type, "");
  TEST_EQ(zero_count, 0);
  TEST_EQ(nonprobative_count, 0);

  // make sure 1 is still in place
  manager.find(1, file_binary_hash, filesize, file_type, zero_count, nonprobative_count);
  TEST_EQ(file_binary_hash, "fbh2");
  TEST_EQ(filesize, 22);
  TEST_EQ(file_type, "ft2");
  TEST_EQ(zero_count, 31);
  TEST_EQ(nonprobative_count, 32);

  // size
  TEST_EQ(manager.size(), 2);
}

// ************************************************************
// lmdb_source_name_manager
// ************************************************************
void lmdb_source_name_manager() {

  // variables
  source_names_t source_names;
  hashdb::lmdb_changes_t changes;
  bool found;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_source_name_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // no source ID when DB is empty
  found = manager.find(1, source_names);
  TEST_EQ(found, false);

  // insert first element
  manager.insert(1, "rn", "fn", changes);
  TEST_EQ(changes.source_name_inserted, 1);
  manager.insert(1, "rn", "fn", changes);
  TEST_EQ(changes.source_name_already_present, 1);
  manager.insert(1, "rn2", "fn2", changes);
  TEST_EQ(changes.source_name_inserted, 2);
  manager.insert(1, "rn1", "fn1", changes);
  TEST_EQ(changes.source_name_inserted, 3);

  // insert second element
  manager.insert(2, "rn11", "fn11", changes);
  TEST_EQ(changes.source_name_inserted, 4);

  // find first element
  found = manager.find(1, source_names);
  TEST_EQ(found, true);
  source_names_t::const_iterator it = source_names.begin();
  TEST_EQ(source_names.size(), 3);
  TEST_EQ(it->first, "rn");
  TEST_EQ(it->second, "fn");
  ++it;
  TEST_EQ(it->first, "rn1");
  TEST_EQ(it->second, "fn1");
  ++it;
  TEST_EQ(it->first, "rn2");
  TEST_EQ(it->second, "fn2");

  // find second element
  found = manager.find(2, source_names);
  TEST_EQ(found, true);
  it = source_names.begin();
  TEST_EQ(source_names.size(), 1);
  TEST_EQ(it->first, "rn11");
  TEST_EQ(it->second, "fn11");

  // no source ID when DB is not empty
  found = manager.find(3, source_names);
  TEST_EQ(found, false);
  TEST_EQ(source_names.size(), 0);

  // size
  TEST_EQ(manager.size(), 4);
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // lmdb_hash_manager
  lmdb_hash_manager_create();
  lmdb_hash_manager_write();
  lmdb_hash_manager_read();
  lmdb_hash_manager_settings();
  lmdb_hash_manager_count();

  // lmdb_hash_data_manager
  lmdb_hash_data_manager_empty();
  lmdb_hash_data_manager_type1();
  lmdb_hash_data_manager_type1_0_1();
  lmdb_hash_data_manager_type1_1_0();
  lmdb_hash_data_manager_type2_and_type3();
  lmdb_hash_data_manager_count_and_iterator();

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

