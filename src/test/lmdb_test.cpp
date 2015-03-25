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
 * Test ability of LMDB to detect map full condition.
 * Also serves as a clean example of how LMDB is used.
 */

#include "lmdb.h"
#include "unistd.h"
#include <sys/stat.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <cassert>
 
static const int env_flags = MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP;

// generate a random key
std::string random_key() {
  char hash[16];
  for (size_t i=0; i<16; i++) {
    hash[i]=(static_cast<char>(rand()));
  }
  return std::string(hash, 16);
}

// DB element size
inline static size_t size(MDB_env* env) {

  // obtain statistics
  MDB_stat stat;
  int rc = mdb_env_stat(env, &stat);
  if (rc != 0) {
    // program error
    std::cerr << "size failure: " << mdb_strerror(rc) << "\n";
    assert(0);
  }
  return stat.ms_entries;
}

void create_new_db() {
  // remove existing DB files, if present, and create temp_dir_lmdb_test for test
  remove("temp_dir_lmdb_test/data.mdb");
  remove("temp_dir_lmdb_test/lock.mdb");
  rmdir("temp_dir_lmdb_test");
#ifdef _WIN32
  if(mkdir("temp_dir_lmdb_test")) {
    assert(0);
  }
#else
  if(mkdir("temp_dir_lmdb_test",0777)){
    assert(0);
  }
#endif

  MDB_env* env;
  int rc = mdb_env_create(&env);
  if (rc != 0) {
    assert(0);
  }
  rc = mdb_env_open(env, "temp_dir_lmdb_test", env_flags, 0664);
  if (rc != 0) {
    assert(0);
  }
  mdb_env_close(env);
}

bool test_db() {
  // run DB test to show that the mdb_put operation breaks when map is full
  MDB_env* env;
  int rc = mdb_env_create(&env);
  if (rc != 0) {
    assert(0);
  }
  rc = mdb_env_open(env, "temp_dir_lmdb_test", env_flags, 0664);
  if (rc != 0) {
    assert(0);
  }

  // flags
  int txn_flags = 0;
  int dbi_flags = MDB_CREATE | MDB_DUPSORT;


  // add until transaction failure
  for (int i=1; i< 100000; i++) {

    // resources
    MDB_txn* txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;

    // create txn object
    rc = mdb_txn_begin(env, NULL, txn_flags, &txn);
    if (rc != 0) {
      assert(0);
    }

    // create the database handle integer
    rc = mdb_dbi_open(txn, NULL, dbi_flags, &dbi);
    if (rc != 0) {
      assert(0);
    }

    // prepare the element
    std::string key_string = random_key();
    key.mv_size = key_string.size();
    key.mv_data = static_cast<void*>(const_cast<char*>(key_string.c_str()));
    const std::string data_string("some bytes of value data");
    data.mv_size = data_string.size();
    data.mv_data = static_cast<void*>(const_cast<char*>(data_string.c_str()));
 
    // add the element
    rc = mdb_put(txn, dbi, &key, &data, MDB_NODUPDATA);
    if (rc != 0) {
      assert(0);
    }

    // commit and close txn object
    rc = mdb_txn_commit(txn);
    if (rc != 0) {
      std::cerr << "LMDB txn commit failure, as expected, at entry " << i << ": " << mdb_strerror(rc) << "\n";
      return true;
    }
  }
  return false;
}

int main(int argc, char* argv[]) {

  create_new_db();
  bool success = test_db();
  return((success) ? 0 : 1);
}

