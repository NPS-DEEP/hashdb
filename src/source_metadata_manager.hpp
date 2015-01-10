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
 * Provides simple lookup and add interfaces for a two-index btree store.
 */

#ifndef SOURCE_METADATA_MANAGER_HPP
#define SOURCE_METADATA_MANAGER_HPP

#include "hash_t_selector.h" // to define hash_t
#include <string>
#include <cassert>
#include "boost/btree/index_helpers.hpp"
#include "boost/btree/btree_index_set.hpp"
#include "file_modes.h"
#include "source_metadata.hpp"

// a lock is required to protect btree
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class source_metadata_manager_t {

  private:
  struct map_value_t {
    uint64_t filesize;
    hash_t hashdigest;
    map_value_t(uint64_t p_filesize, hash_t p_hashdigest) :
                           filesize(p_filesize), hashdigest(p_hashdigest) {
    }
    map_value_t() : filesize(0), hashdigest() {
      // zero out the file hash digest
      for (uint32_t i=0; i<hash_t::size(); i++) {
        hashdigest.digest[i] = 0;
      }
    }
  };

  typedef std::pair<uint64_t, map_value_t> map_element_t;
  typedef boost::btree::btree_map<uint64_t, map_value_t> map_t;

  // settings
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;

  map_t* map;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // disallow these
  source_metadata_manager_t(const source_metadata_manager_t&);
  source_metadata_manager_t& operator=(const source_metadata_manager_t&);

  public:
  source_metadata_manager_t(const std::string p_hashdb_dir,
                           file_mode_type_t p_file_mode_type) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode_type),
       map(),
       M() {

    MUTEX_INIT(&M);

    try {
      map = new map_t(hashdb_dir + "/source_metadata_store",
             file_mode_type_to_btree_flags_bitmask(file_mode));
    } catch(std::exception& e) {
      // something went wrong
      std::cerr << "Unable to open the source metadata store: "
                << e.what() << "\nIf this database was created using v1.0.0, please upgrade it using the 'upgrade' command.\n";
      exit(1);
    }
  }

  ~source_metadata_manager_t() {
    delete map;
  }

  /**
   * Insert and return true but if source_lookup_index is already there,
   * return false.
   */
  bool insert(uint64_t source_id, uint64_t filesize, hash_t hashdigest) {

    MUTEX_LOCK(&M);

    // btree must be writable
    if (file_mode == READ_ONLY) {
      // program error
      assert(0);
    }

    // emplace
    std::pair<map_t::const_iterator, bool> response = map->emplace(
                             source_id, map_value_t(filesize, hashdigest));

    // return success of emplace
    MUTEX_UNLOCK(&M);
    return response.second;
  }

  /**
   * Find source metadata given source lookup index.
   * Return true and source metadata else false and empty source metadata.
   */
  std::pair<bool, source_metadata_t> find(uint64_t source_lookup_index) const {

    MUTEX_LOCK(&M);

    // use map iterator to dereference the source metadata
    map_t::const_iterator it = map->find(source_lookup_index);

    if (it == map->end()) {
      // not there
      MUTEX_UNLOCK(&M);
      return std::pair<bool, source_metadata_t>(false, source_metadata_t());
    } else {
      // compose and return source metadata
      source_metadata_t source_metadata(
                    it->first, it->second.filesize, it->second.hashdigest);
      MUTEX_UNLOCK(&M);
      return std::pair<bool, source_metadata_t>(true, source_metadata);
    }
  }

  // size
  size_t size() const {
    return map->size();
  }
};

#endif
