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
#include "hashdigest_types.h"
#include "bloom_filter_manager.hpp"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "hashdigest_types.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

class bloom_rebuild_manager_t {
  private:
  const std::string hashdb_dir;
  hashdb_settings_t settings;

  bloom_filter_manager_t<md5_t>* md5_manager;
  bloom_filter_manager_t<sha1_t>* sha1_manager;
  bloom_filter_manager_t<sha256_t>* sha256_manager;

  // do not allow copy or assignment
  bloom_rebuild_manager_t(const bloom_rebuild_manager_t&);
  bloom_rebuild_manager_t& operator=(const bloom_rebuild_manager_t&);

  public:
  bloom_rebuild_manager_t(const std::string& p_hashdb_dir,
                          hashdb_settings_t new_bloom_settings) :
                hashdb_dir(p_hashdb_dir),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                md5_manager(0),
                sha1_manager(0),
                sha256_manager(0) {

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
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager = new bloom_filter_manager_t<md5_t>(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions);
 
        return;
      case HASHDIGEST_SHA1:
        sha1_manager = new bloom_filter_manager_t<sha1_t>(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions);

        return;
      case HASHDIGEST_SHA256:
        sha256_manager = new bloom_filter_manager_t<sha256_t>(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions);

        return;
      default: assert(0);
    }
  }

  ~bloom_rebuild_manager_t() {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5: delete md5_manager; return;
      case HASHDIGEST_SHA1: delete sha1_manager; return;
      case HASHDIGEST_SHA256: delete sha256_manager; return;
      default: assert(0);
    }
  }

  // add hash value
  void add_hash_value(const hashdigest_t& hashdigest) {

    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager->add_hash_value(md5_t::fromhex(hashdigest.hashdigest));
        return;
      case HASHDIGEST_SHA1:
        sha1_manager->add_hash_value(sha1_t::fromhex(hashdigest.hashdigest));
        return;
      case HASHDIGEST_SHA256:
        sha256_manager->add_hash_value(sha256_t::fromhex(hashdigest.hashdigest));
        return;
      default: assert(0);
    }
  }
};

#endif

