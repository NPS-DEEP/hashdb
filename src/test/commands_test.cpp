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
#include "directory_helper.hpp"
#include "commands.hpp"
#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

void do_test1() {
  
  // clean up from any previous run
  rm_hashdb_dir("temp_dir1");
  rm_hashdb_dir("temp_dir2");
  rm_hashdb_dir("temp_dir3");
  rm_hashdb_dir("temp_dir4");
  rm_hashdb_dir("temp_dir5");

  // create new hashdbs
  hashdb_settings_t settings;
  settings.bloom1_is_used = false;
  commands_t<md5_t>::create(settings, "temp_dir1");
  commands_t<md5_t>::create(settings, "temp_dir2");
  commands_t<md5_t>::create(settings, "temp_dir3");
  commands_t<md5_t>::create(settings, "temp_dir4");
  commands_t<md5_t>::create(settings, "temp_dir5");

  // import
  commands_t<md5_t>::import("test_repository_name", "sample_dfxml", "temp_dir1");

  // export
  commands_t<md5_t>::do_export("temp_dir1", "temp_dfxml_out");

  // add
  commands_t<md5_t>::add("temp_dir1", "temp_dir1");
  
  // add_multiple
  commands_t<md5_t>::add_multiple("temp_dir1", "temp_dir2", "temp_dir3");

  // intersect
  commands_t<md5_t>::intersect("temp_dir1", "temp_dir3", "temp_dir2");

  // subtract
  commands_t<md5_t>::subtract("temp_dir1", "temp_dir2", "temp_dir3");

  // deduplicate
  commands_t<md5_t>::deduplicate("temp_dir1", "temp_dir4");

  // rebuild_bloom
  commands_t<md5_t>::rebuild_bloom(settings, "temp_dir1");

  // server
  // not tested

  // scan
  commands_t<md5_t>::scan("temp_dir5", "sample_dfxml");

  // expand_identified_blocks
  commands_t<md5_t>::expand_identified_blocks("temp_dir5", "identified_blocks.txt");

  // sources
  commands_t<md5_t>::sources("temp_dir5");

  // statistics
  commands_t<md5_t>::statistics("temp_dir5");
}

void do_test2() {
  
  // clean up from any previous run
  rm_hashdb_dir("temp_dir");

  // create new hashdb
  hashdb_settings_t settings;
  commands_t<md5_t>::create(settings, "temp_dir");

  // test ability to manage many repository names
  for (int i=0; i<2500; i++) {
    // generate unique repository name
    std::ostringstream ss;
    ss << "test_repository_name_" << i;

    // import
    commands_t<md5_t>::import(ss.str(), "sample_dfxml", "temp_dir");
  }

  // size
  commands_t<md5_t>::size("temp_dir");

  // sources
  commands_t<md5_t>::sources("temp_dir");

  // statistics
  commands_t<md5_t>::statistics("temp_dir");
}

int cpp_main(int argc, char* argv[]) {
#ifdef HAVE_MCHECK
  mtrace();
#endif
  std::cout << "test1\n";
  do_test1();
  std::cout << "test2\n";
  do_test2();
#ifdef HAVE_MCHECK
  muntrace();
#endif
  return 0;
}

