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

uint64_t encode(uint64_t index, uint64_t offset) {
  return index<<34 | offset;
}

void test_encoding(uint64_t index, uint64_t offset, uint64_t encoding) {
  std::cout << "test_encoding index: " << index
            << ", offset: " << offset
            << ", encoding: " << encoding << "\n";
  uint64_t temp_index = source_lookup_encoding::get_source_lookup_index(encoding);
  uint64_t temp_offset = source_lookup_encoding::get_file_offset(encoding);
  uint64_t temp_encoding = source_lookup_encoding::get_source_lookup_encoding(index, offset);
  BOOST_TEST_EQ(index, temp_index);
  BOOST_TEST_EQ(offset, temp_offset);
  BOOST_TEST_EQ(encoding, temp_encoding);
}

int cpp_main(int argc, char* argv[]) {

  // make all std::cout diagnostics be in hex
  std::cout << std::hex;

  // max values
  uint64_t max_index = ((uint64_t)1<<30) - 1;
  uint64_t max_offset = (((uint64_t)1<<34) - 1) * HASHDB_BYTE_ALIGNMENT;

  // test encodings
  // valid
  test_encoding(0, 0, 0);

  // valid
  test_encoding(1, 2*HASHDB_BYTE_ALIGNMENT, (uint64_t)1<<34 | (uint64_t)2);

  test_encoding(max_index, max_offset, max_index<<34 | max_offset/HASHDB_BYTE_ALIGNMENT);

  // source lookup index too large
  BOOST_TEST_THROWS(test_encoding(max_index+1, max_offset, (max_index+1)<<34 | (max_offset/HASHDB_BYTE_ALIGNMENT)), std::runtime_error);
  BOOST_TEST_THROWS(test_encoding(max_index, max_offset+1, (max_index<<34) | ((max_offset/HASHDB_BYTE_ALIGNMENT)+max_offset)), std::runtime_error);

  // done
  int status = boost::report_errors();
  return status;
}

