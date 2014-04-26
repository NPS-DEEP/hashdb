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
 * Provides service for rebuilding the bloom filters
 * including overwriting settings and deleting old bloom filter files
 */

#ifndef BLOOM_REBUILD_MANAGER_HPP
#define BLOOM_REBUILD_MANAGER_HPP
#include "file_modes.h"
#include "bloom_filter_manager.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

template<typename T>
class bloom_rebuild_manager_t {

  private:
  const std::string hashdb_dir;
  hashdb_settings_t settings;
  bloom_filter_manager_t<T> *manager;

  // do not allow copy or assignment
  bloom_rebuild_manager_t(const bloom_rebuild_manager_t&);
  bloom_rebuild_manager_t& operator=(const bloom_rebuild_manager_t&);

  public:
  bloom_rebuild_manager_t(const std::string& p_hashdb_dir,
                          hashdb_settings_t new_bloom_settings) :
                hashdb_dir(p_hashdb_dir),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                manager(0) {

    // replace bloom settings with new changed settings
    settings.bloom1_is_used = new_bloom_settings.bloom1_is_used;
    settings.bloom1_M_hash_size = new_bloom_settings.bloom1_M_hash_size;
    settings.bloom1_k_hash_functions = new_bloom_settings.bloom1_k_hash_functions;
    settings.bloom2_is_used = new_bloom_settings.bloom2_is_used;
    settings.bloom2_M_hash_size = new_bloom_settings.bloom2_M_hash_size;
    settings.bloom2_k_hash_functions = new_bloom_settings.bloom2_k_hash_functions;

    // write back new changed settings
    hashdb_settings_manager_t::replace_settings(hashdb_dir, settings);

    // remove existing bloom files
    std::string filename1 = hashdb_dir + "/bloom_filter_1";
    std::string filename2 = hashdb_dir + "/bloom_filter_2";
    remove(filename1.c_str());
    remove(filename2.c_str());

    // initialize the bloom_filter_manager appropriate for the settings
    manager = new bloom_filter_manager_t<T>(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions);
  }

  ~bloom_rebuild_manager_t() {
    delete manager;
  }

  // add hash value
  void add_hash_value(const T& key) {
    manager->add_hash_value(key);
  }
};

#endif

