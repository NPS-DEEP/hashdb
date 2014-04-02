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
#include "file_modes.h"
#include "hashdigest_types.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_element.hpp"
#include "hashdb_changes.hpp"
#include "logger.hpp"
#include "query_by_socket.hpp"

// this implementation uses pthread lock to protect the hash database
#ifdef HAVE_PTHREAD
#include <pthread.h>
#define MUTEX_LOCK(M)   pthread_mutex_lock(M)
#define MUTEX_UNLOCK(M) pthread_mutex_unlock(M)
#else
#define MUTEX_LOCK(M)   {}
#define MUTEX_UNLOCK(M) {}
#endif

/**
 * version of the hashdb query library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

  // constructor for importing
  hashdb_t::hashdb_t(const std::string& p_hashdb_dir,
           const std::string& hashdigest_type,
           uint32_t p_block_size,
           uint32_t p_max_duplicates) :
                       hashdb_dir(p_hashdb_dir), // socket not implemented yet
                       mode(HASHDB_IMPORT),
                       hashdb_manager(0),
                       hashdb_changes(0),
                       logger(0),
                       query_by_socket(0),
                       block_size(p_block_size),
                       max_duplicates(p_max_duplicates),
                       M() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif

    // create and write settings to hashdb_dir
    hashdb_settings_t settings;
    bool success = string_to_hashdigest_type(hashdigest_type,
                                             settings.hashdigest_type);
    if (success == false) {
      // setup must be successful
      std::cerr << "Error: Invalid hash algorithm name: '" << hashdigest_type
                << "'\nCannot continue.\n";
      exit(1);
    }
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
  int hashdb_t::import(const import_input_md5_t& input) {
    return import_private(input);
  }
  int hashdb_t::import(const import_input_sha1_t& input) {
    return import_private(input);
  }
  int hashdb_t::import(const import_input_sha256_t& input) {
    return import_private(input);
  }
  template<typename T>
  int hashdb_t::import_private(const std::vector<import_element_t<T> >& input) {

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
                                      hashdigest.hashdigest_type,
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
  hashdb_t::hashdb_t(const std::string& path_or_socket) :
                     hashdb_dir(path_or_socket), // socket not implemented yet
                     mode(path_or_socket.find("//") ==
                         std::string::npos ? HASHDB_SCAN : HASHDB_SCAN_SOCKET),
//                     mode(HASHDB_SCAN),
                     hashdb_manager(0),
                     hashdb_changes(0),
                     logger(0),
                     query_by_socket(0),
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
      // open query by socket service for scanning
      query_by_socket = new query_by_socket_t(hashdb_dir);
    } else {
      assert(0);
      exit(1);
    }
  }

  // scan
  int hashdb_t::scan(const scan_input_md5_t& input, scan_output_t& output) const {
    if (mode == HASHDB_SCAN_SOCKET) {
      return query_by_socket->scan<std::pair<uint64_t, md5_t> >(&QUERY_MD5, input, output);
    } else {
      return scan_private(input, output);
    }
  }
  int hashdb_t::scan(const scan_input_sha1_t& input, scan_output_t& output) const {
    if (mode == HASHDB_SCAN_SOCKET) {
      return query_by_socket->scan<std::pair<uint64_t, sha1_t> >(&QUERY_SHA1, input, output);
    } else {
      return scan_private(input, output);
    }
  }
  int hashdb_t::scan(const scan_input_sha256_t& input, scan_output_t& output) const {
    if (mode == HASHDB_SCAN_SOCKET) {
      return query_by_socket->scan<std::pair<uint64_t, sha256_t> >(&QUERY_SHA256, input, output);
    } else {
      return scan_private(input, output);
    }
  }
  template<typename T>
  int hashdb_t::scan_private(const std::vector<std::pair<uint64_t, T> >& input,
                             scan_output_t& output) const {

    // check mode
    if (mode != HASHDB_SCAN) {
      return -1;
    }

    // clear any old output
    output.clear();
           
    // no socket implemented yet, so directly use hashdb

    // perform all scans in one locked operation.
    // There is basically no cost for grouping since this iterates db access.
    // There is gain if this is a large sorted request.
    MUTEX_LOCK(&M);

    // scan each input in turn
    typename std::vector<std::pair<uint64_t, T> >::const_iterator it = input.begin();
    while (it != input.end()) {
      uint32_t count = hashdb_manager->find_count(it->second);
      if (count > 0) {
        std::pair<uint64_t, uint32_t> return_item(it->first, count);
        output.push_back(return_item);
//        output.push_back(typename std::vector<std::pair<uint64_t, uint32_t> >(
//                                                          it->first, count));
      }
      ++it;
    }

    MUTEX_UNLOCK(&M);

    // good, done
    return 0;
  }

  // destructor
  hashdb_t::~hashdb_t() {
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
        delete query_by_socket;
        return;
    }
  }

// mac doesn't seem to be up to c++11 yet
#ifndef HAVE_CXX11
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t::hashdb_t(const hashdb_t& other) :
                 hashdb_dir(""),
                 mode(HASHDB_NONE),
                 hashdb_manager(0),
                 hashdb_changes(0),
                 logger(0),
                 query_by_socket(0),
                 block_size(0),
                 max_duplicates(0),
                 M() {
    assert(0);
    exit(1);
  }
  // if c++11 fail at compile time else fail at runtime upon invocation
  hashdb_t& hashdb_t::operator=(const hashdb_t& other) {
    assert(0);
    exit(1);
  }
#endif

