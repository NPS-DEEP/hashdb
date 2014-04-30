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
 * Test the bloom filter manager
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
#include "bloom_filter_manager.hpp"
#include "dfxml/src/hash_t.h"
#include "file_modes.h"

static const char temp_dir[] = "temp_dir";
static const char temp_bloom1[] = "temp_dir/bloom_filter_1";
static const char temp_bloom2[] = "temp_dir/bloom_filter_2";

template<typename T>
void run_rw_tests(std::string& hashdb_dir,
                  file_mode_type_t file_mode,
                  bool bloom1_is_used,
                  uint32_t bloom1_M_hash_size,
                  uint32_t bloom1_k_hash_functions,
                  bool bloom2_is_used,
                  uint32_t bloom2_M_hash_size,
                  uint32_t bloom2_k_hash_functions) {

  T key;

  remove(temp_bloom1);
  remove(temp_bloom2);

  bloom_filter_manager_t<T> bloom(hashdb_dir, file_mode,
                  bloom1_is_used, bloom1_M_hash_size, bloom1_k_hash_functions,
                  bloom2_is_used, bloom2_M_hash_size, bloom2_k_hash_functions);

  to_key(101, key);

  // key should not be there yet
  BOOST_TEST_EQ(bloom.is_positive(key), false);

  // add and retest
  bloom.add_hash_value(key);
  BOOST_TEST_EQ(bloom.is_positive(key), true);
}

int cpp_main(int argc, char* argv[]) {
  static std::string temp_dir_string("temp_dir");
  make_dir_if_not_there(temp_dir);

//std::cout << "bfmt.a\n";
  run_rw_tests<md5_t>(temp_dir_string, RW_NEW, true, 28, 2, false, 28, 2);
//std::cout << "bfmt.b\n";
  run_rw_tests<md5_t>(temp_dir_string, RW_NEW, false, 28, 2, true, 28, 2);
//std::cout << "bfmt.c\n";
/*
  run_rw_tests<sha1_t>(temp_dir_string, RW_NEW, true, 28, 2, false, 28, 2);
//std::cout << "bfmt.d\n";
  run_rw_tests<sha1_t>(temp_dir_string, RW_NEW, false, 28, 2, true, 28, 2);
//std::cout << "bfmt.e\n";
  run_rw_tests<sha256_t>(temp_dir_string, RW_NEW, true, 28, 2, false, 28, 2);
//std::cout << "bfmt.f\n";
  run_rw_tests<sha256_t>(temp_dir_string, RW_NEW, false, 28, 2, true, 28, 2);
//std::cout << "bfmt.g\n";
*/

  // done
  int status = boost::report_errors();
  return status;
}

