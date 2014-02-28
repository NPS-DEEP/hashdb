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

/**
 * version of the hashdb query library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

  // constructor for importing
  hashdb_t::hashdb_t(const std::string& hashdb_dir,
           const std::string& hashdigest_type,
           uint32_t p_block_size,
           uint32_t p_max_duplicates) :
                       mode(HASHDB_IMPORT),
                       hashdb_manager(0),
                       hashdb_changes(0),
                       block_size(p_block_size),
                       max_duplicates(p_max_duplicates) {

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

    // create hashdb_manager for importing
    hashdb_manager = new hashdb_manager_t(hashdb_dir, RW_NEW);
    hashdb_changes = new hashdb_changes_t;
  }

  // these theree imports are nearly identical; they should use template.
  // Import md5
  int hashdb_t::import(md5_t hash,
             std::string repository_name,
             std::string filename,
             uint64_t file_offset) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // create hashdb element to insert
    hashdigest_t hashdigest(hash);
    hashdb_element_t hashdb_element(hashdigest.hashdigest,
                                    hashdigest.hashdigest_type,
                                    block_size,
                                    repository_name,
                                    filename,
                                    file_offset);

    hashdb_manager->insert(hashdb_element, *hashdb_changes);

    // good, done
    return 0;
  }
  // Import sha1
  int hashdb_t::import(sha1_t hash,
             std::string repository_name,
             std::string filename,
             uint64_t file_offset) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // create hashdb element to insert
    hashdigest_t hashdigest(hash);
    hashdb_element_t hashdb_element(hashdigest.hashdigest,
                                    hashdigest.hashdigest_type,
                                    block_size,
                                    repository_name,
                                    filename,
                                    file_offset);

    hashdb_manager->insert(hashdb_element, *hashdb_changes);

    // good, done
    return 0;
  }
  // Import sha256
  int hashdb_t::import(sha256_t hash,
             std::string repository_name,
             std::string filename,
             uint64_t file_offset) {

    // check mode
    if (mode != HASHDB_IMPORT) {
      return -1;
    }

    // create hashdb element to insert
    hashdigest_t hashdigest(hash);
    hashdb_element_t hashdb_element(hashdigest.hashdigest,
                                    hashdigest.hashdigest_type,
                                    block_size,
                                    repository_name,
                                    filename,
                                    file_offset);

    hashdb_manager->insert(hashdb_element, *hashdb_changes);

    // good, done
    return 0;
  }

  // constructor for scanning
  hashdb_t::hashdb_t(const std::string& path_or_socket) :
                       mode(HASHDB_SCAN),
                       hashdb_manager(0),
                       hashdb_changes(0),
                       block_size(0),
                       max_duplicates(0) {

    // no socket implemented yet, so go directly to hashdb_dir
    std::string hashdb_dir = path_or_socket;

    // open hashdb_manager for scanning
    hashdb_manager = new hashdb_manager_t(hashdb_dir, READ_ONLY);
  }

  // these theree scans are nearly identical; they should use template.
  // Scan md5
  int hashdb_t::scan(const std::vector<std::pair<uint64_t, md5_t> >& input,
                     scan_output_t& output) {

    // check mode
    if (mode != HASHDB_SCAN) {
      return -1;
    }

    // clear any old output
    output.clear();
           
    // no socket implemented yet, so directly use hashdb

    // scan each input in turn
    std::vector<std::pair<uint64_t, md5_t> >::const_iterator it = input.begin();
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

    // good, done
    return 0;
  }

  // Scan sha1
  int hashdb_t::scan(const std::vector<std::pair<uint64_t, sha1_t> >& input,
                     scan_output_t& output) {

    // check mode
    if (mode != HASHDB_SCAN) {
      return -1;
    }

    // clear any old output
    output.clear();
           
    // no socket implemented yet, so directly use hashdb

    // scan each input in turn
    std::vector<std::pair<uint64_t, sha1_t> >::const_iterator it = input.begin();
    while (it != input.end()) {
      uint32_t count = hashdb_manager->find_count(it->second);
      if (count > 0) {
        std::pair<uint64_t, uint32_t> return_item(it->first, count);
        output.push_back(return_item);
//        output.push_back(std::vector<std::pair<uint64_t, uint32_t> >(
//                                                          it->first, count));
      }
      ++it;
    }

    // good, done
    return 0;
  }

  // Scan sha256
  int hashdb_t::scan(const std::vector<std::pair<uint64_t, sha256_t> >& input,
                     scan_output_t& output) {

    // check mode
    if (mode != HASHDB_SCAN) {
      return -1;
    }

    // clear any old output
    output.clear();
           
    // no socket implemented yet, so directly use hashdb

    // scan each input in turn
    std::vector<std::pair<uint64_t, sha256_t> >::const_iterator it = input.begin();
    while (it != input.end()) {
      uint32_t count = hashdb_manager->find_count(it->second);
      if (count > 0) {
        std::pair<uint64_t, uint32_t> return_item(it->first, count);
        output.push_back(return_item);
//        output.push_back(std::vector<std::pair<uint64_t, uint32_t> >(
//                                                          it->first, count));
      }
      ++it;
    }

    // good, done
    return 0;
  }

  // destructor
  hashdb_t::~hashdb_t() {
    switch(mode) {
      case HASHDB_NONE:
        return;
      case HASHDB_IMPORT:
        delete hashdb_manager;
        delete hashdb_changes;
        return;
      case HASHDB_SCAN:
        delete hashdb_manager;
        return;
    }
  }

  hashdb_t::hashdb_t(const hashdb_t& other) :
                 mode(HASHDB_NONE),
                 hashdb_manager(0),
                 hashdb_changes(0),
                 block_size(0),
                 max_duplicates(0) {
    // fix zzzzzz
    assert(0);
  }
  hashdb_t& hashdb_t::operator=(const hashdb_t& other) {
    // fix zzzzzz
    assert(0);
  }

