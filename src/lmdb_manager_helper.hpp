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
 * Defines the static commands that hashdb_manager can execute.
 */

#ifndef LMDB_MANAGER_HELPER
#define LMDB_MANAGER_HELPER
#include <unistd.h>
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "hashdb_directory_manager.hpp"
#include "lmdb_hash_store.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "bloom_filter_manager.hpp"

// Standard includes
#include <cstdlib>
#include <string>

/**
 * Provides static helpers for LMDB managers.
 * This totally static class is a class rather than an namespace
 * so it can have private members.
 */

class lmdb_manager_helper {

  public:

  // create
  static void create(const std::string& hashdb_dir,
                     const hashdb_settings_t& settings) {

    // create the hashdb directory
    hashdb_directory_manager_t::create_new_hashdb_dir(hashdb_dir);

    // write the settings
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // create the new stores
    lmdb_hash_store_t(hashdb_dir, RW_NEW);
    lmdb_name_store_t(hashdb_dir, RW_NEW);
    lmdb_source_store_t(hashdb_dir, RW_NEW);

    // create Bloom
    bloom_filter_manager_t(hashdb_dir, RW_NEW,
                           settings.bloom1_is_used,
                           settings.bloom1_M_hash_size,
                           settings.bloom1_k_hash_functions);
   }
};

#endif

