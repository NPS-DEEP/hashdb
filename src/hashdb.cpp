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
#include "hashdb_manager.hpp"
#include <dfxml/src/hash_t.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <climits>
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_element.hpp"
#include "hashdb_changes.hpp"
#include "logger.hpp"
#include "tcp_client_manager.hpp"

// this implementation uses pthread lock to protect the hash database
#include "mutex_lock.hpp"

/**
 * version of the hashdb library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

  // constructor for importing
  hashdb_t__<typename T>::hashdb_t(const std::string& p_hashdb_dir,
                                   uint32_t p_block_size,
                                   uint32_t p_max_duplicates) :
                       hashdb_dir(p_hashdb_dir),
                       mode(HASHDB_IMPORT),
                       hashdb_manager(0),
                       hashdb_changes(0),
                       logger(0),
                       tcp_client_manager(0),
                       block_size(p_block_size),
                       max_duplicates(p_max_duplicates),
                       M() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif

    // create and write settings to hashdb_dir
    hashdb_settings_t settings;
    settings.hash_block_size = block_size;
    settings.maximum_hash_duplicates = max_duplicates;
    hashdb_settings_manager_t::write_settings(hashdb_dir, settings);

    // create hashdb_manager
    hashdb_manager = new hashdb_manager_t(hashdb_dir, RW_NEW);
    hashdb_changes = new hashdb_changes_t;

    // open logger
    logger = new logger_t(hashdb_dir, "hashdb library import");
    logger->add_timestamp("begin import");
  }

  // import
  int hashdb_t__<typename T>::import(const import_input_t& input) {
    return import_private(input);

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // import each input in turn
    typename std::vector<import_element_t<T> >::const_iterator it = input.begin();

    // perform all scans in one locked operation.
    // There is basically no cost for grouping since this iterates db access.
    // There is gain if this is a large sorted request.
    MUTEX_LOCK(&M);

    while (it != input.end()) {
      // convert input to hashdb_element_t
      hashdigest_t hashdigest(it->hash);
      hashdb_element_t hashdb_element(hashdigest.hashdigest,
                                      block_size,
                                      it->repository_name,
                                      it->filename,
                                      it->file_offset);

      // add hashdb_element_t to hashdb_manager
      hashdb_manager->insert(hashdb_element, *hashdb_changes);

      ++it;
    }

    MUTEX_UNLOCK(&M);

    // good, done
    return 0;
  }

  // constructor for scanning
  hashdb_t__<typename T>::hashdb_t(const std::string& path_or_socket) :
                     hashdb_dir(path_or_socket),
                     mode(path_or_socket.find("tcp://") != 0
                         ? HASHDB_SCAN : HASHDB_SCAN_SOCKET),
                     hashdb_manager(0),
                     hashdb_changes(0),
                     logger(0),
                     tcp_client_manager(0),
                     block_size(0),
                     max_duplicates(0),
                     M() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif

    // open the correct scan resource
    if (mode == HASHDB_SCAN) {
      // open hashdb_manager for scanning
      hashdb_manager = new hashdb_manager_t(hashdb_dir, READ_ONLY);
    } else if (mode == HASHDB_SCAN_SOCKET) {
      // open TCP socket service for scanning
      tcp_client_manager = new tcp_client_manager_t(path_or_socket);
    } else {
      assert(0);
      exit(1);
    }
  }

  // scan
  int hashdb_t__<typename T>::scan(const scan_input_t& input, scan_output_t& output) const {

    // check mode
    if (mode != HASHDB_SCAN) {
      return -1;
    }

    // clear any old output
    output.clear();
           
    // since we optimize by limiting the return index size to uint32_t,
    // we must reject vector size > uint32_t
    if (input.size() > ULONG_MAX) {
      std::cerr << "Error: array too large.  discarding.\n";
      return -1;
    }

    // perform all scans in one locked operation.
    // There is basically no cost for grouping since this iterates db access.
    // There is gain if this is a large sorted request.
    MUTEX_LOCK(&M);

    // scan each input in turn
    uint32_t input_size = (uint32_t)input.size();
    for (uint32_t i=0; i<input_size; i++) {
      uint32_t count = hashdb_manager->find_count(input[i]);
      if (count > 0) {
        output.push_back(std::pair<uint32_t, uint32_t>(i, count));
      }
    }

    MUTEX_UNLOCK(&M);

    // good, done
    return 0;
  }

  // destructor
  hashdb_t__<typename T>::~hashdb_t() {
    switch(mode) {
      case HASHDB_NONE:
        return;
      case HASHDB_IMPORT:
        logger->add_timestamp("end import");
        logger->add_hashdb_changes(*hashdb_changes);
        delete hashdb_manager;
        delete hashdb_changes;
        delete logger;
        return;
      case HASHDB_SCAN:
        delete hashdb_manager;
        return;
      case HASHDB_SCAN_SOCKET:
        delete tcp_client_manager;
        return;
    }
  }

// mac doesn't seem to be up to c++11 yet
#ifndef HAVE_CXX11
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t__<typename T>::hashdb_t(const hashdb_t__<typename T>hashdb_t& other) :
                 hashdb_dir(""),
                 mode(HASHDB_NONE),
                 hashdb_manager(0),
                 hashdb_changes(0),
                 logger(0),
                 tcp_client_manager(0),
                 block_size(0),
                 max_duplicates(0),
                 M() {
    assert(0);
    exit(1);
  }
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t__<typename T>& hashdb_t__<typename T>::operator=(const hashdb_t__<typename T>& other) {
    assert(0);
    exit(1);
  }
#endif

