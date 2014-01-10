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
 * Test the red-black tree map.
 */

#include "map_red_black_tree.hpp"
#include <iostream>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "boost_fix.hpp"
#include "hashdb_types.h"
#include "map_red_black_tree.hpp"

int cpp_main(int argc, char* argv[]) {

  // for now, just get map_red_black_tree.hpp to compile.  Then add tests.
  map *map_red_black_tree_t<uint8_t[16], uint64_t>
     = new map *map_red_black_tree_t<uint8_t[16], uint64_t>
       ("temp_rbtree", "rbtree", 10000, 10000, RW_NEW);
  delete map;
  std::cout << "Done.\n";
}

