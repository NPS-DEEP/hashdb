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
 * Class file for the hashdb library.
 */

#include <config.h>
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
#include "hashdb.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <climits>
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "hashdb_changes.hpp"
#include "history_manager.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_rw_manager.hpp"
#include "lmdb_ro_manager.hpp"
#include "lmdb_rw_new.hpp"
#include "bloom_filter_manager.hpp"
#include "logger.hpp"
#ifndef HAVE_CXX11
#include <cassert>
#endif

/**
 * version of the hashdb library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

  // constructor
  hashdb_t::hashdb_t() :
                   path_or_socket(""),
                   block_size(0),
                   max_duplicates(0),
                   mode(HASHDB_NONE),
                   rw_manager(),
                   ro_manager(),
                   logger(0) {
  }

  // open for importing else abort with error message.
  void hashdb_t::open_import(const std::string& p_hashdb_dir,
                             uint32_t p_block_size,
                             uint32_t p_max_duplicates) {

    if (mode != HASHDB_NONE) {
      std::cerr << "Usage error: invalid mode " << (int)mode << "\n";
      exit(1);
    }
    mode = HASHDB_IMPORT;
    path_or_socket = p_hashdb_dir;
    block_size = p_block_size;
    max_duplicates = p_max_duplicates;

    // create settings
    hashdb_settings_t settings;
    settings.hash_block_size = block_size;
    settings.maximum_hash_duplicates = max_duplicates;

    // create the databases
    lmdb_rw_new::create(path_or_socket, settings);

    // open for writing
    rw_manager = new lmdb_rw_manager_t(path_or_socket);

    // open logger
    logger = new logger_t(path_or_socket, "hashdb library import");
    logger->add_hashdb_settings(settings);
    logger->add_timestamp("begin import");
  }

  // import
  int hashdb_t::import(const std::string& binary_hash,
                       const uint64_t file_offset,
                       const std::string& repository_name,
                       const std::string& filename,
                       const uint64_t filesize,
                       const std::string& file_binary_hash,
                       const std::string& block_hash_label) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // insert the source data
    rw_manager->insert(binary_hash,
                       file_offset,
                       block_size,
                       lmdb_source_data_t(repository_name,
                                          filename,
                                          filesize,
                                          file_binary_hash),
                       block_hash_label);

    // good, done
    return 0;
  }

  // open for scanning else abort with error message.
  void hashdb_t::open_scan(const std::string& p_path_or_socket) {
    path_or_socket = p_path_or_socket;
    if (mode != HASHDB_NONE) {
      std::cerr << "Usage error: invalid mode " << (int)mode << "\n";
      exit(1);
    }

    // perform setup based on selection
    if (path_or_socket.find("tcp://") == 0) {
      mode = HASHDB_SCAN_SOCKET;
      // open TCP socket service for scanning
      std::cerr << "TCP scan currently not implemented.\nAborting.\n";
      exit(1);
    } else {
      mode = HASHDB_SCAN;
      // open reader manager for scanning
      ro_manager = new lmdb_ro_manager_t(path_or_socket);
    }
  }

  // scan
  int hashdb_t::scan(const std::string& binary_hash,
                     uint32_t& count) const {

    switch(mode) {
      case HASHDB_SCAN: {
        count = ro_manager->find_count(binary_hash);
        return 0;
      }

      case HASHDB_SCAN_SOCKET: {
        std::cerr << "TCP scan currently not implemented\n";
      }

      default: {
        std::cerr << "Error: unable to scan, wrong scan mode.\n";
        return -1;
      }
    }
  }

  // destructor
  hashdb_t::~hashdb_t() {
    switch(mode) {
      case HASHDB_NONE:
        return;
      case HASHDB_IMPORT:
        logger->add_timestamp("end import");
        logger->add_hashdb_changes(rw_manager->changes);
        delete logger;
        delete rw_manager;

        // create new history trail
        history_manager_t::append_log_to_history(path_or_socket);
        return;

      case HASHDB_SCAN:
        delete ro_manager;
        return;
      case HASHDB_SCAN_SOCKET:
        std::cerr << "TCP scan currently not implemented\n";
        return;
      default:
        // program error
        assert(0);
    }
  }

// mac doesn't seem to be up to c++11 yet
#ifndef HAVE_CXX11
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t::hashdb_t(const hashdb_t& other) :
                 path_or_socket(""),
                 block_size(0),
                 max_duplicates(0),
                 mode(HASHDB_NONE),
                 rw_manager(0),
                 ro_manager(0),
                 logger(0) {
    assert(0);
    exit(1);
  }
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t& hashdb_t::operator=(const hashdb_t& other) {
    assert(0);
    exit(1);
  }
#endif

