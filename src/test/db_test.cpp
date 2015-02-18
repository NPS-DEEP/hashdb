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
#include "lmdb_hash_store.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_source_it_data.hpp"
#include "lmdb_helper.h"
#include "directory_helper.hpp"
#include "file_modes.h"

// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

static const char temp_dir[] = "temp_dir_db_test.hdb";
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

void rw_new() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  // create each db
  lmdb_hash_store_t hash_store(temp_dir, RW_NEW, 512, 0);
  lmdb_name_store_t name_store(temp_dir, RW_NEW);
  lmdb_source_store_t source_store(temp_dir, RW_NEW);
}

void rw_modify_hash_store() {
  // open db
  lmdb_hash_store_t hash_store(temp_dir, RW_MODIFY, 512, 0);

  // exercise hash store interfaces
  lmdb_hash_it_data_t hash_it_data;
  hash_store.insert(binary_aa, 1, 2*4096);
  hash_store.insert(binary_aa, 3, 4*4096);
  hash_store.insert(binary_aa, 5, 6*4096);
  hash_store.insert(binary_aa, 7, 8*4096);
  hash_store.insert(binary_bb, 1, 2*4096);
  hash_store.insert(binary_bb, 3, 4*4096);
  hash_store.insert(binary_bb, 5, 6*4096);
  hash_store.insert(binary_bb, 7, 8*4096);
  hash_store.insert(binary_cc, 1, 2*4096);
  hash_store.insert(binary_big, 1, 2*4096);
  hash_store.insert(binary_big, 1, 42*4096);

  TEST_EQ(hash_store.size(), 11);
  TEST_EQ(hash_store.find(binary_aa, 1, 2*4096), true);
  TEST_EQ(hash_store.find(binary_aa, 9, 2*4096), false);
  TEST_EQ(hash_store.find(binary_aa, 1, 9*4096), false);
  TEST_EQ(hash_store.find(binary_cc, 1, 2*4096), true);
  TEST_EQ(hash_store.find_count(binary_aa), 4);
  TEST_EQ(hash_store.find_count(binary_cc), 1);

  hash_it_data = hash_store.find_first(binary_aa);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 1, 2*4096, true));
  hash_store.find_begin();
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 1, 2*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 3, 4*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 5, 6*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 7, 8*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_bb, 1, 2*4096, true));


  hash_it_data = hash_store.find_first(binary_cc);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_cc, 1, 2*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data.is_valid, false);

  TEST_EQ(hash_store.erase(binary_aa, 3, 4*4096), true);
  TEST_EQ(hash_store.erase(binary_aa, 3, 4*4096), false);
  TEST_EQ(hash_store.size(), 10);

  TEST_EQ(hash_store.erase(binary_big), 2);
  TEST_EQ(hash_store.size(), 8);
}

void read_only_hash_store() {
  // open db
  lmdb_hash_store_t hash_store(temp_dir, READ_ONLY, 512, 0);

  TEST_EQ(hash_store.size(), 8);
  TEST_EQ(hash_store.find(binary_aa, 1, 2*4096), true);
  TEST_EQ(hash_store.find(binary_aa, 9, 2*4096), false);
  TEST_EQ(hash_store.find(binary_aa, 1, 9*4096), false);
  TEST_EQ(hash_store.find(binary_cc, 1, 2*4096), true);
  TEST_EQ(hash_store.find_count(binary_aa), 3);
  TEST_EQ(hash_store.find_count(binary_cc), 1);

  lmdb_hash_it_data_t hash_it_data;
  hash_it_data = hash_store.find_first(binary_aa);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 1, 2*4096, true));
  TEST_EQ(hash_store.find_begin(), hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 1, 2*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_aa, 5, 6*4096, true));
  hash_it_data = hash_store.find_next(hash_it_data);
  hash_it_data = hash_store.find_next(hash_it_data);
  TEST_EQ(hash_it_data, lmdb_hash_it_data_t(binary_bb, 1, 2*4096, true));
}

void rw_modify_name_store() {
  // open db
  lmdb_name_store_t name_store(temp_dir, RW_MODIFY);

  // exercise RW interfaces
  std::pair<bool,uint64_t> pair;
  pair = name_store.find("a","b");
  TEST_EQ(pair.first, false);
  pair = name_store.insert("a","b");
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);
  pair = name_store.find("a","b");
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 1);
  pair = name_store.insert("long repository name","long filename");
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 2);
  pair = name_store.find("long repository name","long filename");
  TEST_EQ(pair.first, true);
  TEST_EQ(pair.second, 2);
  pair = name_store.find("long repository namel","ong filename");
  TEST_EQ(pair.first, false);
  pair = name_store.find("long repository name","ong filename");
  TEST_EQ(pair.first, false);
  pair = name_store.find("long repository nam","long filename");
  TEST_EQ(pair.first, false);
  pair = name_store.find("long repository nzzz","long filenzzz");
  TEST_EQ(pair.first, false);
  TEST_EQ(name_store.size(), 2);
  pair = name_store.insert("a","b");
}

void read_only_name_store() {
  // open db
  lmdb_name_store_t name_store(temp_dir, READ_ONLY);

  // exercise interfaces
  std::pair<bool,uint64_t> pair;
  pair = name_store.find("a","b");
  TEST_EQ(pair.first, true);
  pair = name_store.find("long repository name","long filename");
  TEST_EQ(pair.first, true);
  pair = name_store.find("long repository nzzz","long filenzzz");
  TEST_EQ(pair.first, false);
  TEST_EQ(name_store.size(), 2);
}

void rw_modify_source_store() {
  // open db
  lmdb_source_store_t source_store(temp_dir, RW_MODIFY);

  // checks while size is zero
  lmdb_source_data_t data;
  lmdb_source_it_data_t it_data;
  TEST_EQ(source_store.size(), 0);

  it_data = source_store.find_first();
  TEST_EQ(it_data.is_valid, false);

  // check encoding and decoding
  lmdb_source_data_t data0("r2", "fn3", 4, "hash5");
  TEST_EQ(source_store.add(0, data0), true);
  lmdb_source_data_t data0b = source_store.find(0);
  TEST_EQ(data0b.repository_name, "r2");
  TEST_EQ(data0b.filename, "fn3");
  TEST_EQ(data0b.filesize, 4);
  TEST_EQ(data0b.binary_hash, "hash5");

  // check add
  TEST_EQ(source_store.add(1, data), true);
  TEST_EQ(source_store.add(1, data), false);
  data.repository_name = "rn";
  TEST_EQ(source_store.add(1, data), true);
  TEST_EQ(source_store.add(1, data), false);
  data.filename = "fn";
  TEST_EQ(source_store.add(1, data), true);
  TEST_EQ(source_store.add(1, data), false);
  data.filesize = 20;
  TEST_EQ(source_store.add(1, data), true);
  TEST_EQ(source_store.add(1, data), false);
  data.binary_hash = "yy";
  TEST_EQ(source_store.add(1, data), true);
  TEST_EQ(source_store.add(1, data), false);

  TEST_EQ(source_store.size(), 2);

  data.repository_name = "repository_name";
  data.filename = "filename";
  data.filesize = 30;
  data.binary_hash = "some hash digest";
  TEST_EQ(source_store.add(2, data), true);

  data = source_store.find(1);
  TEST_EQ(data.repository_name, "rn");
  it_data = source_store.find_first();
  TEST_EQ(it_data.source_data.repository_name, "r2");
  TEST_EQ(it_data.is_valid, true);
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.source_data.repository_name, "rn");
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.source_data.repository_name, "repository_name");
  TEST_EQ(it_data.is_valid, true);
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.is_valid, false);
}

void read_only_source_store() {
  // open db
  lmdb_source_store_t source_store(temp_dir, READ_ONLY);

  lmdb_source_data_t data;
  lmdb_source_it_data_t it_data;
  it_data = source_store.find_first();
  TEST_EQ(it_data.source_data.repository_name, "r2");
  TEST_EQ(it_data.is_valid, true);
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.source_data.repository_name, "rn");
  TEST_EQ(it_data.is_valid, true);
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.source_data.repository_name, "repository_name");
  TEST_EQ(it_data.is_valid, true);
  it_data = source_store.find_next(it_data.source_lookup_index);
  TEST_EQ(it_data.is_valid, false);
}

int main(int argc, char* argv[]) {

  make_dir_if_not_there(temp_dir);

  // new
  std::cout << "rw_new_tests ...\n";
  rw_new();

  // hash store
  std::cout << "rw_modify_hash_store ...\n";
  rw_modify_hash_store();
  std::cout << "read_only_hash_store ...\n";
  read_only_hash_store();

  // name store
  std::cout << "rw_modify_name_store ...\n";
  rw_modify_name_store();
  std::cout << "read_only_name_store ...\n";
  read_only_name_store();

  // source store
  std::cout << "rw_modify_source_store ...\n";
  rw_modify_source_store();
  std::cout << "read_only_source_store ...\n";
  read_only_source_store();




  std::cout << "db_test Done.\n";
  return 0;
}

