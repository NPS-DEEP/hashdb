// Author:  Bruce Allen
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
 * Tracks source data by source file hash for two reasons:
 *   1) to know to not re-process the same file hash.
 *   2) to track nonprobative_count to save it when the total is ready.
 *
 * Usage:
 *   seen_source(file_hash) to see if this file hash is processing or is done
 *   add_source(file_hash, source data) to begin processing
 *   update_nonprobative_count(file_hash, nonprobative_count) to update count
 *     and if done, to write the source data to the DB.
 */

#ifndef SOURCE_DATA_MANAGER_HPP
#define SOURCE_DATA_MANAGER_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <map>

namespace hasher {

class source_data_manager_t {

  private:
  hashdb::import_manager_t* const import_manager;

  class source_data_t {
    public:
    const uint64_t filesize;
    std::string file_type;
    const size_t parts_total;
    size_t parts_done;
    size_t nonprobative_count;
    source_data_t(const uint64_t p_filesize,
                  const std::string& p_file_type,
                  const size_t p_parts_total) :
           filesize(p_filesize), file_type(p_file_type),
           parts_total(p_parts_total), parts_done(0),
           nonprobative_count(0) {
    }
  };
    
  std::map<std::string, source_data_t> source_data_map;
  mutable pthread_mutex_t M;
  
  // do not allow copy or assignment
  source_data_manager_t(const source_data_manager_t&);
  source_data_manager_t& operator=(const source_data_manager_t&);

  void lock() {
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&M);
  }

  public:
  source_data_manager_t(hashdb::import_manager_t* const p_import_manager) :
               import_manager(p_import_manager),
               source_data_map(), M() {
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }

  ~source_data_manager_t() {
    pthread_mutex_destroy(&M);
  }

  bool add_source(const std::string& file_hash, const uint64_t filesize,
                  const std::string& file_type, const size_t parts_total) {
    lock();
    if (source_data_map.find(file_hash) != source_data_map.end()) {
      // already added
      unlock();
      return false;
    }
    // add
    source_data_map.insert(std::pair<std::string, source_data_t>(
             file_hash, source_data_t(filesize, file_type, parts_total)));
    unlock();
    return true;
  }

  void update_nonprobative_count(const std::string& file_hash,
                                 const uint64_t nonprobative_count) {
    lock();

    // add nonprobative_count to source_data
    std::map<std::string, source_data_t>::const_iterator it =
                                            source_data_map.find(file_hash);
    if (it == source_data_map.end()) {
      // program error
      assert(0);
    }
    if (it->second.parts_done == it->second.parts_total) {
      // program error
      assert(0);
    }
    source_data_t source_data = it->second;
    const size_t parts_total = source_data.parts_total;
    source_data_map.erase(it);
    source_data.nonprobative_count += nonprobative_count;
    ++source_data.parts_done;
    source_data_map.insert(std::pair<std::string, source_data_t>(
                                                   file_hash, source_data));
    unlock();

    // if this is the final update, add source data to DB
    if (source_data.parts_done == parts_total) {
      import_manager->insert_source_data(file_hash,
                                         source_data.filesize,
                                         source_data.file_type,
                                         source_data.nonprobative_count);
    }
  }

  bool seen_source(const std::string& file_hash) {
    lock();
    bool has_hash = source_data_map.find(file_hash) != source_data_map.end();
    unlock();
    return has_hash;
  }
};

} // end namespace hasher

#endif
