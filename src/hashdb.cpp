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
#include <hash_t_selector.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <climits>
#include "file_modes.h"
#include "hashdb_directory_manager.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_element.hpp"
#include "hashdb_changes.hpp"
#include "history_manager.hpp"
#include "logger.hpp"
#include "tcp_client_manager.hpp"
#include "source_metadata_element.hpp"
#ifndef HAVE_CXX11
#include <cassert>
#endif

// this implementation uses pthread lock to protect the hash database
#include "mutex_lock.hpp"

// static resources for supporting PTHREAD mode
static pthread_key_t pthread_hashdb_manager_key;
static pthread_key_t pthread_tcp_client_manager_key;
static pthread_once_t pthread_hashdb_manager_key_once = PTHREAD_ONCE_INIT;
static pthread_once_t pthread_tcp_client_manager_key_once = PTHREAD_ONCE_INIT;
static std::vector<hashdb_manager_t*> pthread_hashdb_managers;
static std::vector<tcp_client_manager_t*> pthread_tcp_client_managers;
static void make_pthread_hashdb_manager_key() {
  (void) pthread_key_create(&pthread_hashdb_manager_key, NULL);
}
static void make_pthread_tcp_client_manager_key() {
  (void) pthread_key_create(&pthread_tcp_client_manager_key, NULL);
}

/**
 * version of the hashdb library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

  // constructor
  template<>
  hashdb_t__<hash_t>::hashdb_t__() :
                   path_or_socket(""),
                   block_size(0),
                   max_duplicates(0),
                   mode(HASHDB_NONE),
                   hashdb_manager(0),
                   tcp_client_manager(0),
//                   pthread_hashdb_managers(),
//                   pthread_tcp_client_managers(),
                   logger(0),
                   M() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif
  }

  // open for importing, return true else false with error string.
  template<>
  std::pair<bool, std::string> hashdb_t__<hash_t>::open_import(
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
      // create the hashdb directory at the path
      hashdb_directory_manager_t::create_new_hashdb_dir(path_or_socket);

      // create and write settings at the hashdb directory path
      hashdb_settings_t settings;
      settings.hash_block_size = block_size;
      settings.maximum_hash_duplicates = max_duplicates;
      hashdb_settings_store_t::write_settings(path_or_socket, settings);

      // create hashdb_manager
      hashdb_manager = new hashdb_manager_t(path_or_socket, RW_NEW);

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
  template<>
  int hashdb_t__<hash_t>::import(const import_input_t& input) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // import each input in turn
    std::vector<import_element_t>::const_iterator it = input.begin();

    // perform all imports in one locked operation.
    // There is basically no cost for grouping since this iterates db access.
    // There is gain if this is a large sorted request.
    MUTEX_LOCK(&M);

    while (it != input.end()) {
      // convert input to hashdb_element_t
      hashdb_element_t hashdb_element(it->hash,
                                         block_size,
                                         it->repository_name,
                                         it->filename,
                                         it->file_offset);

      // add hashdb_element_t to hashdb_manager
      hashdb_manager->insert(hashdb_element);

      ++it;
    }

    MUTEX_UNLOCK(&M);

    // good, done
    return 0;
  }

  // import metadata
  template<>
  int hashdb_t__<hash_t>::import_metadata(const std::string& repository_name,
                                          const std::string& filename,
                                          uint64_t filesize,
                                          const hash_t& hashdigest) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // lock while importing
    MUTEX_LOCK(&M);

    // acquire existing or new source lookup index
    uint64_t source_id = hashdb_manager->insert_source(
                                                  repository_name, filename);
 
    // add the metadata associated with this source
    hashdb_manager->insert_source_metadata(source_id, filesize, hashdigest);

    MUTEX_UNLOCK(&M);

    // good, done
    return 0;
  }

  // Open for scanning with a lock around one scan resource.
  template<>
  std::pair<bool, std::string> hashdb_t__<hash_t>::open_scan(
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
      try {
        tcp_client_manager = new tcp_client_manager_t(path_or_socket);
      } catch (std::runtime_error& e) {
        return std::pair<bool, std::string>(false, e.what());
      }
    } else {
      mode = HASHDB_SCAN;
      // open hashdb_manager for scanning
      try {
        hashdb_manager = new hashdb_manager_t(path_or_socket, READ_ONLY);
      } catch (std::runtime_error& e) {
        return std::pair<bool, std::string>(false, e.what());
      }
    }
    return std::pair<bool, std::string>(true, "");
  }

  // Open for scanning with a separate scan resource per thread.
  template<>
  std::pair<bool, std::string> hashdb_t__<hash_t>::open_scan_pthread(
                                         const std::string& p_path_or_socket) {
    path_or_socket = p_path_or_socket;
    if (mode != HASHDB_NONE) {
      std::cerr << "invalid mode " << (int)mode << "\n";
      assert(0);
      exit(1);
    }

    // perform setup based on selection
    if (path_or_socket.find("tcp://") == 0) {
      mode = HASHDB_SCAN_SOCKET_PTHREAD;
    } else {
      mode = HASHDB_SCAN_PTHREAD;
    }
    return std::pair<bool, std::string>(true, "");
  }

  // scan
  template<>
  int hashdb_t__<hash_t>::scan(const scan_input_t& input,
                               scan_output_t& output) const {

    // clear any old output
    output.clear();
           
    switch(mode) {
      case HASHDB_SCAN: {
        // run scan using hashdb_manager

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

      case HASHDB_SCAN_SOCKET: {
        // run scan using tcp_client_manager
        return tcp_client_manager->scan(input, output);
      }

      case HASHDB_SCAN_PTHREAD: {

        hashdb_manager_t* pthread_hashdb_manager;

        // get thread-specific pthread_hashdb_manager, possibly creating it
        (void) pthread_once(&pthread_hashdb_manager_key_once,
                            make_pthread_hashdb_manager_key);
        if (pthread_getspecific(pthread_hashdb_manager_key) == NULL) {

          // create hashdb_manager for scanning
          try {
            pthread_hashdb_manager =
                             new hashdb_manager_t(path_or_socket, READ_ONLY);
          } catch (std::runtime_error& e) {
            std::cerr << e.what() << "\n";
            return -1;
          }

          // bind the manager to this pthread key
          pthread_setspecific(pthread_hashdb_manager_key,
                              pthread_hashdb_manager);

          // save pointer for destructor
          MUTEX_LOCK(&M);
          pthread_hashdb_managers.push_back(pthread_hashdb_manager);
          MUTEX_UNLOCK(&M);
        } else {
          pthread_hashdb_manager =(hashdb_manager_t*)pthread_getspecific(
                                                 pthread_hashdb_manager_key);
        }

        // run scan using the thread-specific hashdb_manager, unlocked

        // scan each input in turn
        uint32_t input_size_pthread = (uint32_t)input.size();
        for (uint32_t i_pthread=0; i_pthread<input_size_pthread; i_pthread++) {
          uint32_t count_pthread = pthread_hashdb_manager->find_count(
                                                           input[i_pthread]);
          if (count_pthread > 0) {
            output.push_back(std::pair<uint32_t, uint32_t>(i_pthread, count_pthread));
          }
        }

        // good, done
        return 0;
      }

      case HASHDB_SCAN_SOCKET_PTHREAD: {
        tcp_client_manager_t* pthread_tcp_client_manager;

        // get thread-specific pthread_tcp_client_manager, possibly creating it
        (void) pthread_once(&pthread_tcp_client_manager_key_once,
                            make_pthread_tcp_client_manager_key);
        if (pthread_getspecific(pthread_tcp_client_manager_key) == NULL) {

          // create tcp_client_manager for scanning
          try {
            pthread_tcp_client_manager =
                                   new tcp_client_manager_t(path_or_socket);
          } catch (std::runtime_error& e) {
            std::cerr << e.what() << "\n";
            return -1;
          }

          // bind the manager to this pthread key
          pthread_setspecific(pthread_tcp_client_manager_key,
                              pthread_tcp_client_manager);

          // save pointer for destructor
          MUTEX_LOCK(&M);
          pthread_tcp_client_managers.push_back(pthread_tcp_client_manager);
          MUTEX_UNLOCK(&M);
        } else {
          pthread_tcp_client_manager =
                                 (tcp_client_manager_t*)pthread_getspecific(
                                            pthread_tcp_client_manager_key);
        }

        // run scan using pthread_tcp_client_manager
        return pthread_tcp_client_manager->scan(input, output);
      }

      default: {
        std::cerr << "Error: unable to scan, wrong scan mode.\n";
        return -1;
      }
    }
  }

  // destructor
  template<>
  hashdb_t__<hash_t>::~hashdb_t__() {
    switch(mode) {
      case HASHDB_NONE:
        return;
      case HASHDB_IMPORT:
        logger->add_timestamp("end import");
        logger->add_hashdb_changes(hashdb_manager->changes);
        delete logger;
        delete hashdb_manager;

        // create new history trail
        history_manager_t::append_log_to_history(path_or_socket);
        return;

      case HASHDB_SCAN:
        delete hashdb_manager;
        return;
      case HASHDB_SCAN_SOCKET:
        delete tcp_client_manager;
        return;
      case HASHDB_SCAN_PTHREAD:
        for (std::vector<hashdb_manager_t*>::const_iterator it =
             pthread_hashdb_managers.begin();
             it != pthread_hashdb_managers.end(); ++it) {
          delete *it;
        }
        return;
      case HASHDB_SCAN_SOCKET_PTHREAD:
        for (std::vector<tcp_client_manager_t*>::const_iterator tcp_it =
             pthread_tcp_client_managers.begin();
             tcp_it != pthread_tcp_client_managers.end(); ++tcp_it) {
          delete *tcp_it;
        }
        return;
      default:
        // program error
        assert(0);
    }
  }

// mac doesn't seem to be up to c++11 yet
#ifndef HAVE_CXX11
  // if c++11 fail at compile time else fail at runtime upon invocation
  template<>
  hashdb_t__<hash_t>::hashdb_t__(const hashdb_t__<hash_t>& other) :
                 path_or_socket(""),
                 block_size(0),
                 max_duplicates(0),
                 mode(HASHDB_NONE),
                 hashdb_manager(0),
                 tcp_client_manager(0),
                 logger(0),
                 M() {
    assert(0);
    exit(1);
  }
  // if c++11 fail at compile time else fail at runtime upon invocation
  template<>
  hashdb_t__<hash_t>& hashdb_t__<hash_t>::operator=(const hashdb_t__<hash_t>& other) {
    assert(0);
    exit(1);
  }
#endif

