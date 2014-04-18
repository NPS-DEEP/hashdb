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

#ifndef STATISTICS_MANAGER_HPP
#define STATISTICS_MANAGER_HPP

#include "hashdb_settings.hpp"
#include "map_types.h"
#include "file_modes.h"
#include "map_manager.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

/**
 * Provides hashdb metadata information as a string of XML information.
 */
class statistics_manager_t {
  public:
  static void show_statistics(const std::string& hashdb_dir) {

    // show histogram statistic
    show_histogram(hashdb_dir);
  }

  private:
  static void show_histogram(const std::string& hashdb_dir) {

    // get hashdb settings in order to use map_manager
    hashdb_settings_t settings;
    hashdb_settings_reader_t::read_settings(hashdb_dir+"/settings.xml", settings);

    // open map_manager
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        show_histogram<md5_t>(hashdb_dir, settings.map_type); return;
      case HASHDIGEST_SHA1:
        show_histogram<sha1_t>(hashdb_dir, settings.map_type); return;
      case HASHDIGEST_SHA256:
        show_histogram<sha256_t>(hashdb_dir, settings.map_type); return;
      default: assert(0); exit(1);
    }
  }

  template<typename T>
  static void show_histogram(const std::string& hashdb_dir,
                             map_type_t map_type) {

    // use map_manager
    map_manager_t<T> map_manager(hashdb_dir, READ_ONLY, map_type);

    // total number of hashes in the database
    size_t total_hashes = 0;

    // total number of unique hashes
    size_t total_unique_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, size_t>* hash_histogram =
                new std::map<uint32_t, size_t>();
    
    // iterate over all hashes in map and set statistics variables
    // note that *it is std::pair<T, uint64_t>
    for (map_iterator_t<T> it = map_manager.begin();
              it != map_manager.end(); ++it) {

      // get count for this hash
      uint32_t count = source_lookup_encoding::get_count(it->second);

      // update totals
      total_hashes += count;
      if (count == 1) {
        ++total_unique_hashes;
      }

      // update hash_histogram information
      // look for existing entry
      std::map<uint32_t, size_t>::iterator hash_histogram_it = hash_histogram->find(count);
      if (hash_histogram_it == hash_histogram->end()) {

        // this is the first hash found with this count value
        hash_histogram->insert(std::pair<uint32_t, size_t>(count, 1));

      } else {

        // increment #hashes with this count
        size_t old_number = hash_histogram_it->second;
        hash_histogram->erase(count);
        hash_histogram->insert(std::pair<uint32_t, size_t>(
                                           count, old_number + 1));
      }
    }

    // now show the statistics

    // totals
    std::cout << "total hashes: " << total_hashes << "\n"
              << "unique hashes: " << total_unique_hashes << "\n";

    // histogram
    std::cout << "Histogram of count, #hashes with count:\n";
    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>::iterator hash_histogram_it2;
    for (hash_histogram_it2 = hash_histogram->begin();
         hash_histogram_it2 != hash_histogram->end(); ++hash_histogram_it2) {
      std::cout << ", " << hash_histogram_it2->first << "\n"
                << ", " << hash_histogram_it2->second << "\n";
    }
    std::cout << "\n";
  }
};

#endif

