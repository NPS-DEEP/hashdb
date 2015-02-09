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
#include "hashdb_directory_manager.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "hashdb_changes.hpp"
#include "history_manager.hpp"
#include "lmdb_hash_store.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_change_manager.hpp"
#include "lmdb_reader_manager.hpp"
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
                   change_manager(),
                   reader_manager(),
                   logger(0) {
  }

  // open for importing, return true else false with error string.
  std::pair<bool, std::string> hashdb_t::open_import(
                                             const std::string& p_hashdb_dir,
                                             uint32_t p_block_size,
                                             uint32_t p_max_duplicates) {

    if (mode != HASHDB_NONE) {
      std::cerr << "invalid mode " << (int)mode << "\n";
      assert(0);
      exit(1);
    }
    mode = HASHDB_IMPORT;
    path_or_socket = p_hashdb_dir;
    block_size = p_block_size;
    max_duplicates = p_max_duplicates;

    try {
      // create settings
      hashdb_settings_t settings;
      settings.hash_block_size = block_size;
      settings.maximum_hash_duplicates = max_duplicates;

      // create the databases
      lmdb_manager_helper::create(hashdb_dir, settings);

      // open for writing
      change_manager = new lmdb_change_manager_t(path_or_socket);

      // open logger
      logger = new logger_t(path_or_socket, "hashdb library import");
      logger->add_hashdb_settings(settings);
      logger->add_timestamp("begin import");

      return std::pair<bool, std::string>(true, "");
    } catch (std::runtime_error& e) {
      return std::pair<bool, std::string>(false, e.what());
    }
  }

  // import
  int hashdb_t::import(const import_input_t& input) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // import each input in turn
    std::vector<import_element_t>::const_iterator it = input.begin();

    // import all elements
    while (it != input.end()) {

      // set source data record
      lmdb_source_data_t source_data(it->repository_name, it->filename, 0, "");

      // insert the source data
      change_manager->insert(it->binary_hash, source_data, it->file_offset);

      ++it;
    }

    // good, done
    return 0;
  }

  // import metadata
  int hashdb_t::import_metadata(const std::string& repository_name,
                                          const std::string& filename,
                                          uint64_t filesize,
                                          const std::string& binary_hash) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // set source data record
    lmdb_source_data_t source_data(repository_name, filename,
                                   filesize, binary_hash);

    // add the change
    change_manager->add_source_data(source_data);

    // good, done
    return 0;
  }

  // Open for scanning with a lock around one scan resource.
  std::pair<bool, std::string> hashdb_t::open_scan(
                                         const std::string& p_path_or_socket) {
    path_or_socket = p_path_or_socket;
    if (mode != HASHDB_NONE) {
      std::cerr << "invalid mode " << (int)mode << "\n";
      assert(0);
      exit(1);
    }

    // perform setup based on selection
    if (path_or_socket.find("tcp://") == 0) {
      mode = HASHDB_SCAN_SOCKET;
      // open TCP socket service for scanning
      std::cerr << "TCP scan currently not implemented\n";
      return std::pair<bool, std::string>(false, "TCP scan currently not implemented");
    } else {
      mode = HASHDB_SCAN;
      // open reader manager for scanning
      try {
        reader_manager = new lmdb_reader_manager_t(path_or_socket);
      } catch (std::runtime_error& e) {
        return std::pair<bool, std::string>(false, e.what());
      }
    }
    return std::pair<bool, std::string>(true, "");
  }

  // scan
  int hashdb_t::scan(const scan_input_t& input, scan_output_t& output) const {

    // clear any old output
    output.clear();
           
    switch(mode) {
      case HASHDB_SCAN: {
        // run scan

        // since we optimize by limiting the return index size to uint32_t,
        // we must reject vector size > uint32_t
        if (input.size() > ULONG_MAX) {
          std::cerr << "Error: array too large.  discarding.\n";
          return -1;
        }

        // scan each input in turn
        uint32_t input_size = (uint32_t)input.size();
        for (uint32_t i=0; i<input_size; i++) {
          uint32_t count = reader_manager->find_count(input[i]);
          if (count > 0) {
            output.push_back(std::pair<uint32_t, uint32_t>(i, count));
          }
        }

        // good, done
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
        logger->add_hashdb_changes(change_manager->changes);
        delete logger;
        delete change_manager;

        // create new history trail
        history_manager_t::append_log_to_history(path_or_socket);
        return;

      case HASHDB_SCAN:
        delete reader_manager;
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
  hashdb_t::hashdb_t__(const hashdb_t& other) :
                 path_or_socket(""),
                 block_size(0),
                 max_duplicates(0),
                 mode(HASHDB_NONE),
                 change_manager(0),
                 reader_manager(0),
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

