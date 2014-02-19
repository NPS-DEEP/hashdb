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
 * Provides access to the map manager without concern for the multimap,
 * useful for iterating over hashdigests rather than over fully qualified
 * hashdb_element data.
 */

#ifndef HASHDB_MAP_ONLY_MANAGER_HPP
#define HASHDB_MAP_ONLY_MANAGER_HPP
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "map_btree.hpp"
#include "map_flat_sorted_vector.hpp"
#include "map_red_black_tree.hpp"
#include "map_unordered_hash.hpp"
#include "file_modes.h"
#include "map_types.h"
#include "map_iterator.hpp"
#include "map_manager.hpp"
#include "hashdb_map_only_iterator.hpp"


class hashdb_map_only_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;

  map_manager_t<md5_t>* md5_map_manager;
  map_manager_t<sha1_t>* sha1_map_manager;
  map_manager_t<sha256_t>* sha256_map_manager;

  // do not allow copy or assignment
  hashdb_map_only_manager_t(const hashdb_map_only_manager_t&);
  hashdb_map_only_manager_t& operator=(const hashdb_map_only_manager_t&);

  public:
  hashdb_map_only_manager_t(const std::string& p_hashdb_dir,
                            file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                md5_map_manager(0),
                sha1_map_manager(0),
                sha256_map_manager(0) {

    // initialize the map_multimap_manager appropriate for the settings
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_map_manager = new map_manager_t<md5_t>(hashdb_dir, file_mode, settings.map_type);
        return;
      case HASHDIGEST_SHA1:
        sha1_map_manager = new map_manager_t<sha1_t>(hashdb_dir, file_mode, settings.map_type);
        return;
      case HASHDIGEST_SHA256:
        sha256_map_manager = new map_manager_t<sha256_t>(hashdb_dir, file_mode, settings.map_type);
        return;
      default: assert(0);
    }
  }

  ~hashdb_map_only_manager_t() {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5: delete md5_map_manager; return;
      case HASHDIGEST_SHA1: delete sha1_map_manager; return;
      case HASHDIGEST_SHA256: delete sha256_map_manager; return;
      default: assert(0);
    }
  }

  // begin
  hashdb_map_only_iterator_t begin() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdb_map_only_iterator_t(md5_map_manager->begin());
      case HASHDIGEST_SHA1:
        return hashdb_map_only_iterator_t(sha1_map_manager->begin());
      case HASHDIGEST_SHA256:
        return hashdb_map_only_iterator_t(sha256_map_manager->begin());
      default: assert(0);
    }
  }

  // end
  hashdb_map_only_iterator_t end() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdb_map_only_iterator_t(md5_map_manager->end());
      case HASHDIGEST_SHA1:
        return hashdb_map_only_iterator_t(sha1_map_manager->end());
      case HASHDIGEST_SHA256:
        return hashdb_map_only_iterator_t(sha256_map_manager->end());
      default: assert(0);
    }
  }
};

#endif


