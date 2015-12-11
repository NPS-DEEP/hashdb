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

#ifndef HASHDB_TOOLS_HPP
#define HASHDB_TOOLS_HPP

#include <config.h>
#include "hashdb.hpp"

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
#include "logger.hpp"
#include <unistd.h>
#include <string>

namespace hashdb {
  /**
   * Return true and "" if hashdb is valid, false and reason if not.
   */
  std::pair<bool, std::string> is_valid_hashdb(
                                      const std::string& hashdb_dir) {

    // path must exist
    if (access(hashdb_dir.c_str(), F_OK) != 0) {
      return std::pair<bool, std::string>(false, "No hashdb at path '"
                       + hashdb_dir + "'.");
    }

    // settings file must exist
    std::string settings_filename = hashdb_dir + "/settings.xml";
    if (access(settings_filename.c_str(), F_OK) != 0) {
        return std::pair<bool, std::string>(false, "Path '"
                     + hashdb_dir + "' is not a hashdb database.");
    }

    // settings version must be compatible
    hashdb_settings_t settings(
                        hashdb_settings_store_t::read_settings(hashdb_dir));
    if (settings.data_store_version < 3) {
        return std::pair<bool, std::string>(false, "The hashdb at path '"
                     + hashdb_dir + "' is not compatible.");
    }

    return std::pair<bool, std::string>(true, "");
  }

  /**
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  // private
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                                     const hashdb_settings_t& settings,
                                     const std::string& log_string) {

    // path must be empty
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      return std::pair<bool, std::string>(false, "Path '"
                       + hashdb_dir + "' already exists.");
    }

    // create the new hashdb directory
    int status;
#ifdef WIN32
    status = mkdir(hashdb_dir.c_str());
#else
    status = mkdir(hashdb_dir.c_str(),0777);
#endif
    if (status != 0) {
      return std::pair<bool, std::string>(false,
                       "Unable to create new hashdb database at path '"
                       + hashdb_dir + "'.");
    }

    // create the settings file
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // create new LMDB stores
    lmdb_hash_manager_t(hashdb_dir, RW_NEW);
    lmdb_hash_label_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_id_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_metadata_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_name_manager_t(hashdb_dir, RW_NEW);

    // create the log
    logger_t logger(hashdb_dir, log_string);
    logger.close();

    return std::pair<bool, std::string>(true, "");
  }

  /**
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  // create using default settings
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                                             const std::string& log_string) {
    hashdb_settings_t settings;
    return create_hashdb(hashdb_dir, settings, log_string);
  }

  // create using settings from other hashdb
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                                             const std::string& from_hashdb_dir,
                                             const std::string& log_string) {

    // validate from path
    std::pair<bool, std::string> pair = hashdb::is_valid_hashdb(hashdb_dir);
    if (pair.first == false) {
      return pair;
    }

    // read settings from the from path
    hashdb_settings_t settings(
                        hashdb_settings_store_t::read_settings(hashdb_dir));

    return create_hashdb(hashdb_dir, settings, log_string);
  }

  // create using minimal settings
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     const std::string& log_string) {
    hashdb_settings_t settings;
    settings.sector_size = sector_size;
    settings.block_size = block_size;
    return create_hashdb(hashdb_dir, settings, log_string);
  }

  // create using full settings
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     bool bloom_is_used,
                     uint32_t bloom_M_hash_size,
                     uint32_t bloom_k_hash_functions,
                     const std::string& log_string) {
    hashdb_settings_t settings;
    settings.sector_size = sector_size;
    settings.block_size = block_size;
    settings.bloom_is_used = bloom_is_used;
    settings.bloom_M_hash_size = bloom_M_hash_size;
    settings.bloom_k_hash_functions = bloom_k_hash_functions;
    return create_hashdb(hashdb_dir, settings, log_string);
  }

  /**
   * Return true and "" if bloom filter rebuilds, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  std::pair<bool, std::string> rebuild_bloom(
                     const std::string& hashdb_dir,
                     const bool bloom_is_used,
                     const uint32_t bloom_M_hash_size,
                     const uint32_t bloom_k_hash_functions,
                     const std::string& log_string) {

    // validate path
    std::pair<bool, std::string> pair = hashdb::is_valid_hashdb(hashdb_dir);
    if (pair.first == false) {
      return pair;
    }

    // read existing settings
    hashdb_settings_t settings;
    settings = hashdb_settings_store_t::read_settings(hashdb_dir);

    // change the bloom filter settings
    settings.bloom_is_used = bloom_is_used;
    settings.bloom_M_hash_size = bloom_M_hash_size;
    settings.bloom_k_hash_functions = bloom_k_hash_functions;

    // write back the changed settings
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    logger_t logger(hashdb_dir, log_string);

    // log the new settings
    logger.add_hashdb_settings(settings);

    // remove existing bloom filter
    std::string filename = hashdb_dir + "/bloom_filter";
    remove(filename.c_str());

    // open the bloom filter manager
    bloom_filter_manager_t bloom_filter_manager(hashdb_dir,
                               RW_NEW,
                               settings.bloom_is_used,
                               settings.bloom_M_hash_size,
                               settings.bloom_k_hash_functions);

    // only add hashes if bloom is used
    if (settings.bloom_is_used) {

      // open hash DB
      lmdb_hash_manager_t manager(hashdb_dir, READ_ONLY);

      // add hashes to the bloom filter
      id_offset_pairs_t id_offset_pairs;
      logger.add_timestamp("begin rebuild_bloom");
      std::string binary_hash = manager.find_begin(id_offset_pairs);
      while (binary_hash != "") {
        bloom_filter_manager.add_hash_value(binary_hash);
        binary_hash = manager.find_next(binary_hash, id_offset_pairs);
      }
    }

    // close logger
    logger.add_timestamp("end rebuild_bloom");
    logger.close();

    // done
    return std::pair<bool, std::string>(true, "");
  }

} // namespace hashdb

#endif

  // private
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                                     const hashdb_settings_t& settings,
                                     const std::string& log_string) {

    // path must be empty
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      return std::pair<bool, std::string>(false, "Path '"
                       + hashdb_dir + "' already exists.");
    }

    // create the new hashdb directory
    int status;
#ifdef WIN32
    status = mkdir(hashdb_dir.c_str());
#else
    status = mkdir(hashdb_dir.c_str(),0777);
#endif
    if (status != 0) {
      return std::pair<bool, std::string>(false,
                       "Unable to create new hashdb database at path '"
                       + hashdb_dir + "'.");
    }

    // create the settings file
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // create new LMDB stores
    lmdb_hash_manager_t(hashdb_dir, RW_NEW);
    lmdb_hash_label_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_id_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_metadata_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_name_manager_t(hashdb_dir, RW_NEW);

    // create the log
    logger_t logger(hashdb_dir, log_string);
    logger.close();

    return std::pair<bool, std::string>(true, "");
  }

