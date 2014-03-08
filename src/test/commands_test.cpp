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
#include "commands.hpp"

// map types:
// MAP_BTREE, MAP_FLAT_SORTED_VECTOR, MAP_RED_BLACK_TREE, MAP_UNORDERED_HASH
// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

void rm_hashdb_dir(std::string hashdb_dir) {
  remove((hashdb_dir + "/bloom_filter_1").c_str());
  remove((hashdb_dir + "/bloom_filter_2").c_str());
  remove((hashdb_dir + "/hash_duplicates_store").c_str());
  remove((hashdb_dir + "/hash_store").c_str());
  remove((hashdb_dir + "/history.xml").c_str());
  remove((hashdb_dir + "/log.xml").c_str());
  remove((hashdb_dir + "/settings.xml").c_str());
  remove((hashdb_dir + "/source_filename_store.dat").c_str());
  remove((hashdb_dir + "/source_filename_store.idx1").c_str());
  remove((hashdb_dir + "/source_filename_store.idx2").c_str());
  remove((hashdb_dir + "/source_lookup_store.dat").c_str());
  remove((hashdb_dir + "/source_lookup_store.idx1").c_str());
  remove((hashdb_dir + "/source_lookup_store.idx2").c_str());
  remove((hashdb_dir + "/source_repository_name_store.dat").c_str());
  remove((hashdb_dir + "/source_repository_name_store.idx1").c_str());
  remove((hashdb_dir + "/source_repository_name_store.idx2").c_str());

  if (access(hashdb_dir.c_str(), F_OK) == 0) {
    // dir exists so remove it
    int status = rmdir(hashdb_dir.c_str());
    if (status != 0) {
      std::cout << "failed to remove hashdb_dir " << hashdb_dir << "\n";
    }
  }
}

void do_test() {
  
  // clean up from any previous run
  rm_hashdb_dir("temp_dir1");
  rm_hashdb_dir("temp_dir2");
  rm_hashdb_dir("temp_dir3");
  rm_hashdb_dir("temp_dir4");
  rm_hashdb_dir("temp_dir5");

  // create new hashdbs
  hashdb_settings_t settings;
  settings.bloom1_is_used = false;
  commands_t::create(settings, "temp_dir1");
  commands_t::create(settings, "temp_dir2");
  commands_t::create(settings, "temp_dir3");
  commands_t::create(settings, "temp_dir4");
  commands_t::create(settings, "temp_dir5");

  // import
  commands_t::import("test_repository_name", "sample_dfxml", "temp_dir1");

  // export
  commands_t::do_export("temp_dir1", "temp_dfxml_out");

  // add
  commands_t::add("temp_dir1", "temp_dir1");
  
  // add_multiple
  commands_t::add_multiple("temp_dir1", "temp_dir2", "temp_dir3");

  // intersect
  commands_t::intersect("temp_dir1", "temp_dir3", "temp_dir2");

  // subtract
  commands_t::subtract("temp_dir1", "temp_dir2", "temp_dir3");

  // deduplicate
  commands_t::deduplicate("temp_dir1", "temp_dir4");

  // rebuild_bloom
  commands_t::rebuild_bloom(settings, "temp_dir1");

  // server
  // not tested

  // scan
  commands_t::scan("temp_dir5", "sample_dfxml");

  // expand_identified_blocks
  commands_t::expand_identified_blocks("temp_dir5", "identified_blocks.txt");

  // get_sources
  commands_t::get_sources("temp_dir5");

  // get_statistics
  commands_t::get_statistics("temp_dir5");
}

int cpp_main(int argc, char* argv[]) {
  do_test();
  return 0;
}

