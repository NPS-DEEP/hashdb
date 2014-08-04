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
 * Test the identified_blocks.txt reader.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "identified_blocks_reader_iterator.hpp"
#include "identified_blocks_reader.hpp"

static const std::string identified_blocks(DATADIR "identified_blocks.txt");

void do_test() {
  identified_blocks_reader_t reader(identified_blocks);
  identified_blocks_reader_iterator_t it = reader.begin();
  BOOST_TEST_EQ(it->first, "10485760");
  BOOST_TEST_EQ(it->second, "3b6b477d391f73f67c1c01e2141dbb17");

  for (int i=0; i<16; ++i) {
    // the first of 16 is validated above, but all may be inspected
    // std::cout << it->first + ", " + it->second + "\n";
    ++it;
  }
  BOOST_TEST_EQ((it == reader.end()), true);
}
  
int cpp_main(int argc, char* argv[]) {
  do_test();

  // done
  int status = boost::report_errors();
  return status;
}

