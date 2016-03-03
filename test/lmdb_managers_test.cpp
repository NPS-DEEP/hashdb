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
#include "../src_libhashdb/hashdb.hpp"
#include "../src/hex_helper.hpp"
#include "directory_helper.hpp"

typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

typedef std::pair<std::string, std::string> source_name_t;
typedef std::set<source_name_t>             source_names_t;

static const std::string hashdb_dir = "temp_dir_lmdb_managers_test.hdb";
static const std::string binary_0(hex_to_bin(
                                  "00000000000000000000000000000000"));
static const std::string binary_1(hex_to_bin(
                                  "00000000000000000000000000000001"));
static const std::string binary_2(hex_to_bin(
                                  "00000000000000000000000000000002"));
static const std::string binary_10(hex_to_bin(
                                  "10000000000000000000000000000000"));
static const std::string binary_11(hex_to_bin(
                                  "10000000000000000000000000000001"));
static const std::string binary_12(hex_to_bin(
                                  "10000000000000000000000000000002"));
static const std::string binary_13(hex_to_bin(
                                  "10000000000000000000000000000003"));
static const std::string binary_14(hex_to_bin(
                                  "10000000000000000000000000000004"));
static const std::string binary_15(hex_to_bin(
                                  "10000000000000000000000000000005"));
static const std::string binary_26(hex_to_bin(
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
    manager.insert(hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);

    TEST_EQ(manager.find(hex_to_bin("00000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hex_to_bin("7fffffffffffffffffffffffffffffff")),0);
    TEST_EQ(manager.find(hex_to_bin("80000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hex_to_bin("ffffffffffffffffffffffffffffffff")),1);

  } { // demonstrate that the db is cleared
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 0);
    TEST_EQ(manager.size(), 0);
    TEST_EQ(manager.find(hex_to_bin("00000000000000000000000000000000")),0);

  } { // 1 prefix bit, no suffix, demonstrate adding 0 instead of 1
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 0);
    manager.insert(hex_to_bin("00000000000000000000000000000000"), 1, changes);

    TEST_EQ(manager.find(hex_to_bin("00000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hex_to_bin("80000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hex_to_bin("7fffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hex_to_bin("8fffffffffffffffffffffffffffffff")),0);

  } { // 2 prefix bits, no suffix
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 2, 0);

    manager.insert(hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);
    TEST_EQ(manager.find(hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hex_to_bin("cfffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hex_to_bin("c0000000000000000000000000000000")),1);
    TEST_EQ(manager.find(hex_to_bin("40000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hex_to_bin("80000000000000000000000000000000")),0);

  } { // 1 prefix bit, 1 suffix byte
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 1);

    manager.insert(hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);

    TEST_EQ(manager.find(hex_to_bin("00000000000000000000000000000000")),0);
    TEST_EQ(manager.find(hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hex_to_bin("800000000000000000000000000000ff")),1);
    TEST_EQ(manager.find(hex_to_bin("700000000000000000000000000000ff")),0);
    TEST_EQ(manager.find(hex_to_bin("800000000000000000000000000000fe")),0);

  } { // 9 prefix bits, 2 suffix bytes
    make_new_hashdb_dir(hashdb_dir);
    hashdb::lmdb_hash_manager_t manager(hashdb_dir, hashdb::RW_NEW, 9, 2);

    manager.insert(hex_to_bin("ffffffffffffffffffffffffffffffff"), 1, changes);
    TEST_EQ(manager.find(hex_to_bin("ffffffffffffffffffffffffffffffff")),1);
    TEST_EQ(manager.find(hex_to_bin("ffff000000000000000000000000ffff")),1);
    TEST_EQ(manager.find(hex_to_bin("ff80000000000000000000000000ffff")),1);
    TEST_EQ(manager.find(hex_to_bin("ff00000000000000000000000000ffff")),0);
    TEST_EQ(manager.find(hex_to_bin("ff800000000000000000000000007fff")),0);
    TEST_EQ(manager.find(hex_to_bin("ff80000000000000000000000000ff7f")),0);
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
void lmdb_hash_data_manager() {

  // variables
  uint64_t entropy;
  std::string block_label;
  id_offset_pairs_t id_offset_pairs;
  hashdb::lmdb_changes_t changes;
  id_offset_pairs_t::const_iterator it;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                                    hashdb_dir, hashdb::RW_NEW, 512, 100000);

  TEST_EQ(id_offset_pairs.size(), 0);

  // binary_0 not there
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), 0);
  TEST_EQ(manager.find_count(binary_0), 0);

  // invalid file offset, count = 0
  TEST_EQ(manager.insert(binary_0, 1, 513, 1, "bl", changes), 0);
  TEST_EQ(changes.hash_data_data_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 0);
  TEST_EQ(changes.hash_data_source_inserted, 0);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 1);

  // insert binary_0
  TEST_EQ(manager.insert(binary_0, 1, 512, 1, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);

  // binary_0 there
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), true);
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");
  TEST_EQ(id_offset_pairs.size(), 1);
  it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);
  TEST_EQ(manager.find_count(binary_0), 1);

  // binary_1 not there
  TEST_EQ(manager.find(binary_1, entropy, block_label, id_offset_pairs), 0);

  // insert same hash, same source, same data
  TEST_EQ(manager.insert(binary_0, 1, 512, 1, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 1);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 1);
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), true);
  TEST_EQ(entropy, 1);
  TEST_EQ(block_label, "bl");
  TEST_EQ(id_offset_pairs.size(), 1);
  it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);

  // same hash, same source, different data
  TEST_EQ(manager.insert(binary_0, 1, 512, 2, "bl2", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 1);
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), true);
  TEST_EQ(entropy, 2);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(id_offset_pairs.size(), 1);
  it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);

  // add same hash, new source, same data
  TEST_EQ(manager.insert(binary_0, 1, 1024, 2, "bl2", changes), 2);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 2);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 1);
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), true);
  TEST_EQ(entropy, 2);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(id_offset_pairs.size(), 2);
  it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);
  ++it;
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 1024);
  TEST_EQ(manager.find_count(binary_0), 2);

  // same hash, new source, different data
  size_t count;
  count = manager.insert(binary_0, 1, 2048, 3, "bl33", changes);
  TEST_EQ(count, 3);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 2);
  TEST_EQ(changes.hash_data_data_same, 2);
  TEST_EQ(changes.hash_data_source_inserted, 3);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 1);
  TEST_EQ(manager.find(binary_0, entropy, block_label, id_offset_pairs), true);
  TEST_EQ(entropy, 3);
  TEST_EQ(block_label, "bl33");
  TEST_EQ(id_offset_pairs.size(), 3);
  it = id_offset_pairs.begin();
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 512);
  ++it;
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 1024);
  ++it;
  TEST_EQ(it->first, 1);
  TEST_EQ(it->second, 2048);
  TEST_EQ(manager.find_count(binary_0), 3);
  
  // check iterator
  count = manager.insert(binary_1, 2, 4096, 4, "bl4", changes);
  TEST_EQ(count, 1);
  TEST_EQ(changes.hash_data_data_inserted, 2);
  std::string binary_hash;
  bool did_find;
  did_find = manager.find_begin(binary_hash);
  TEST_EQ(did_find, true);
  TEST_EQ(binary_hash, binary_0)
  did_find = manager.find_next(binary_hash, binary_hash);
  TEST_EQ(did_find, true);
  TEST_EQ(binary_hash, binary_1)
  did_find = manager.find_next(binary_hash, binary_hash);
  TEST_EQ(did_find, false);
  TEST_EQ(binary_hash, "")

  // size
  TEST_EQ(manager.size(), 5);
}

void lmdb_hash_data_manager_settings1() {
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 1);

  // initial state
  TEST_EQ(changes.hash_data_data_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 0);
  TEST_EQ(changes.hash_data_source_inserted, 0);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add hash data
  TEST_EQ(manager.insert(binary_0, 1, 1, 1, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 0);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, same hash data, same source
  TEST_EQ(manager.insert(binary_0, 1, 1, 1, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 1);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, same hash data, different source at max
  TEST_EQ(manager.insert(binary_0, 1, 2, 1, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 2);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 2);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, different hash data, different source at max
  size_t count = manager.insert(binary_0, 1, 3, 12, "bl2", changes);
  TEST_EQ(count, 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 2);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 3);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);
}

void lmdb_hash_data_manager_settings2() {
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW, 1, 2);

  // initial state
  TEST_EQ(changes.hash_data_data_inserted, 0);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 0);
  TEST_EQ(changes.hash_data_source_inserted, 0);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add hash data and source
  TEST_EQ(manager.insert(binary_0, 1, 1, 10, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 0);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 0);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, same hash data, same source
  TEST_EQ(manager.insert(binary_0, 1, 1, 10, "bl", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 0);
  TEST_EQ(changes.hash_data_data_same, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 1);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, different hash data, same source
  TEST_EQ(manager.insert(binary_0, 1, 1, 12, "bl2", changes), 1);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 1);
  TEST_EQ(changes.hash_data_source_inserted, 1);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, same hash data, different source
  TEST_EQ(manager.insert(binary_0, 1, 2, 12, "bl2", changes), 2);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 2);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 0);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, same hash data, different source above max
  TEST_EQ(manager.insert(binary_0, 1, 3, 12, "bl2", changes), 2);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 1);
  TEST_EQ(changes.hash_data_data_same, 3);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 1);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);

  // add same hash, different hash data, different source above max
  TEST_EQ(manager.insert(binary_0, 1, 3, 13, "bl33", changes), 2);
  TEST_EQ(changes.hash_data_data_inserted, 1);
  TEST_EQ(changes.hash_data_data_changed, 2);
  TEST_EQ(changes.hash_data_data_same, 3);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_source_already_present, 2);
  TEST_EQ(changes.hash_data_source_at_max, 2);
  TEST_EQ(changes.hash_data_invalid_file_offset, 0);
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
  did_find = manager.find_begin(file_binary_hash);
  TEST_EQ(did_find, false);
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
  did_find = manager.find_begin(file_binary_hash);
  TEST_EQ(did_find, true);
  TEST_EQ(file_binary_hash, binary_0);
  did_find = manager.find_next(file_binary_hash, file_binary_hash);
  TEST_EQ(did_find, true);
  TEST_EQ(file_binary_hash, binary_1);
  did_find = manager.find_next(file_binary_hash, file_binary_hash);
  TEST_EQ(did_find, true);
  TEST_EQ(file_binary_hash, binary_2);
  did_find = manager.find_next(file_binary_hash, file_binary_hash);
  TEST_EQ(did_find, false);
  TEST_EQ(file_binary_hash, "");
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
  uint64_t nonprobative_count;
  bool found;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_source_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // no source ID
  found =
    manager.find(1, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(found, false);

  // insert
  manager.insert(1, "fbh", 2, "ft", 3, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 0);
  TEST_EQ(changes.source_data_same, 0);
  found =
    manager.find(1, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(found, true);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(nonprobative_count, 3);

  // insert same
  manager.insert(1, "fbh", 2, "ft", 3, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 0);
  TEST_EQ(changes.source_data_same, 1);
  found =
    manager.find(1, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(found, true);
  TEST_EQ(file_binary_hash, "fbh");
  TEST_EQ(filesize, 2);
  TEST_EQ(file_type, "ft");
  TEST_EQ(nonprobative_count, 3);

  // change
  manager.insert(1, "fbh2", 22, "ft2", 32, changes);
  TEST_EQ(changes.source_data_inserted, 1);
  TEST_EQ(changes.source_data_changed, 1);
  TEST_EQ(changes.source_data_same, 1);
  manager.find(1, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(file_binary_hash, "fbh2");
  TEST_EQ(filesize, 22);
  TEST_EQ(file_type, "ft2");
  TEST_EQ(nonprobative_count, 32);

  // insert second
  manager.insert(0, "", 0, "", 0, changes);
  manager.find(0, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(file_binary_hash, "");
  TEST_EQ(filesize, 0);
  TEST_EQ(file_type, "");
  TEST_EQ(nonprobative_count, 0);

  // make sure 1 is still in place
  manager.find(1, file_binary_hash, filesize, file_type, nonprobative_count);
  TEST_EQ(file_binary_hash, "fbh2");
  TEST_EQ(filesize, 22);
  TEST_EQ(file_type, "ft2");
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
  lmdb_hash_data_manager();
  lmdb_hash_data_manager_settings1();
  lmdb_hash_data_manager_settings2();

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

