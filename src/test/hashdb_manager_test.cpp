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
 * Test the map_multimap_manager.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "to_key_helper.hpp"
#include "settings.hpp"
#include "settings_manager.hpp"
#include "map_types.h"
#include "multimap_types.h"
#include "hashdigest_types.h"
#include "file_modes.h"
#include "hashdb_manager.hpp"
#include "hashdb_iterator.hpp"
#include "hashdb_element.hpp"
#include "dfxml/src/hash_t.h"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir";
//static const char temp_hash_store[] = "temp_dir/hash_store";
static const char temp_bloom_filter_1[] = "temp_dir/bloom_filter_1";

void do_test(map_type_t map_type, multimap_type_t multimap_type) {

  // valid sha1 values
  sha1_t k1;
  sha1_t k2;
  sha1_t k3;
  to_key(1, k1);
  to_key(2, k2);
  to_key(3, k3);
//  const uint64_t v1 = 111;
//  const uint64_t v2 = 222;
//  const uint64_t v3 = 333;
  hashdigest_t d1(k1);
  hashdigest_t d2(k2);
  hashdigest_t d3(k3);

  // md5 will be invalid since hashdb expects sha1
  md5_t k1_md5;
  to_key(1, k1_md5);
  hashdigest_t d1_md5(k1_md5);

  hashdb_element_t element;

  // clean up from any previous run
//  remove(temp_hash_store);
  remove(temp_bloom_filter_1);

  // create working settings
  settings_t settings;
  settings.hashdigest_type = HASHDIGEST_SHA1;
  settings.map_type = MAP_BTREE;
  settings.multimap_type = MULTIMAP_BTREE;
  settings.maximum_hash_duplicates = 2;
  settings_manager_t::write_settings(temp_dir, settings);

  // create working changes object
  hashdb_changes_t changes;

  // open new hashdb manager
  hashdb_manager_t manager(temp_dir, RW_NEW);

  // ************************************************************
  // initial state
  // ************************************************************
  // check initial size
  BOOST_TEST_EQ(manager.map_size(), 0);
  BOOST_TEST_EQ(manager.multimap_size(), 0);

  // check initial iterator
  BOOST_TEST_EQ((manager.begin() == manager.end()), true);

  // ************************************************************
  // insert, remove, and hashdb_changes variables
  // note: some of these tests additionally test failure ordering
  // ************************************************************
  // insert valid
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 1);

  // insert, wrong hash block size
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             5, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_wrong_hash_block_size, 1);

  // insert, file offset not aligned
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 5);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_file_offset_not_aligned, 1);

  // insert, wrong hashdigest type
  element = hashdb_element_t(d1_md5.hashdigest, d1_md5.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_wrong_hashdigest_type, 1);

  // insert, add second valid
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 4096);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 2);

  // insert, exceeds max duplicates
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 4096 * 2);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_exceeds_max_duplicates, 1);

  // insert, duplicate element
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_inserted_duplicate_element, 1);

  // remove, no element
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "undefined_rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 1);

  // remove successfully
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 1);

  // remove, no element
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 2);

  // insert, valid
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(changes.hashes_inserted, 3);

  // remove_key successfully
  BOOST_TEST_EQ(changes.hashes_removed, 1);
  manager.remove_key(d1, changes);
  BOOST_TEST_EQ(changes.hashes_removed, 3);

  // remove_key no hash
  manager.remove_key(d1, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_hash, 1);

  // remove, wrong hash block size
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             5, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_wrong_hash_block_size, 1);

  // remove, file offset not aligned
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 5);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_file_offset_not_aligned, 1);

  // remove, wrong hashdigest type
  element = hashdb_element_t(d1_md5.hashdigest, d1_md5.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_wrong_hashdigest_type, 1);

  // remove, no element
  element = hashdb_element_t(d2.hashdigest, d2.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.remove(element, changes);
  BOOST_TEST_EQ(changes.hashes_not_removed_no_element, 3);

  // ************************************************************
  // has_key, size, iterator
  // ************************************************************
  BOOST_TEST_EQ(manager.has_key(d1), false);
  BOOST_TEST_EQ(manager.has_key(d1_md5), false);

  // setup with one element to make iterator simple
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);
  BOOST_TEST_EQ(manager.map_size(), 1);
  BOOST_TEST_EQ(manager.multimap_size(), 0);
  hashdb_iterator_t it(manager.begin());
  BOOST_TEST_EQ(it->hashdigest, "0000000000000000000000000000000000000001");
  BOOST_TEST_EQ(it->hashdigest_type, "SHA1");
  BOOST_TEST_EQ(it->hash_block_size, 4096);
  BOOST_TEST_EQ(it->repository_name, "rep1");
  BOOST_TEST_EQ(it->filename, "file1");
  BOOST_TEST_EQ(it->file_offset, 0);
  BOOST_TEST_EQ((*it).file_offset, 0);
  ++it;
  BOOST_TEST_EQ((it == manager.end()), true);


  // setup with two elements under one key and one element under another key
  element = hashdb_element_t(d1.hashdigest, d1.hashdigest_type,
                             4096, "second_rep1", "file1", 0);
  manager.insert(element, changes);
  element = hashdb_element_t(d2.hashdigest, d2.hashdigest_type,
                             4096, "rep1", "file1", 0);
  manager.insert(element, changes);

  BOOST_TEST_EQ(manager.has_key(d1), true);
  BOOST_TEST_EQ(manager.map_size(), 2);
  BOOST_TEST_EQ(manager.multimap_size(), 2);

  hashdb_iterator_t it2(manager.begin());
  ++it2;
  it2++;
  ++it2;

  BOOST_TEST_EQ((it2 == manager.end()), true);
}

int cpp_main(int argc, char* argv[]) {
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH

  do_test(MAP_BTREE, MULTIMAP_BTREE);
  do_test(MAP_FLAT_SORTED_VECTOR, MULTIMAP_FLAT_SORTED_VECTOR);
  do_test(MAP_RED_BLACK_TREE, MULTIMAP_RED_BLACK_TREE);
  do_test(MAP_UNORDERED_HASH, MULTIMAP_UNORDERED_HASH);
  do_test(MAP_BTREE, MULTIMAP_BTREE);

  // done
  int status = boost::report_errors();
  return status;
}

