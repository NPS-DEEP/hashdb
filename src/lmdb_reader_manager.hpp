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
 * Provides DB reader services.
 */

#ifndef LMDB_READER_MANAGER_HPP
#define LMDB_READER_MANAGER_HPP
#include "hashdb_settings.hpp"
#include "globals.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "lmdb_hash_store.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_source_it_data.hpp"
#include "lmdb_reader_it_data.hpp"
#include "lmdb_helper.h"
#include "bloom_filter_manager.hpp"

class lmdb_reader_manager_t {

  private:
  const std::string hashdb_dir;
  const hashdb_settings_t settings;

  // bloom filter manager
  bloom_filter_manager_t bloom_filter_manager;

  // DB stores
  lmdb_hash_store_t hash_store;
  lmdb_source_store_t source_store;

  // do not allow copy or assignment
  lmdb_reader_manager_t(const lmdb_reader_manager_t&);
  lmdb_reader_manager_t& operator=(const lmdb_reader_manager_t&);

  public:
  lmdb_reader_manager_t(const std::string& p_hashdb_dir) :

          hashdb_dir(p_hashdb_dir),
          settings(hashdb_settings_store_t::read_settings(hashdb_dir)),
          bloom_filter_manager(hashdb_dir, READ_ONLY,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions),
          hash_store(hashdb_dir, READ_ONLY),
          source_store(hashdb_dir, READ_ONLY) {
  }

  size_t find_count(const std::string& binary_hash) const {
    if (!bloom_filter_manager.is_positive(binary_hash)) {
      return 0;
    }
    return hash_store.find_count(binary_hash);
  }

  lmdb_hash_it_data_t find_first(const std::string& binary_hash) const {
    return hash_store.find_first(binary_hash);
  }

  lmdb_hash_it_data_t find_begin() const {
    return hash_store.find_begin();
  }

  lmdb_hash_it_data_t find_next(const lmdb_hash_it_data_t& hash_it_data) const {
    return hash_store.find_next(lmdb_hash_it_data_t& hash_it_data);
  }

  lmdb_source_data_t find_source(uint64_t source_lookup_index) const {
    return source_store.find_source(source_lookup_index);
  }
};

#endif

