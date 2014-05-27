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
 * Defines the static commands that hashdb_manager can execute.
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include "command_line.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "history_manager.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_iterator.hpp"
#include "bloom_rebuild_manager.hpp"
#include "logger.hpp"
#include "dfxml_hashdigest_reader_manager.hpp"
#include "dfxml_hashdigest_writer.hpp"
#include "identified_blocks_reader.hpp"
#include "tcp_server_manager.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdb.hpp"
#include "statistics_command.hpp"
#include "duplicates_command.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Provides the commands that hashdb_manager can execute.
 * This totally static class is a class rather than an namespace
 * so it can have private members.
 */

// file modes:
// READ_ONLY, RW_NEW, RW_MODIFY

template<typename T>
class commands_t {
  private:
  // perform intersection, optimized for speed
  static void intersect_optimized(const hashdb_manager_t<T>& smaller_hashdb,
                                  const hashdb_manager_t<T>& larger_hashdb,
                                  hashdb_manager_t<T>& hashdb3,
                                  hashdb_changes_t& changes) {

    // get iterator for smaller db
    hashdb_iterator_t<T> smaller_it = smaller_hashdb.begin();

    // iterate over smaller db
    while (smaller_it != smaller_hashdb.end()) {

      // see if hashdigest is in larger db
      uint32_t larger_count = larger_hashdb.find_count(smaller_it->key);

      if (larger_count > 0) {
        // there is a match in larger db
        T matching_key = smaller_it->key;

        // add hashes from smaller
        uint32_t smaller_count = smaller_hashdb.find_count(matching_key);
        for (uint32_t i = 0; i<smaller_count; ++i) {
          hashdb_element_t<T> hashdb_element = *smaller_it;
          hashdb3.insert(hashdb_element, changes);
          ++smaller_it;
        }

        // add hashes from larger
        std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair =
                                      larger_hashdb.find(matching_key);
        while (it_pair.first != it_pair.second) {
          hashdb3.insert(*it_pair.first, changes);
          ++it_pair.first;
        }

      } else {
        // no match, so just move on
        ++smaller_it;
      }
    }
  }

  public:

  // create
  static void create(const hashdb_settings_t& settings,
                     const std::string& hashdb_dir) {
    hashdb_settings_manager_t::write_settings(hashdb_dir, settings);

    // get hashdb files to exist because other file modes require them
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, RW_NEW);

    logger_t logger(hashdb_dir, "create");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add_hashdb_settings(settings);

    // create new history trail
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);
  }

  // import
  static void import(const std::string& repository_name,
                     const std::string& dfxml_file,
                     const std::string& hashdb_dir) {

    logger_t logger(hashdb_dir, "import");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, RW_MODIFY);
    dfxml_hashdigest_reader_manager_t<T> reader_manager(dfxml_file, repository_name);

    typename dfxml_hashdigest_reader_manager_t<T>::const_iterator it = reader_manager.begin();

    hashdb_changes_t changes;

    logger.add_timestamp("begin import");

    // insert each entry from *it into the hashdb
    while (it != reader_manager.end()) {
      hashdb_manager.insert(*it, changes);
      ++it;
    }
    logger.add_timestamp("end import");
    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // export
  static void do_export(const std::string& hashdb_dir,
                        const std::string& dfxml_file) {
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);
    dfxml_hashdigest_writer_t<T> writer(dfxml_file);
    hashdb_iterator_t<T> it = hashdb_manager.begin();
    while (it != hashdb_manager.end()) {
      writer.add_hashdb_element(*it);
      ++it;
    }
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {

    logger_t logger(hashdb_dir2, "add");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, RW_MODIFY);

    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin add");
    while (it1 != hashdb_manager1.end()) {
      hashdb_manager2.insert(*it1, changes);
      ++it1;
    }
    logger.add_timestamp("end add");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // add_multiple A and B to C
  static void add_multiple(const std::string& hashdb_dir1,
                           const std::string& hashdb_dir2,
                           const std::string& hashdb_dir3) {

    logger_t logger(hashdb_dir3, "add_multiple");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager3(hashdb_dir3, RW_MODIFY);

    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();
    hashdb_iterator_t<T> it2 = hashdb_manager2.begin();
    hashdb_iterator_t<T> it1_end = hashdb_manager1.end();
    hashdb_iterator_t<T> it2_end = hashdb_manager2.end();

    hashdb_changes_t changes;
    logger.add_timestamp("begin add_multiple");

    // while elements are in both, insert ordered by key, prefering it1 first
    while ((it1 != it1_end) && (it2 != it2_end)) {
      if (it1->key.hexdigest() <= it2->key.hexdigest()) {
        hashdb_manager3.insert(*it1, changes);
        ++it1;
      } else {
        hashdb_manager3.insert(*it2, changes);
        ++it2;
      }
    }

    // hashdb1 or hashdb2 has become depleted so insert remaining elements
    while (it1 != it1_end) {
      hashdb_manager3.insert(*it1, changes);
      ++it1;
    }
    while (it2 != it2_end) {
      hashdb_manager3.insert(*it2, changes);
      ++it2;
    }

    logger.add_timestamp("end add_multiple");
    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& hashdb_dir3) {

    logger_t logger(hashdb_dir3, "intersect");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);

    // resources
    const hashdb_manager_t<T> manager1(hashdb_dir1, READ_ONLY);
    const hashdb_manager_t<T> manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> manager3(hashdb_dir3, RW_MODIFY);
    hashdb_changes_t changes;

    logger.add_timestamp("begin intersect");

    // optimise processing based on smaller db
    if (manager1.map_size() <= manager2.map_size()) {
      intersect_optimized(manager1, manager2, manager3, changes);
    } else {
      intersect_optimized(manager2, manager1, manager3, changes);
    }

    logger.add_timestamp("end intersect");
    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // subtract: hashdb1 - hashdb 2 -> hashdb3
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& hashdb_dir3) {

    logger_t logger(hashdb_dir2, "subtract");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager3(hashdb_dir3, RW_MODIFY);
    hashdb_changes_t changes;

    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();

    logger.add_timestamp("begin subtract");

    while (it1 != hashdb_manager1.end()) {
      
      // subtract or copy the hash
      if (hashdb_manager2.find_count(it1->key) > 0) {
        // hashdb2 has the hash so drop the hash
      } else {
        // hashdb2 does not have the hash so copy it to hashdb3
        hashdb_manager3.insert(*it1, changes);
      }
      ++it1;
    }

    logger.add_timestamp("end subtract");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir1,
                          const std::string& hashdb_dir2) {

    // Use map_manager to iterate over hashes.

    logger_t logger(hashdb_dir2, "deduplicate");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    map_manager_t<T> map_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, RW_MODIFY);

    // get resources for hashdb_element lookup
    source_lookup_index_manager_t source_lookup_index_manager(hashdb_dir1, READ_ONLY);
    hashdb_settings_t settings = hashdb_settings_manager_t::read_settings(hashdb_dir1);
    hashdb_element_lookup_t<T> hashdb_element_lookup(
                          &source_lookup_index_manager, &settings);

    typename map_manager_t<T>::map_iterator_t map_it1 = map_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin deduplicate");
    while (map_it1 != map_manager1.end()) {

      // for deduplicate, only keep hashes whose count=1
      if (source_lookup_encoding::get_count(map_it1->second) == 1) {
        // good, use it
        hashdb_manager2.insert(hashdb_element_lookup(*map_it1), changes);
      }

      ++map_it1;
    }

    logger.add_timestamp("end deduplicate");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";

  }

  // rebuild bloom
  static void rebuild_bloom(const hashdb_settings_t& settings,
                            const std::string& hashdb_dir) {

    logger_t logger(hashdb_dir, "rebuild_bloom");
    logger.add("hashdb_dir", hashdb_dir);

    // start the bloom rebuild manager
    bloom_rebuild_manager_t<T> bloom_rebuild_manager(hashdb_dir, settings);

    // log the revised settings
    hashdb_settings_t revised_settings = hashdb_settings_manager_t::read_settings(hashdb_dir);
    logger.add_hashdb_settings(revised_settings);

    // get the map manager and its iterator
    map_manager_t<T> map_manager(hashdb_dir, READ_ONLY);
    typename map_manager_t<T>::map_iterator_t it = map_manager.begin();

    logger.add_timestamp("begin rebuild_bloom");
    while (it != map_manager.end()) {
      // add the hash to the bloom filter
      bloom_rebuild_manager.add_hash_value(it->first);
      ++it;
    }
    logger.add_timestamp("end rebuild_bloom");

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);
  }

  // server
  static void server(const std::string& hashdb_dir,
                     const std::string& port_number_string) {

    uint16_t port_number;
    try {
      port_number = boost::lexical_cast<uint16_t>(port_number_string);
    } catch(...) {
      std::cerr << "Invalid port: '" << port_number_string << "'\n";
      exit(1);
    }

    // start the server
    std::cout << "Starting the hashdb server scan service.  Press Ctrl-C to quit.\n";
    tcp_server_manager_t<T> tcp_server_manager(hashdb_dir, port_number);
//    std::cout << "The hashdb service server is running.  Press Ctrl-C to quit.\n";
/*
    std::cout << "The hashdb service server is running.  Press Return to quit.\n";
    std::cout << "The hashdb service server is running.  Press Return to quit.\n";
    std::string buffer;
    getline(std::cin, buffer);
    std::cout << "Done.\n";
*/
  }

  // scan
  static void scan(const std::string& path_or_socket,
                   const std::string& dfxml_file) {

    // get dfxml reader
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_manager_t<T> reader_manager(dfxml_file, repository_name);

    // done if no entries
    if (reader_manager.begin() == reader_manager.end()) {
      std::cout << "No entries in DFXML file\n";
      return;
    }

    // create space on the heap for the scan input and output vectors
    std::vector<T>* scan_input = new std::vector<T>;
    typename hashdb_t__<T>::scan_output_t* scan_output = new typename hashdb_t__<T>::scan_output_t();

    // create the hashdb scan service
    hashdb_t__<T> hashdb(path_or_socket);

    // get dfxml block hash entries into the scan_input request vector
    typename dfxml_hashdigest_reader_manager_t<T>::const_iterator it = reader_manager.begin();
    while (it != reader_manager.end()) {
      scan_input->push_back(it->key);
      ++it;
    }

    // perform the scan
    hashdb.scan(*scan_input, *scan_output);

    // show hash values that match
    typename hashdb_t__<T>::scan_output_t::const_iterator it2(scan_output->begin());
    while (it2 != scan_output->end()) {
      // print: '<index> \t <hexdigest> \t <count> \n' where count>0
      if (it2->second > 0) {
        std::cout << it2->first << "\t"
                  << (*scan_input)[it2->first] << "\t" // hexdigest
                  << it2->second << "\n";
      }
      ++it2;
    }

    // delete heap allocation
    delete scan_input;
    delete scan_output;
  }

  // scan expanded, does not use socket
  static void scan_expanded(const std::string& hashdb_dir,
                            const std::string& dfxml_file) {

    // open hashdb
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // get dfxml reader
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_manager_t<T> reader_manager(dfxml_file, repository_name);

    typename dfxml_hashdigest_reader_manager_t<T>::const_iterator it = reader_manager.begin();

    // search hashdb for hash values in dfxml
    while (it != reader_manager.end()) {

      uint32_t count = hashdb_manager.find_count(it->key);
      if (count == 0) {
        ++it;
        continue;
      }

      // get range of hash match
      std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > range_it_pair(
                                             hashdb_manager.find(it->key));
//      hashdb_iterator_t<T> hashdb_it = range_it_pair.first;
//      hashdb_iterator_t<T> hashdb_it_end = range_it_pair.second;

      while (range_it_pair.first != range_it_pair.second) {
        hashdb_element_t<T> e = *(range_it_pair.first);
        std::cout << e.key.hexdigest() << "\t"
                  << "repository name='" << e.repository_name
                  << "', filename='" << e.filename << "\n";
        ++range_it_pair.first;
      }
      ++it;
    }
  }

  // expand identified_blocks.txt
  static void expand_identified_blocks(const std::string& hashdb_dir,
                            const std::string& identified_blocks_file) {

    // open hashdb
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // get the identified_blocks.txt file reader
    identified_blocks_reader_t reader(identified_blocks_file);

    // read identified blocks from input and write out matches
    identified_blocks_reader_iterator_t it = reader.begin();
    while(it != reader.end()) {
      std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair =
             hashdb_manager.find(T::fromhex(it->second));
      while (it_pair.first != it_pair.second) {
        // write match to output:
        // offset tab hashdigest tab repository name, filename
        std::cout << it->first << "\t"
                  << it_pair.first->key.hexdigest() << "\t"
                  << "repository name=" << it_pair.first->repository_name
                  << ",filename=" << it_pair.first->filename
                  << ",file offset=" << it_pair.first->file_offset
                  << "\n";

        ++it_pair.first;
      }
      ++it;
    }
  }

  // print sources referenced in this database
  static void sources(const std::string& hashdb_dir) {

    // open the source lookup index manager for hashdb_dir
    source_lookup_index_manager_t manager(hashdb_dir, READ_ONLY);
    source_lookup_index_iterator_t it = manager.begin();

    // there is nothing to report if the source lookup index map is empty
    if (it == manager.end()) {
      std::cout << "The source lookup index map is empty.\n";
      return;
    }

    // report each entry
    while (it != manager.end()) {
      std::cout << "repository name='" << it->first
                << "', filename='" << it->second << "'\n";
      ++it;
    }
  }

  // show hashdb size values
  static void size(const std::string& hashdb_dir) {
    // open hashdb
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // there is nothing to report if the database is empty
    if (hashdb_manager.map_size() == 0
     && hashdb_manager.multimap_size() == 0
     && hashdb_manager.source_lookup_store_size() == 0
     && hashdb_manager.repository_name_lookup_store_size() == 0
     && hashdb_manager.filename_lookup_store_size() == 0) {
      std::cout << "The hash database is empty.\n";
      return;
    }

    // print size values
    std::cout << "  hash store: "
              << hashdb_manager.map_size() << "\n"
              << "  hash duplicates store: "
              << hashdb_manager.multimap_size() << "\n"
              << "  source lookup store: "
              << hashdb_manager.source_lookup_store_size() << "\n"
              << "  source repository name store: "
              << hashdb_manager.repository_name_lookup_store_size() << "\n"
              << "  source filename store: "
              << hashdb_manager.filename_lookup_store_size() << "\n";
  }

  // show hashdb statistics
  static void statistics(const std::string& hashdb_dir) {
    statistics_command_t::show_statistics<T>(hashdb_dir);
  }

  // show hashdb duplicates for a given duplicates count
  static void duplicates(const std::string& hashdb_dir,
                         const std::string& duplicates_string) {

    // convert duplicates string to number
    uint32_t duplicates_number;
    try {
      duplicates_number = boost::lexical_cast<uint32_t>(duplicates_string);
    } catch(...) {
      std::cerr << "Invalid number of duplicates: '" << duplicates_string << "'\n";
      exit(1);
    }
    duplicates_command_t<T>::show_duplicates(hashdb_dir, duplicates_number);
  }
};

#endif

