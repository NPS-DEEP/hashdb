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

#ifndef DUPLICATES_COMMAND_HPP
#define DUPLICATES_COMMAND_HPP

#include "hashdb_settings.hpp"
#include "map_types.h"
#include "file_modes.h"
#include "map_manager.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

/**
 * Support duplicates command.
 */
class duplicates_command_t {
  public:
  template<typename T>
  static void show_duplicates(const std::string& hashdb_dir,
                              uint32_t duplicates) {

    // get hashdb settings in order to use map_manager
    hashdb_settings_t settings;
    hashdb_settings_reader_t::read_settings(hashdb_dir+"/settings.xml", settings);


    // open map_manager
    map_manager_t<T> map_manager(hashdb_dir, READ_ONLY, map_type);
    map_manager_t<T>::map_iterator_t it = map_manager.begin();

    // there is nothing to report if the map is empty
    if (it == map_manager.end()) {
      std::cout << "The map is empty.\n";
      return;
    }

    // iterate over all hashes in map and show duplicates
    // note that *it is std::pair<T, uint64_t>
    size_t i=0;
    while (it != map_manager.end()) {

      // get count for this hash
      uint32_t count = source_lookup_encoding::get_count(it->second);

      if (count == duplicates) {
        // show this hash in form "<index> \t <hexdigest> \n"
        std::cout << ++i << "\t" << it->first << "\t" << count << "\n";
      }
      ++it;
    }

    // indicate if none found
    if (i==0) {
      std::cout << "There are no hash duplicates of count " << duplicates << ".\n";
    }
  }
};

#endif

