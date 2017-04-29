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

void make_new_hashdb_dir(std::string p_hashdb_dir) {
  // remove any previous hashdb_dir
  rm_hashdb_dir(p_hashdb_dir);

  // create the hashdb directory
  require_no_dir(p_hashdb_dir);
  create_new_dir(p_hashdb_dir);
}

void check_changes(const hashdb::lmdb_changes_t& changes,
                   const size_t hash_data_inserted,
                   const size_t hash_data_merged,
                   const size_t hash_data_merged_same,
                   const size_t hash_data_mismatched_data_detected,
                   const size_t hash_data_mismatched_sub_count_detected) {

  // enable for diagnostics
  //std::cerr << hash_data_source_inserted
  //          << "," << hash_data_inserted
  //          << "," << hash_data_merged
  //          << "," << hash_data_merged_same
  //          << "," << hash_data_mismatched_data_detected
  //          << "," << hash_data_mismatched_sub_count_detected
  //          << "\n";

  // test each
  TEST_EQ(changes.hash_data_inserted, hash_data_inserted);
  TEST_EQ(changes.hash_data_merged, hash_data_merged);
  TEST_EQ(changes.hash_data_merged_same, hash_data_merged_same);
  TEST_EQ(changes.hash_data_mismatched_data_detected,
          hash_data_mismatched_data_detected);
  TEST_EQ(changes.hash_data_mismatched_sub_count_detected,
          hash_data_mismatched_sub_count_detected);
}

void test_empty() {

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // attempt to insert an empty key.  A warning is sent to stderr.
  TEST_EQ(manager.insert("", 1000, "bl", 1, 0, changes), 0);

  // validate changes
  check_changes(changes,0,0,0,0,0);

  // check that binary_0 is not there
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                                             source_id_offsets), false);
  TEST_FLOAT_EQ(k_entropy, 0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_offsets.size(), 0);

  // iterator
  TEST_EQ(manager.first_hash(), "");

  // size
  TEST_EQ(manager.size(), 0);
}

// insert
void test_insert_type1() {

// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // insert something at source_id=1
  TEST_EQ(manager.insert(binary_0, 1000, "bl", 1, changes), 1);
  check_changes(changes,1,0,0,0,0);
  TEST_EQ(manager.insert(binary_0, 2000, "bl", 1, changes), 2);
  check_changes(changes,2,0,0,1,0);
  TEST_EQ(manager.insert(binary_0, 1000, "zz", 1, changes), 3);
  check_changes(changes,3,0,0,2,0);
  TEST_EQ(manager.insert(binary_0, 2000, "zz", 1, changes), 4);
  check_changes(changes,3,0,0,3,0);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 4);
  TEST_EQ(source_id_sub_counts.size(), 1);

  // validate nothing at binary_1
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), false);
  TEST_EQ(k_entropy, 0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_sub_counts.size(), 0);
}

// insert split
void test_insert_split() {

// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;


  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // insert something at source_id=1
  TEST_EQ(manager.insert(binary_0, 1000, "bl", 1, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 1, changes), 2);

  // split type 1 into type 2 and two type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 1);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 3);
  TEST_EQ(source_id_sub_counts.size(), 2);
  TEST_EQ(source_id_sub_counts[0].source_id, 1);
  TEST_EQ(source_id_sub_counts[0].sub_count, 2);
  TEST_EQ(source_id_sub_counts[1].source_id, 2);
  TEST_EQ(source_id_sub_counts[1].sub_count, 1);

  // change type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 1);
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 5);
  TEST_EQ(source_id_sub_counts.size(), 2);
  TEST_EQ(source_id_sub_counts[0].source_id, 1);
  TEST_EQ(source_id_sub_counts[0].sub_count, 2);
  TEST_EQ(source_id_sub_counts[1].source_id, 2);
  TEST_EQ(source_id_sub_counts[1].sub_count, 3);

  // new type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 1);
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 9);
  TEST_EQ(source_id_sub_counts.size(), 3);
  TEST_EQ(source_id_sub_counts[0].source_id, 1);
  TEST_EQ(source_id_sub_counts[0].sub_count, 2);
  TEST_EQ(source_id_sub_counts[1].source_id, 2);
  TEST_EQ(source_id_sub_counts[1].sub_count, 3);
  TEST_EQ(source_id_sub_counts[2].source_id, 3);
  TEST_EQ(source_id_sub_counts[2].sub_count, 4);

  // final
  check_changes(changes,8,0,0,7,0);
}

// merge
void test_merge() {

// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // merge something at source_id=1
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 10);
  check_changes(changes,1,10,0,0,0);
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 10);
  check_changes(changes,1,10,10,0,0);
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 11, changes), 10);
  check_changes(changes,1,10,10,0,1);
  TEST_EQ(manager.merge(binary_0, 1000, "l", 1, 10, changes), 10);
  check_changes(changes,1,10,10,1,1);
  TEST_EQ(manager.merge(binary_0, 2000, "z", 1, 10, changes), 10);
  check_changes(changes,1,10,10,2,1);
  TEST_EQ(manager.merge(binary_0, 1000, "z", 1, 10, changes), 10);
  check_changes(changes,1,10,10,3,1);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 10);
  TEST_EQ(source_id_sub_counts.size(), 1);

  // split type 1 into type 2 and two type 3
  TEST_EQ(manager.merge(binary_0, 0, "", 2, 100, changes), 1);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 110);
  TEST_EQ(source_id_sub_counts.size(), 2);
  TEST_EQ(source_id_sub_counts[0].source_id, 1);
  TEST_EQ(source_id_sub_counts[0].sub_count, 10);
  TEST_EQ(source_id_sub_counts[1].source_id, 2);
  TEST_EQ(source_id_sub_counts[1].sub_count, 100);

  // adding to type 3 does nothing, source_id=1
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 10);
  check_changes(changes,0,110,20,3,1);
  TEST_EQ(manager.merge(binary_0, 1000, "", 1, 100, changes), 10);
  check_changes(changes,0,110,20,3,2);

  // adding to type 3 does nothing, source_id=2
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 100, changes), 10);
  check_changes(changes,0,110,120,3,2);
  TEST_EQ(manager.merge(binary_0, 1000, "", 1, 10, changes), 10);
  check_changes(changes,0,110,120,3,3);

  // new type 3
  TEST_EQ(manager.merge(binary_2, 0, "", 3, 1000, changes), 1);
  check_changes(changes,0,1110,120,3,3);

  // find
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 1110);
  TEST_EQ(source_id_sub_counts.size(), 3);
  TEST_EQ(source_id_sub_counts[0].source_id, 1);
  TEST_EQ(source_id_sub_counts[0].sub_count, 10);
  TEST_EQ(source_id_sub_counts[1].source_id, 2);
  TEST_EQ(source_id_sub_counts[1].sub_count, 100);
  TEST_EQ(source_id_sub_counts[2].source_id, 3);
  TEST_EQ(source_id_sub_counts[2].sub_count, 1000);
}

void test_maximums() {
// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // merge above max at source_id=1
  TEST_EQ(manager.merge(binary_0, 0, "", 1, 65536, changes), 65535);
  check_changes(changes,1,65535,0,0,0);

  // validate clip
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(count, 65535);

  // insert above max at source_id=1
  TEST_EQ(manager.insert(binary_0, 0, "", 1, changes), 65535);
  check_changes(changes,1,65535,0,0,0);

  // accept more at source_id=2
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 65535);
  check_changes(changes,1,65535,0,0,0);
}

void test_block_label() {
// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // test max block_label length, Type 1
  manager.insert(binary_0, 0, "0123456789a", 1, changes);
  check_changes(changes,1,0,0,0,0);
  manager.find(binary_1, k_entropy, block_label, count, source_id_offsets);
  TEST_EQ(block_label, "0123456789");

  manager.merge(binary_1, 0, "0123456789a", 1, 10, changes);
  check_changes(changes,1,10,0,0,0);
  manager.find(binary_2, k_entropy, block_label, count, source_id_offsets);
  TEST_EQ(block_label, "0123456789");

  // test max block_label length during transition to type 2
  manager.insert(binary_0, 0, "0123456789a", 2, changes);
  check_changes(changes,2,10,0,0,0);
  manager.find(binary_1, k_entropy, block_label, count, source_id_offsets);
  TEST_EQ(block_label, "0123456789");

  // test max block_label length when at type 2
  manager.insert(binary_0, 0, "0123456789a", 1, changes);
  check_changes(changes,3,10,0,0,0);
  manager.find(binary_1, k_entropy, block_label, count, source_id_offsets);
  TEST_EQ(block_label, "0123456789");
}

void test_other_manager_functions() {
// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // add some items
  manager.insert(binary_1, 0, "", 1, changes);
  manager.merge(binary_1, 0, "", 2, 4, changes);
  manager.merge(binary_2, 0, "", 1, 10, changes);
  
  // find_count
  TEST_EQ(manager.find_count(binary_0), 0);
  TEST_EQ(manager.find_count(binary_1), 5);
  TEST_EQ(manager.find_count(binary_2), 10);

  // iterator
  std::string block_hash = manager.first_hash();
  TEST_EQ(block_hash, binary_1);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, binary_2);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, "");

  // size
  TEST_EQ(manager.size(), 4);
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

test_empty();
test_insert_type1();
test_insert_split();
test_merge();
test_maximums();
test_block_label();
test_other_manager_functions();

  // done
  std::cout << "lmdb_hash_data_manager_test Done.\n";
  return 0;
}

