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
 * Provides iterator accessors for iterating over just the map manager.
 */

#ifndef HASHDIGEST_MANAGER_HPP
#define HASHDIGEST_MANAGER_HPP
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "hashdigest_types.h"
#include "map_types.h"
#include "map_manager.hpp"
#include "map_iterator.hpp"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "hashdigest_types.h"
#include "hashdigest_iterator.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

class hashdigest_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;

  map_manager_t<md5_t>* md5_manager;
  map_manager_t<sha1_t>* sha1_manager;
  map_manager_t<sha256_t>* sha256_manager;

  // do not allow copy or assignment
  hashdigest_manager_t(const hashdigest_manager_t&);
  hashdigest_manager_t& operator=(const hashdigest_manager_t&);

  public:
  hashdigest_manager_t(const std::string& p_hashdb_dir,
                       file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                md5_manager(0),
                sha1_manager(0),
                sha256_manager(0) {

    // initialize the map_multimap_manager appropriate for the settings
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager = new map_manager_t<md5_t>(
                          hashdb_dir, file_mode, settings.map_type);
        return;
      case HASHDIGEST_SHA1:
        sha1_manager = new map_manager_t<sha1_t>(
                          hashdb_dir, file_mode, settings.map_type);
        return;
      case HASHDIGEST_SHA256:
        sha256_manager = new map_manager_t<sha256_t>(
                          hashdb_dir, file_mode, settings.map_type);
        return;
      default: assert(0);
    }
  }

  ~hashdigest_manager_t() {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5: delete md5_manager; return;
      case HASHDIGEST_SHA1: delete sha1_manager; return;
      case HASHDIGEST_SHA256: delete sha256_manager; return;
      default: assert(0);
    }
  }

  // begin
  hashdigest_iterator_t begin() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdigest_iterator_t(md5_manager->begin());
      case HASHDIGEST_SHA1:
        return hashdigest_iterator_t(sha1_manager->begin());
      case HASHDIGEST_SHA256:
        return hashdigest_iterator_t(sha256_manager->begin());
      default: assert(0);
    }
  }

  // end
  hashdigest_iterator_t end() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdigest_iterator_t(md5_manager->end());
      case HASHDIGEST_SHA1:
        return hashdigest_iterator_t(sha1_manager->end());
      case HASHDIGEST_SHA256:
        return hashdigest_iterator_t(sha256_manager->end());
      default: assert(0);
    }
  }

  // quick easy statistic
  size_t map_size() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return md5_manager->map_size();
      case HASHDIGEST_SHA1:
        return sha1_manager->map_size();
      case HASHDIGEST_SHA256:
        return sha256_manager->map_size();
      default: assert(0);
    }
  }
};

#endif

