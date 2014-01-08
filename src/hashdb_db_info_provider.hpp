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
 * Provides hashdb metadata information as a string.
 */

#ifndef HASHDB_DB_INFO_PROVIDER_HPP
#define HASHDB_DB_INFO_PROVIDER_HPP

//#include "hashdb_db_manager.hpp"
#include "hash_store.hpp"
#include "hashdb_settings.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

/**
 * Provides hashdb metadata information as a string of XML information.
 */
class hashdb_db_info_provider_t {
  public:
  static int get_hashdb_info(const std::string& hashdb_dir, std::string& info) {
    info = "";

    // make output stream
    std::stringstream ss;

// zz not anymore    int status = get_history(hashdb_dir, ss);
    int status = get_statistics(hashdb_dir, ss);

    // return stream as string
    info = ss.str();
    return status;
  }

  private:
  hashdb_db_info_provider_t();

  private:
  // history
  static int get_history(const std::string& hashdb_dir, std::stringstream& ss) {

    // return history which serves as attribution and metadata information

    // get the history filename
    std::string history_filename = hashdb_filenames_t::history_filename(hashdb_dir);

    // open the history filename
    if(access(history_filename.c_str(),R_OK)){
      std::cerr << "Read failure: History file " << history_filename << " is missing or unreadable.\n";
      return -1;
    }

    // get file stream
    std::fstream in(history_filename.c_str());
    if (!in.is_open()) {
      std::cout << "Cannot open " << history_filename << ": " << strerror(errno) << "\n";
      return -1;
    }

    // parse each line
    std::string line;
    bool error = false;
    while(getline(in, line) && !error) {
      ss << line << std::endl;
    }

    // close resource
    in.close();

    return 0;
  }

  // statistics
  static int get_statistics(const std::string& hashdb_dir,
                            std::stringstream& ss) {

    // statistics come from the hash store rather than through the
    // hashdb_db_manager

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // number of unique hashes in the database
    uint64_t unique_hashes = 0;

    // <count,#hashes with this count>
    std::map<uint32_t, uint64_t>* hash_repeats;
    hash_repeats = new std::map<uint32_t, uint64_t>();

    // <hash with highest count, its count>
    std::pair<md5_t, uint32_t> hash_with_highest_count(md5_t(), 0);

//zzTBD    // repositories and their counts
//zzTBD    std::map<std::string, uint64_t>* repositories;
//zzTBD    repositories = new std::map<std::string, uint64_t>();

    // now start processing

    // get hashdb settings
    hashdb_settings_t settings;
    hashdb_settings_reader_t::read_settings(hashdb_dir, settings);

    // path to hash store
    std::string hash_store_path =
               hashdb_filenames_t::hash_store_filename(hashdb_dir);
  
    hash_store_t* hash_store = new hash_store_t(
                         hash_store_path, READ_ONLY,
                         settings.map_type, settings.map_shard_count);

    // now generate statistics from the elements in hash_store
    for (hash_store_t::hash_store_iterator_t it = hash_store->begin(); it!=hash_store->end(); ++it) {
      uint64_t source_lookup_record = it->second;
      uint32_t count = source_lookup_encoding::get_count(source_lookup_record);

      // total hashes is the sum of count values from the hash store
      total_hashes += count;

      // unique hashes is the sum of hashes with count=1 in the hash store
      if (count == 1) {
        ++unique_hashes;
      }

      // hash repeats in map<count,#hashes with this count>
      std::map<uint32_t, uint64_t>::iterator hash_repeats_it = hash_repeats->find(count);
      if (hash_repeats_it == hash_repeats->end()) {
        // first hash found with this count value
        hash_repeats->insert(std::pair<uint32_t, uint64_t>(count, 1));
      } else {
        // increment #hashes with this count
        uint64_t old_number = hash_repeats_it->second;
        hash_repeats->erase(count);
        hash_repeats->insert(std::pair<uint32_t, uint64_t>(count, old_number + 1));
      }

      // hash and its count for the hash with the highest count
      if (count > hash_with_highest_count.second) {
        hash_with_highest_count = std::pair<md5_t, uint32_t>(it->first, count);
      }

      // repositories and their counts
      // TBD
    }

    // now stream the statistics
    ss << "total hashes: " << total_hashes << "\n";
    ss << "unique hashes: " << unique_hashes << "\n";
    ss << "count and #hashes with this count: \n";
    std::map<uint32_t, uint64_t>::iterator hash_repeats_it2;
    for (hash_repeats_it2 = hash_repeats->begin(); hash_repeats_it2 != hash_repeats->end(); ++hash_repeats_it2) {
      ss << "  " << hash_repeats_it2->first;
      ss << "  " << hash_repeats_it2->second << "\n";
    }
    ss << "hash with highest count: hash: " << hash_with_highest_count.first;
    ss << ", count: " << hash_with_highest_count.second << "\n";

    return 0;
  }
};

#endif

