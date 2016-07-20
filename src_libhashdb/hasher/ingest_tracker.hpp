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
 * Tracks source events during threaded ingest for several reasons:
 *   1) to be able to know if the same source file hash has been processed.
 *   2) to track zero_count and nonprobative_count and store them
 *      when the total is ready.
 * Also tracks total bytes processed in order to provide progress feedback.
 */

#ifndef INGEST_TRACKER_HPP
#define INGEST_TRACKER_HPP

#include <cstring>
#include <sstream>
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
#include "tprint.hpp"

namespace hasher {

class ingest_tracker_t {

  private:
  class source_data_t {
    public:
    const uint64_t filesize;
    std::string file_type;
    const size_t parts_total;
    size_t parts_done;
    size_t zero_count;
    size_t nonprobative_count;
    source_data_t(const uint64_t p_filesize,
                  const std::string& p_file_type,
                  const size_t p_parts_total) :
           filesize(p_filesize), file_type(p_file_type),
           parts_total(p_parts_total), parts_done(0),
           zero_count(0),
           nonprobative_count(0) {
    }
  };
    
  hashdb::import_manager_t* const import_manager;
  std::map<std::string, source_data_t> source_data_map;
  std::set<std::string> preexisting_sources;
  const uint64_t bytes_total;
  uint64_t bytes_done;
  uint64_t bytes_reported_done;
  mutable pthread_mutex_t M;
  
  // do not allow copy or assignment
  ingest_tracker_t(const ingest_tracker_t&);
  ingest_tracker_t& operator=(const ingest_tracker_t&);

  void lock() {
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&M);
  }

  // identify all preexisting sources to skip loading their block hashes
  void identify_preexisting_sources() {
    std::string file_hash = import_manager->first_source();
    while (file_hash != "") {
      preexisting_sources.insert(file_hash);
      file_hash = import_manager->next_source(file_hash);
    }
  }

  public:
  ingest_tracker_t(hashdb::import_manager_t* const p_import_manager,
                   const size_t p_bytes_total) :
               import_manager(p_import_manager),
               source_data_map(),
               preexisting_sources(),
               bytes_total(p_bytes_total),
               bytes_done(0),
               bytes_reported_done(0),
               M() {
    identify_preexisting_sources();
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }

  ~ingest_tracker_t() {
    pthread_mutex_destroy(&M);
  }

  // true so record if added, else false if already there
  bool add_source(const std::string& file_hash, const uint64_t filesize,
                  const std::string& file_type, const size_t parts_total) {
    lock();
    if (preexisting_sources.find(file_hash) != preexisting_sources.end() ||
        source_data_map.find(file_hash) != source_data_map.end()) {
      // already added
      unlock();
      return false;
    } else {
      // add this new source
      source_data_map.insert(std::pair<std::string, source_data_t>(
             file_hash, source_data_t(filesize, file_type, parts_total)));
      unlock();
      return true;
    }
  }

  void track_source(const std::string& file_hash,
                    const uint64_t zero_count,
                    const uint64_t nonprobative_count) {
    lock();

    // update count values in source_data
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
    source_data.zero_count += zero_count;
    source_data.nonprobative_count += nonprobative_count;
    ++source_data.parts_done;
    source_data_map.insert(std::pair<std::string, source_data_t>(
                                                   file_hash, source_data));

    unlock();

    // if this is the final update for this source, add this source data to DB
    if (source_data.parts_done == parts_total) {
      import_manager->insert_source_data(file_hash,
                                         source_data.filesize,
                                         source_data.file_type,
                                         source_data.zero_count,
                                         source_data.nonprobative_count);
    }
  }

  void track_bytes(const uint64_t count) {
    static const size_t INCREMENT = 134217728; // = 2^27 = 100 MiB
    lock();
    bytes_done += count;
    if (bytes_done == bytes_total ||
        bytes_done > bytes_reported_done + INCREMENT) {

      // print %done
      std::stringstream ss;
      ss << "# " << bytes_done
         << " of " << bytes_total
         << " bytes completed (" << bytes_done * 100 / bytes_total
         << "%)\n";
      hashdb::tprint(std::cout, ss.str());


      // next milestone
      bytes_reported_done += INCREMENT;
    }
    unlock();
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
