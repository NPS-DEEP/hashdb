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
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "file_modes.h"
#include "hashdb_manager.hpp"
#include "hashdb_iterator.hpp"
#include "hashdb_element.hpp"
#include "../hash_t_selector.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir_hashdb_manager_test";

void write_settings() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // create working settings
  hashdb_settings_t settings;
  settings.maximum_hash_duplicates = 4;
  hashdb_settings_manager_t::write_settings(temp_dir, settings);
}

template<typename T>
void do_test() {

  // valid hashdigest values
  T k1;
  T k2;
  to_key(1, k1);
  to_key(2, k2);

  hashdb_element_t<T> element;

  // create working changes object
  hashdb_changes_t changes;

  // open new hashdb manager
  hashdb_manager_t<T> manager(temp_dir, RW_NEW);

  // ************************************************************
  // initial state
  // ************************************************************
  // check initial size
  BOOST_TEST_EQ(manager.map_size(), 0);

  // check initial iterator
  BOOST_TEST_EQ((manager.begin() == manager.end()), true);

  // ************************************************************
  // insert, remove, and hashdb_changes variables
  // note: some of these tests additionally test failure ordering
  // ************************************************************
  // insert valid
  element = hashdb_element_t<T>(k1,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 1);
  BOOST_TEST_EQ(manager.map_size(), 1);

  // insert, mismatched hash block size
  element = hashdb_element_t<T>(k1,
                             5, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_mismatched_hash_block_size, 1);
  BOOST_TEST_EQ(manager.map_size(), 1);

  // insert, file offset not aligned
  element = hashdb_element_t<T>(k1,
                             4096, "rep1", "file1", 5);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_invalid_byte_alignment, 1);
  BOOST_TEST_EQ(manager.map_size(), 1);

  // insert, no exact duplicates
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file1", 4096);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 2);
  BOOST_TEST_EQ(manager.map_size(), 2);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_duplicate_element, 1);
  BOOST_TEST_EQ(manager.map_size(), 2);

  // max 4 elements of same hash
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file2", 4096);
  manager.insert(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep2", "file1", 4096);
  manager.insert(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep3", "file1", 4096); // too many
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_exceeds_max_duplicates, 1);
  BOOST_TEST_EQ(manager.map_size(), 5);

  // delete elements of same hash
  element = hashdb_element_t<T>(k2, 4096, "rep3", "file1", 4096); // not present
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 0);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 1);
  BOOST_TEST_EQ(manager.map_size(), 5);
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file1", 4096);
  manager.remove(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file2", 4096);
  manager.remove(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep2", "file1", 4096);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 4);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 1);
  BOOST_TEST_EQ(manager.map_size(), 1);

  // remove, entry of single element
  element = hashdb_element_t<T>(k1, 4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 5);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 1);
  BOOST_TEST_EQ(manager.map_size(), 0);

  // remove, no element
  element = hashdb_element_t<T>(k1, 4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 2);
  BOOST_TEST_EQ(manager.map_size(), 0);

  // insert, valid, previously deleted
  element = hashdb_element_t<T>(k1, 4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 6);

  // remove_key successfully
  manager.remove_key(k1, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 6);

  // add two of same key then remove_key successfully
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  element = hashdb_element_t<T>(k2, 4096, "rep1", "file2", 4096);
  manager.insert(element, changes);
  manager.remove_key(k2, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 8);

  // remove_key no hash
  manager.remove_key(k1, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_hash, 1);

  // remove, mismatched hash block size
  element = hashdb_element_t<T>(k1,
                             5, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_mismatched_hash_block_size, 1);

  // remove, file offset not aligned
  element = hashdb_element_t<T>(k1,
                             4096, "rep1", "file1", 5);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_invalid_byte_alignment, 1);

  // remove, no element
  element = hashdb_element_t<T>(k2,
                             4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 3);

  // ************************************************************
  // find, find_count, size, iterator
  // ************************************************************
  BOOST_TEST_EQ(manager.find_count(k1), 0);
  BOOST_TEST_EQ(manager.find_count(k1), 0);

  // setup with one element to make iterator simple
  element = hashdb_element_t<T>(k1,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(manager.map_size(), 1);
  hashdb_iterator_t<T> it(manager.begin());
  BOOST_TEST_EQ(it->key, k1);
  BOOST_TEST_EQ(it->hash_block_size, 4096);
  BOOST_TEST_EQ(it->repository_name, "rep1");
  BOOST_TEST_EQ(it->filename, "file1");
  BOOST_TEST_EQ(it->file_offset, 0);
  BOOST_TEST_EQ((*it).file_offset, 0);
  ++it;
  BOOST_TEST_EQ((it == manager.end()), true);

  // setup with two elements under one key and one element under another key
  element = hashdb_element_t<T>(k1,
                             4096, "second_rep1", "file1", 0);
  manager.insert(element, changes);
  element = hashdb_element_t<T>(k2,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);

  BOOST_TEST_EQ(manager.find_count(k1), 2);
  BOOST_TEST_EQ(manager.map_size(), 3);

  hashdb_iterator_t<T> it2(manager.begin());
  ++it2;
  it2++;
  ++it2;

  BOOST_TEST_EQ((it2 == manager.end()), true);

  // check iterator pair from find
  std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair;
  it_pair = manager.find(k1);
  ++it_pair.first;
  ++it_pair.first;
  BOOST_TEST_EQ((it_pair.first == it_pair.second), true);
  it_pair = manager.find(k1);
  ++it_pair.first;
  ++it_pair.first;
  BOOST_TEST_EQ((it_pair.first == it_pair.second), true);

  // ************************************************************
  // install lots of data
  // ************************************************************
  // populate with 1,000,000 entries
  T key;
  BOOST_TEST_EQ(manager.map_size(), 3);
  for (uint64_t n=0; n< 1000000; ++n) {
    to_key(n+1000000, key);
    element = hashdb_element_t<T>(key, 4096, "rep1", "file1", 0);
    manager.insert(element, changes);
  }
  BOOST_TEST_EQ(manager.map_size(), 1000003);
}

int cpp_main(int argc, char* argv[]) {

  make_dir_if_not_there(temp_dir);

// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH

  write_settings();
  do_test<hash_t>();

  // done
  int status = boost::report_errors();
  return status;
}

