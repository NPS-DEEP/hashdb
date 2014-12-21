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
 * Test the hashdb_manager.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "file_modes.h"
#include "hashdb_directory_manager.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_element.hpp"
#include "../hash_t_selector.h"

// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir_hashdb_manager_test.hdb";

void write_settings() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // create the hashdb directory
  hashdb_directory_manager_t::create_new_hashdb_dir(temp_dir);

  // create working settings
  hashdb_settings_t settings;
  settings.maximum_hash_duplicates = 4;
  hashdb_settings_store_t::write_settings(temp_dir, settings);
}

void rw_new_tests() {

  // valid hashdigest values
  hash_t k1;
  hash_t k2;
  to_key(1, k1);
  to_key(2, k2);

  hashdb_element_t element;

  // open new hashdb manager
  hashdb_manager_t manager(temp_dir, RW_NEW);

  // ************************************************************
  // initial state
  // ************************************************************
  // check initial size
  TEST_EQ(manager.map_size(), 0);

  // check initial iterator
std::cerr << "hashdb_manager_test.rw_new_tests_it.a\n";
  TEST_EQ((manager.begin_key() == manager.end_key()), true);
std::cerr << "hashdb_manager_test.rw_new_tests_it.b\n";

  // ************************************************************
  // insert, remove, and hashdb_changes variables
  // note: some of these tests additionally test failure ordering
  // ************************************************************
  // insert valid
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 0);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_inserted, 1);
  TEST_EQ(manager.map_size(), 1);

  // insert, mismatched hash block size
  element = hashdb_element_t(k1, 5, "rep1", "file1", 0);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_not_inserted_mismatched_hash_block_size, 1);
  TEST_EQ(manager.map_size(), 1);

  // insert, file offset not aligned
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 5);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_not_inserted_invalid_byte_alignment, 1);
  TEST_EQ(manager.map_size(), 1);

  // insert, no exact duplicates
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 4096);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_inserted, 2);
  TEST_EQ(manager.map_size(), 2);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_not_inserted_duplicate_element, 1);
  TEST_EQ(manager.map_size(), 2);

  // max 4 elements of same hash
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 0);
  manager.insert(element);
  element = hashdb_element_t(k2, 4096, "rep1", "file2", 4096);
  manager.insert(element);
  element = hashdb_element_t(k2, 4096, "rep2", "file1", 4096);
  manager.insert(element);
  element = hashdb_element_t(k2, 4096, "rep3", "file1", 4096); // too many
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_not_inserted_exceeds_max_duplicates, 1);
  TEST_EQ(manager.map_size(), 5);

  // delete elements of same hash
  element = hashdb_element_t(k2, 4096, "rep3", "file1", 4096); // not present
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_removed, 0);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 1);
  TEST_EQ(manager.map_size(), 5);
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 4096);
  manager.remove(element);
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 0);
  manager.remove(element);
  element = hashdb_element_t(k2, 4096, "rep1", "file2", 4096);
  manager.remove(element);
  element = hashdb_element_t(k2, 4096, "rep2", "file1", 4096);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_removed, 4);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 1);
  TEST_EQ(manager.map_size(), 1);

  // remove, entry of single element
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 0);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_removed, 5);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 1);
  TEST_EQ(manager.map_size(), 0);

  // remove, no element
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 0);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 2);
  TEST_EQ(manager.map_size(), 0);

  // insert, valid, previously deleted
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 0);
  manager.insert(element);
  TEST_EQ(manager.changes.hashes_inserted, 6);

  // remove_hash successfully
  manager.remove_hash(k1);
  TEST_EQ(manager.changes.hashes_removed, 6);
  TEST_EQ(manager.map_size(), 0); //zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz

  // add two of same hash then remove_hash successfully
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 0);
  manager.insert(element);
  element = hashdb_element_t(k2, 4096, "rep1", "file2", 4096);
  manager.insert(element);
  manager.remove_hash(k2);
  TEST_EQ(manager.changes.hashes_removed, 8);

  // remove_hash no hash
  manager.remove_hash(k1);
  TEST_EQ(manager.changes.hashes_not_removed_no_hash, 1);

  // remove, mismatched hash block size
  element = hashdb_element_t(k1, 5, "rep1", "file1", 0);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_not_removed_mismatched_hash_block_size, 1);

  // remove, file offset not aligned
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 5);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_not_removed_invalid_byte_alignment, 1);

  // remove, no element
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 0);
  manager.remove(element);
  TEST_EQ(manager.changes.hashes_not_removed_no_element, 3);

  // ************************************************************
  // find, find_count, size, iterator
  // ************************************************************
  TEST_EQ(manager.find_count(k1), 0);
  TEST_EQ(manager.find_count(k1), 0);

  // setup with one element to make iterator simple
  element = hashdb_element_t(k1, 4096, "rep1", "file1", 0);
  manager.insert(element);
  TEST_EQ(manager.map_size(), 1);
std::cerr << "hashdb_manager_test.rw_new_tests_it.c\n";
  hash_store_key_iterator_t it(manager.begin_key());
std::cerr << "hashdb_manager_test.rw_new_tests_it.d " << &it << "\n";
  element = manager.hashdb_element(it);
  element = manager.hashdb_element(it);
  TEST_EQ(element.key, k1);
  TEST_EQ(element.hash_block_size, 4096);
  TEST_EQ(element.repository_name, "rep1");
  TEST_EQ(element.filename, "file1");
  TEST_EQ(element.file_offset, 0);
  ++it;
  TEST_EQ((it == manager.end_key()), true);

  // setup with two elements under one key and one element under another key
  element = hashdb_element_t(k1, 4096, "second_rep1", "file1", 0);
  manager.insert(element);
  element = hashdb_element_t(k2, 4096, "rep1", "file1", 0);
  manager.insert(element);

  TEST_EQ(manager.find_count(k1), 2);
  TEST_EQ(manager.map_size(), 3);

std::cerr << "hashdb_manager_test.rw_new_tests_it.e\n";
  hash_store_key_iterator_t it2(manager.begin_key());
std::cerr << "hashdb_manager_test.rw_new_tests_it.f " << &it << "\n";
  ++it2;
  it2++;
  ++it2;

  TEST_EQ((it2 == manager.end_key()), true);

  // check iterator pair from find
std::cerr << "hashdb_manager_test.rw_new_tests_it.g\n";
  hash_store_key_iterator_range_t it_pair;
std::cerr << "hashdb_manager_test.rw_new_tests_it.h " << &it_pair.first << ", " << &it_pair.second << "\n";
  it_pair = manager.find(k1);
  ++it_pair.first;
  ++it_pair.first;
  TEST_EQ((it_pair.first == it_pair.second), true);
  it_pair = manager.find(k1);
  ++it_pair.first;
  ++it_pair.first;
  TEST_EQ((it_pair.first == it_pair.second), true);

std::cerr << "hashdb_manager_test.insert.u\n";
  // ************************************************************
  // install lots of data
  // ************************************************************
  // populate with 1,000,000 entries
  hash_t key;
  TEST_EQ(manager.map_size(), 3);
//  int count = 1000000;
  uint count = 1000;
  for (uint64_t n=0; n< count; ++n) {
    to_key(n+1000000, key);
    element = hashdb_element_t(key, 4096, "rep1", "file1", 0);
    manager.insert(element);
  }
  TEST_EQ(manager.map_size(), 1000003);
std::cerr << "hashdb_manager_test.insert.v\n";
}

void ro_tests() {
  // validate that two managers can open the same database read_only
  hashdb_manager_t manager1(temp_dir, READ_ONLY);
  TEST_EQ(manager1.map_size(), 1000003);
  hashdb_manager_t manager2(temp_dir, READ_ONLY);
  TEST_EQ(manager2.map_size(), 1000003);
}

int main(int argc, char* argv[]) {

  make_dir_if_not_there(temp_dir);

std::cerr << "hashdb_manager_test.main.a\n";
  write_settings();
std::cerr << "hashdb_manager_test.main.b\n";
  rw_new_tests();
std::cerr << "hashdb_manager_test.main.c\n";
  ro_tests();
std::cerr << "hashdb_manager_test.main.d\n";

  return 0;
}

