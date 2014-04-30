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
 * Test the source lookup encoding module.
 */

#include <config.h>
#include "source_lookup_encoding.hpp"
#include <iostream>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"

int cpp_main(int argc, char* argv[]) {

  // let any std::cout diagnostics be in hex
  std::cout << std::hex;

  // 32 bits, small values
  uint64_t temp = 0xffffffffffffffff;
  temp = source_lookup_encoding::get_source_lookup_encoding(2, 3);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(temp), 2);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(temp), 3);

  // 32 bits, large value for source_lookup_index
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(0xfffffffe, 4);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(temp), 0xfffffffe);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(temp), 4);

  // 32 bits, large value for hash_block_offset
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(5, 0xfffffffe);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(temp), 5);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(temp), 0xfffffffe);

  // 32 bits, invalid values
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(0xffffffff, 0), std::runtime_error);
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(0, 0xffffffff), std::runtime_error);

  // check count functionality
  temp = source_lookup_encoding::get_source_lookup_encoding(2);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 2);

  // done
  int status = boost::report_errors();
  return status;
}

