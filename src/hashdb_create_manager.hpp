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
 * Create a new hashdb.
 */

#ifndef HASHDB_CREATE_MANAGER_HPP
#define HASHDB_CREATE_MANAGER_HPP

// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include "globals.hpp"
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "lmdb.h"
#include "lmdb_typedefs.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_hash_label_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_metadata_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include "bloom_filter_manager.hpp"
#include "hashdb_changes.hpp"
#include <unistd.h>
#include <string>

// no concurrent changes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class hashdb_create_manager {
  public:
  /**
   * Return true if hashdb directory is created.
   * Return false if hashdb already exists.
   * Fail if path exists but it is not a hashdb directory because no settings.
   */
  static bool create_if_new(const std::string& hashdb_dir,
                            const hashdb_settings_t& settings) {

    // false if hashdb already exists.
    std::string settings_filename = hashdb_dir + "/settings.xml";
    if (access(settings_filename.c_str(), F_OK) == 0) {
      // hashdb already exists
      return false;
    }

    // fail if path exists but not a hashdb
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      std::cout << "Error: path '" << hashdb_dir
                << "' exists but is not a hashdb database.\nAbortinig.\n";
      exit(1);
    }

    // create the new hashdb directory
    int status;
#ifdef WIN32
    status = mkdir(hashdb_dir.c_str());
#else
    status = mkdir(hashdb_dir.c_str(),0777);
#endif
    if (status != 0) {
      std::cerr << "Error: Could not create new hashdb database '"
                << hashdb_dir << "'.\nCannot continue.\n";
      exit(1);
    }
 
    // create the settings file
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // create new LMDB stores
    lmdb_hash_manager_t(hashdb_dir, RW_NEW);
    lmdb_hash_label_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_id_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_metadata_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_name_manager_t(hashdb_dir, RW_NEW);

    return true;
  }
};

#endif

