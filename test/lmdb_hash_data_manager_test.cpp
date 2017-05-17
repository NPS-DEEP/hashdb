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
#include "source_id_sub_counts.hpp"
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
  // std::cout << "test check_changes.a";
  TEST_EQ(changes.hash_data_inserted, hash_data_inserted);
  // std::cout << "b";
  TEST_EQ(changes.hash_data_merged, hash_data_merged);
  // std::cout << "c";
  TEST_EQ(changes.hash_data_merged_same, hash_data_merged_same);
  // std::cout << "d";
  TEST_EQ(changes.hash_data_mismatched_data_detected,
          hash_data_mismatched_data_detected);
  // std::cout << "e";
  TEST_EQ(changes.hash_data_mismatched_sub_count_detected,
          hash_data_mismatched_sub_count_detected);
  // std::cout << "f\n";
}

void test_empty() {

  // variables
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // attempt to insert an empty key.  A warning is sent to stderr.
  TEST_EQ(manager.insert("", 1000, "bl", 1, changes), 0);

  // validate changes
  // std::cout << "test_empty.a\n";
  check_changes(changes,0,0,0,0,0);

  // check that binary_0 is not there
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                                             source_id_sub_counts), false);
  TEST_FLOAT_EQ(k_entropy, 0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_sub_counts.size(), 0);

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
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // insert something at source_id=1
  TEST_EQ(manager.insert(binary_0, 1000, "bl", 1, changes), 1);
  // std::cout << "test_insert_type1.a\n";
  check_changes(changes,1,0,0,0,0);
  TEST_EQ(manager.insert(binary_0, 2000, "bl", 1, changes), 2);
  // std::cout << "test_insert_type1.b\n";
  check_changes(changes,2,0,0,1,0);
  TEST_EQ(manager.insert(binary_0, 1000, "zz", 1, changes), 3);
  // std::cout << "test_insert_type1.c\n";
  check_changes(changes,3,0,0,2,0);
  TEST_EQ(manager.insert(binary_0, 2000, "zz", 1, changes), 4);
  // std::cout << "test_insert_type1.d\n";
  check_changes(changes,4,0,0,3,0);
  // std::cout << "test_insert_type1.e\n";

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 4);
  TEST_EQ(source_id_sub_counts.size(), 1);

  // validate nothing at binary_1
  TEST_EQ(manager.find(binary_1, k_entropy, block_label, count,
                       source_id_sub_counts), false);
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
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;


  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // insert something at source_id=1
  TEST_EQ(manager.insert(binary_0, 1000, "bl", 1, changes), 1);
  TEST_EQ(manager.insert(binary_0, 0, "", 1, changes), 2);

  // split type 1 into type 2 and two type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 3);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 3);
  TEST_EQ(source_id_sub_counts.size(), 2);
  auto it = source_id_sub_counts.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 1);

  // change type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 4);
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 5);
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 5);
  TEST_EQ(source_id_sub_counts.size(), 2);
  it = source_id_sub_counts.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 3);

  // new type 3
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 6);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 7);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 8);
  TEST_EQ(manager.insert(binary_0, 0, "", 3, changes), 9);
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 1000);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 9);
  TEST_EQ(source_id_sub_counts.size(), 3);
  it = source_id_sub_counts.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 3);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 4);

  // final
  // std::cout << "test_insert_split.a\n";
  check_changes(changes,9,0,0,8,0);
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
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // merge something at source_id=1
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 10);
  // std::cout << "test_merge.a\n";
  check_changes(changes,0,1,0,0,0);
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 10);
  // std::cout << "test_merge.b\n";
  check_changes(changes,0,1,1,0,0);
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 11, changes), 10);
  // std::cout << "test_merge.c\n";
  check_changes(changes,0,1,2,0,1);
  TEST_EQ(manager.merge(binary_0, 1000, "l", 1, 10, changes), 10);
  // std::cout << "test_merge.d\n";
  check_changes(changes,0,1,3,1,1);
  TEST_EQ(manager.merge(binary_0, 2000, "z", 1, 10, changes), 10);
  // std::cout << "test_merge.e\n";
  check_changes(changes,0,1,4,2,1);
  TEST_EQ(manager.merge(binary_0, 1000, "z", 1, 10, changes), 10);
  // std::cout << "test_merge.f\n";
  check_changes(changes,0,1,5,3,1);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 10);
  TEST_EQ(source_id_sub_counts.size(), 1);

  // split type 1 into type 2 and two type 3
  TEST_EQ(manager.merge(binary_0, 0, "", 2, 100, changes), 110);
  // std::cout << "test_merge.g\n";
  check_changes(changes,0,2,5,4,1);

  // validate storage for binary_0
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 110);
  TEST_EQ(source_id_sub_counts.size(), 2);

  auto it = source_id_sub_counts.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 10);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 100);

  // adding to type 3 does nothing, source_id=1
  TEST_EQ(manager.merge(binary_0, 2000, "l", 1, 10, changes), 110);
  // std::cout << "test_merge.h\n";
  check_changes(changes,0,2,6,4,1);
  TEST_EQ(manager.merge(binary_0, 1000, "", 1, 100, changes), 110);
  // std::cout << "test_merge.i\n";
  check_changes(changes,0,2,7,5,2);

  // adding to type 3 does nothing, source_id=2
  TEST_EQ(manager.merge(binary_0, 2000, "l", 2, 100, changes), 110);
  // std::cout << "test_merge.j\n";
  check_changes(changes,0,2,8,5,2);
  TEST_EQ(manager.merge(binary_0, 1000, "", 2, 10, changes), 110);
  // std::cout << "test_merge.k\n";
  check_changes(changes,0,2,9,6,3);

  // new type 3
  TEST_EQ(manager.merge(binary_0, 0, "", 3, 1000, changes), 1110);
  // std::cout << "test_merge.l\n";
  check_changes(changes,0,3,9,7,3);

  // find
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(k_entropy, 2000);
  TEST_EQ(block_label, "l");
  TEST_EQ(count, 1110);
  TEST_EQ(source_id_sub_counts.size(), 3);
  it = source_id_sub_counts.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 10);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 100);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 1000);
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
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // merge above max at source_id=1
  TEST_EQ(manager.merge(binary_0, 0, "", 1, 65536, changes), 65535);
  // std::cout << "test_maximums.a\n";
  check_changes(changes,0,1,0,0,0);

  // validate clip
  TEST_EQ(manager.find(binary_0, k_entropy, block_label, count,
                       source_id_sub_counts), true);
  TEST_EQ(count, 65535);

  // insert above max at source_id=1
  TEST_EQ(manager.insert(binary_0, 0, "", 1, changes), 65535);
  // std::cout << "test_maximums.b\n";
  check_changes(changes,1,1,0,0,0);

  // accept more at source_id=2
  TEST_EQ(manager.insert(binary_0, 0, "", 2, changes), 65536);
  // std::cout << "test_maximums.c\n";
  check_changes(changes,2,1,0,0,0);
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
  hashdb::source_id_sub_counts_t source_id_sub_counts;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(hashdb_dir, hashdb::RW_NEW);

  // test max block_label length, Type 1
  manager.insert(binary_0, 0, "0123456789a", 1, changes);
  // std::cout << "test_block_label.a\n";
  check_changes(changes,1,0,0,0,0);
  manager.find(binary_0, k_entropy, block_label, count, source_id_sub_counts);
  TEST_EQ(block_label, "0123456789");

  // add record at different block hash
  manager.merge(binary_1, 0, "123456789ab", 1, 10, changes);
  // std::cout << "test_block_label.b\n";
  check_changes(changes,1,1,0,0,0);
  manager.find(binary_1, k_entropy, block_label, count, source_id_sub_counts);
  TEST_EQ(block_label, "123456789a");

  // test max block_label length during transition to type 2
  manager.insert(binary_0, 0, "0123456789a", 2, changes);
  // std::cout << "test_block_label.c\n";
  check_changes(changes,2,1,0,0,0);
  manager.find(binary_0, k_entropy, block_label, count, source_id_sub_counts);
  TEST_EQ(block_label, "0123456789");

  // test max block_label length when at type 2
  manager.insert(binary_0, 0, "0123456789a", 1, changes);
  // std::cout << "test_block_label.d\n";
  check_changes(changes,3,1,0,0,0);
  manager.find(binary_0, k_entropy, block_label, count, source_id_sub_counts);
  TEST_EQ(block_label, "0123456789");
}

void test_other_manager_functions() {
// hash_data_inserted
// hash_data_merged
// hash_data_merged_same
// hash_data_mismatched_data_detected
// hash_data_mismatched_sub_count_detected

  // variables
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

