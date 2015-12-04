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
 * Test the hashdb managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "hashdb_create_manager.hpp"
#include "hashdb_import_manager.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_changes.hpp"
#include "file_helper.hpp"
#include "directory_helper.hpp"

static const std::string hashdb_dir = "temp_dir_hashdb_managers_test.hdb";
static const std::string binary_0(lmdb_helper::hex_to_binary_hash("00"));
static const std::string binary_aa(lmdb_helper::hex_to_binary_hash("aa"));
static const std::string binary_bb(lmdb_helper::hex_to_binary_hash("bb"));
static const std::string binary_cc(lmdb_helper::hex_to_binary_hash("cc"));
static const std::string binary_ff(lmdb_helper::hex_to_binary_hash("ff"));
//static const std::string binary_big(lmdb_helper::hex_to_binary_hash("0123456789abcdef2123456789abcdef"));

// ************************************************************
// hashdb_create_manager
// ************************************************************
void test_create_manager() {
  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // write default settings
  hashdb_settings_t settings;

  // create the hashdb directory
  bool is_new;
  is_new = hashdb_create_manager::create_if_new(hashdb_dir, settings);
  TEST_EQ(is_new, true);
  is_new = hashdb_create_manager::create_if_new(hashdb_dir, settings);
  TEST_EQ(is_new, false);
}

// ************************************************************
// hashdb_import_manager
// ************************************************************
// no whitelist, no import low entropy
void test_import_manager1() {
  // remove any previous hashdb_dir
  rm_hashdb_dir(hashdb_dir);

  // write default settings
  hashdb_settings_t settings;

  // create the hashdb directory
  bool is_new;
  is_new = hashdb_create_manager::create_if_new(hashdb_dir, settings);
  TEST_EQ(is_new, true);

  hashdb_import_manager_t manager(hashdb_dir, "", false);

  // import data
  //TBD
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // hashdb_create_manager
  test_create_manager();

  // import, no whitelist, do not skip low entropy
  test_import_manager1();

  // done
  std::cout << "hashdb_managers_test Done.\n";
  return 0;
}

