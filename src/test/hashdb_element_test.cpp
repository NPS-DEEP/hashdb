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
 * Test hashdb_element_t constructors.
 */

#include <config.h>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdb_element.hpp"

void run_test() {

  hashdb_element_t element;
  BOOST_TEST_EQ(element.block_size, 0);
//  element("7", "MD5_misspelled", 2, "rep", "file", 3);
//  BOOST_TEST_EQ(element.block_size, 2);
  md5_t temp;
  element = hashdb_element_t(temp);
  BOOST_TEST_EQ(element.hashdigest_type, "MD5");
  sha1_t temp2;
  element = hashdb_element_t(temp2);
  BOOST_TEST_EQ(element.hashdigest_type, "SHA1");
  sha256_t temp3;
  element = hashdb_element_t(temp3);
  BOOST_TEST_EQ(element.hashdigest_type, "SHA256");
}

int cpp_main(int argc, char* argv[]) {
  run_test();

  // done
  int status = boost::report_errors();
  return status;
}

