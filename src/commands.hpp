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
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "hashdb_directory_manager.hpp"
#include "history_manager.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_iterator.hpp"
#include "bloom_filter_manager.hpp"
#include "logger.hpp"
#include "dfxml_hashdigest_reader_manager.hpp"
#include "dfxml_hashdigest_writer.hpp"
#include "identified_blocks_reader.hpp"
#include "tcp_server_manager.hpp"
#include "hashdb.hpp"
#include "random_key.hpp"
#include "progress_tracker.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/filesystem.hpp> // for scan_random command

/**
 * Provides the commands that hashdb_manager can execute.
 * This totally static class is a class rather than an namespace
 * so it can have private members.
 */

template<typename T>
class commands_t {
  private:
  // perform intersection, optimized for speed
  static void intersect_optimized(const hashdb_manager_t<T>& smaller_hashdb,
                                  const hashdb_manager_t<T>& larger_hashdb,
                                  hashdb_manager_t<T>& hashdb3,
                                  hashdb_changes_t& changes,
                                  logger_t* logger) {

    // get iterator for smaller db
    hashdb_iterator_t<T> smaller_it = smaller_hashdb.begin();

    // iterate over smaller db
    progress_tracker_t progress_tracker(smaller_hashdb.map_size() +
                                        larger_hashdb.map_size(), logger);
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
          progress_tracker.track();
          ++smaller_it;
        }

        // add hashes from larger
        std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair =
                                      larger_hashdb.find(matching_key);
        while (it_pair.first != it_pair.second) {
          hashdb3.insert(*it_pair.first, changes);
          progress_tracker.track();
          ++it_pair.first;
        }

      } else {
        // no match, so just move on
        progress_tracker.track();
        ++smaller_it;
      }
    }
    progress_tracker.done();
  }

  // generate random scan input that is unlikely to match the hashdb
  static void generate_scan_input(std::vector<T>* scan_input) {
    scan_input->clear();
    for (int i=0; i<100000; i++) {
      scan_input->push_back(random_key<T>());
    }
  }

  // generate random scan input that matches the hashdb
  static void generate_scan_input(hashdb_manager_t<T>* hashdb_manager,
                      std::vector<T>* scan_input) {

    // the hashdb must not be empty
    if (hashdb_manager->map_size() == 0) {
      std::cerr << "Map is empty.  Aborting.\n";
      exit(1);
    }

    scan_input->clear();
    for (int i=0; i<100000; i++) {
T temp = random_key<T>();

      std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair =
//                                     hashdb_manager->find(random_key<T>());
                                     hashdb_manager->find(temp);
      if (it_pair.first == hashdb_manager->end()) {
        // the random hash is greater than anything in hashdb so use first hash
        scan_input->push_back(hashdb_manager->begin()->key);
      } else {
        // good, use the hash that is not less than the random hash
        scan_input->push_back(it_pair.first->key);
      }
//std::cout << "key: " << temp.hexdigest() << ", using " << scan_input->back() << "\n";
    }
  }

  static void require_compatibility(const hashdb_manager_t<T>& hashdb1,
                                    const hashdb_manager_t<T>& hashdb2) {

    // databases should not be the same one
    if (hashdb1.hashdb_dir == hashdb2.hashdb_dir) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb1.hashdb_dir << "', '"
                << hashdb2.hashdb_dir << "'\n"
                << "Aborting.\n";
      exit(1);
    }

    // hash block size must match
    if (hashdb1.settings.hash_block_size != hashdb2.settings.hash_block_size) {
      std::cerr << "Error: the databases have unequal hash block sizes.\n"
                << hashdb1.hashdb_dir
                << " hash block size: " << hashdb1.settings.hash_block_size
                << "\n" << hashdb2.hashdb_dir
                << " hash block size: " << hashdb2.settings.hash_block_size
                << "\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  static void require_compatibility(const hashdb_manager_t<T>& hashdb1,
                                    const hashdb_manager_t<T>& hashdb2,
                                    const hashdb_manager_t<T>& hashdb3) {

    // databases should not be the same one
    if (hashdb1.hashdb_dir == hashdb2.hashdb_dir
     || hashdb2.hashdb_dir == hashdb3.hashdb_dir
     || hashdb1.hashdb_dir == hashdb3.hashdb_dir) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb1.hashdb_dir << "', '"
                << hashdb2.hashdb_dir << "', '"
                << hashdb3.hashdb_dir << "'\n"
                << "Aborting.\n";
      exit(1);
    }

    // hash block size must match
    if (hashdb1.settings.hash_block_size != hashdb2.settings.hash_block_size
     || hashdb1.settings.hash_block_size != hashdb3.settings.hash_block_size) {
      std::cerr << "Error: the databases have unequal hash block sizes.\n"
                << hashdb1.hashdb_dir
                << " hash block size: " << hashdb1.settings.hash_block_size
                << "\n" << hashdb2.hashdb_dir
                << " hash block size: " << hashdb2.settings.hash_block_size
                << "\n" << hashdb3.hashdb_dir
                << " hash block size: " << hashdb3.settings.hash_block_size
                << "\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  public:

  // create
  static void create(const hashdb_settings_t& settings,
                     const std::string& hashdb_dir) {

    // create the hashdb directory
    hashdb_directory_manager_t::create_new_hashdb_dir(hashdb_dir);

    // write the settings
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // get hashdb files to exist because other file modes require them
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, RW_NEW);

    // log the creation event
    logger_t logger(hashdb_dir, "create");
    logger.add("hashdb_dir", hashdb_dir);

    // close logger
    logger.add_hashdb_settings(settings);
    logger.close();
  }

  // import
  static void import(const std::string& repository_name,
                     const std::string& dfxml_file,
                     const std::string& hashdb_dir) {

    // require that dfxml_file exists
    if (access(dfxml_file.c_str(), F_OK) != 0) {
      std::cerr << "DFXML File '" << dfxml_file
                << "' does not exist.  Aborting.\n";
      exit(1);
    }

    hashdb_manager_t<T> hashdb_manager(hashdb_dir, RW_MODIFY);
    hashdb_changes_t changes;

    logger_t logger(hashdb_dir, "import");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);

    logger.add_timestamp("begin reading DFXML file");
    dfxml_hashdigest_reader_manager_t<T> reader_manager(dfxml_file, repository_name);
    typename dfxml_hashdigest_reader_manager_t<T>::const_iterator it = reader_manager.begin();

    logger.add_timestamp("begin import");

    // start progress tracker
    progress_tracker_t progress_tracker(reader_manager.size(), &logger);

    // insert each entry from *it into the hashdb
    while (it != reader_manager.end()) {
      // update progress tracker
      progress_tracker.track();

      hashdb_manager.insert(*it, changes);
      ++it;
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end import");
    logger.add_hashdb_changes(changes);
    logger.close();

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // export
  static void do_export(const std::string& hashdb_dir,
                        const std::string& dfxml_file) {
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // lets require that dfxml_file does not exist yet
    if (access(dfxml_file.c_str(), F_OK) == 0) {
      std::cerr << "File '" << dfxml_file << "' already exists.  Aborting.\n";
      exit(1);
    }

    // open the dfxml_file
    dfxml_hashdigest_writer_t<T> writer(dfxml_file);

    hashdb_iterator_t<T> it = hashdb_manager.begin();
    progress_tracker_t progress_tracker(hashdb_manager.map_size());

    while (it != hashdb_manager.end()) {
      progress_tracker.track();
      writer.add_hashdb_element(*it);
      ++it;
    }
    progress_tracker.done();
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {

    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2);

    hashdb_changes_t changes;
    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();

    logger_t logger(hashdb_dir2, "add");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add");
    progress_tracker_t progress_tracker(hashdb_manager1.map_size(), &logger);
    while (it1 != hashdb_manager1.end()) {
      progress_tracker.track();
      hashdb_manager2.insert(*it1, changes);
      ++it1;
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add");
    logger.add_hashdb_changes(changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // add_multiple A and B to C
  static void add_multiple(const std::string& hashdb_dir1,
                           const std::string& hashdb_dir2,
                           const std::string& hashdb_dir3) {

    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager3(hashdb_dir3, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2, hashdb_manager3);

    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();
    hashdb_iterator_t<T> it2 = hashdb_manager2.begin();
    hashdb_iterator_t<T> it1_end = hashdb_manager1.end();
    hashdb_iterator_t<T> it2_end = hashdb_manager2.end();

    hashdb_changes_t changes;
    logger_t logger(hashdb_dir3, "add_multiple");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin add_multiple");

    // while elements are in both, insert ordered by key, prefering it1 first
    progress_tracker_t progress_tracker(hashdb_manager1.map_size() +
                                        hashdb_manager2.map_size(), &logger);
    while ((it1 != it1_end) && (it2 != it2_end)) {
      if (it1->key.hexdigest() <= it2->key.hexdigest()) {
        hashdb_manager3.insert(*it1, changes);
        ++it1;
      } else {
        hashdb_manager3.insert(*it2, changes);
        ++it2;
      }
      progress_tracker.track();
    }

    // hashdb1 or hashdb2 has become depleted so insert remaining elements
    while (it1 != it1_end) {
      hashdb_manager3.insert(*it1, changes);
      ++it1;
      progress_tracker.track();
    }
    while (it2 != it2_end) {
      hashdb_manager3.insert(*it2, changes);
      ++it2;
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_multiple");
    logger.add_hashdb_changes(changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& hashdb_dir3) {

    // resources
    const hashdb_manager_t<T> manager1(hashdb_dir1, READ_ONLY);
    const hashdb_manager_t<T> manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> manager3(hashdb_dir3, RW_MODIFY);
    require_compatibility(manager1, manager2, manager3);
    hashdb_changes_t changes;

    logger_t logger(hashdb_dir3, "intersect");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect");

    // optimize processing based on smaller db
    if (manager1.map_size() <= manager2.map_size()) {
      intersect_optimized(manager1, manager2, manager3, changes, &logger);
    } else {
      intersect_optimized(manager2, manager1, manager3, changes, &logger);
    }

    // close logger
    logger.add_timestamp("end intersect");
    logger.add_hashdb_changes(changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // subtract: hashdb1 - hashdb 2 -> hashdb3
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& hashdb_dir3) {

    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t<T> hashdb_manager3(hashdb_dir3, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2, hashdb_manager3);
    hashdb_changes_t changes;

    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();

    logger_t logger(hashdb_dir2, "subtract");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin subtract");

    progress_tracker_t progress_tracker(hashdb_manager1.map_size(), &logger);
    while (it1 != hashdb_manager1.end()) {
      
      // subtract or copy the hash
      if (hashdb_manager2.find_count(it1->key) > 0) {
        // hashdb2 has the hash so drop the hash
      } else {
        // hashdb2 does not have the hash so copy it to hashdb3
        hashdb_manager3.insert(*it1, changes);
      }
      ++it1;
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end subtract");
    logger.add_hashdb_changes(changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir1,
                          const std::string& hashdb_dir2) {

    // open resources for hashdb1
    hashdb_manager_t<T> hashdb_manager1(hashdb_dir1, READ_ONLY);

    // if hashdb_dir2 does not exist, create it with the settings of hashdb_dir1
    std::string filename = hashdb_dir2 + "/settings.xml";
    if (access(filename.c_str(), F_OK) != 0) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open resources for hashdb2
    hashdb_manager_t<T> hashdb_manager2(hashdb_dir2, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2);

    // iterate over hashes.
    hashdb_iterator_t<T> it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;

    logger_t logger(hashdb_dir2, "deduplicate");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin deduplicate");

    progress_tracker_t progress_tracker(hashdb_manager1.map_size(), &logger);
    while (it1 != hashdb_manager1.end()) {

      // for deduplicate, only keep hashes whose count=1
      if (hashdb_manager1.find_count(it1->key) == 1) {
        // good, use it
        hashdb_manager2.insert(*it1, changes);
      }

      ++it1;
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end deduplicate");
    logger.add_hashdb_changes(changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";

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
    identified_blocks_reader_iterator_t it;
    for (it = reader.begin(); it != reader.end(); ++it) {
      // check that the hashdigest length is correct
      if (T::size()*2 != it->second.length()) {
        std::cout << "Invalid hashdigest length, hashdigest ignored.\n";
        continue;
      }

      // find matching range for this key
      std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > it_pair =
             hashdb_manager.find(T::fromhex(it->second));
      while (it_pair.first != it_pair.second) {
        // write match to output:
        // offset tab hashdigest tab repository name, filename
        std::cout << it->first << "\t"
                  << it_pair.first->key.hexdigest() << "\t"
                  << "repository_name=" << it_pair.first->repository_name
                  << ",filename=" << it_pair.first->filename
                  << ",file_offset=" << it_pair.first->file_offset
                  << "\n";

        ++it_pair.first;
      }
    }
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

  // show hashdb size values
  static void size(const std::string& hashdb_dir) {
    // open hashdb
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // there is nothing to report if the database is empty
    if (hashdb_manager.map_size() == 0
     && hashdb_manager.source_lookup_store_size() == 0
     && hashdb_manager.repository_name_lookup_store_size() == 0
     && hashdb_manager.filename_lookup_store_size() == 0) {
      std::cout << "The hash database is empty.\n";
      return;
    }

    // print size values
    std::cout << "  hash store: "
              << hashdb_manager.map_size() << "\n"
              << "  source lookup store: "
              << hashdb_manager.source_lookup_store_size() << "\n"
              << "  source repository name store: "
              << hashdb_manager.repository_name_lookup_store_size() << "\n"
              << "  source filename store: "
              << hashdb_manager.filename_lookup_store_size() << "\n";
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

  // show hashdb hash histogram
  static void histogram(const std::string& hashdb_dir) {
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);
    hashdb_iterator_t<T> it = hashdb_manager.begin();

    // there is nothing to report if the map is empty
    if (it == hashdb_manager.end()) {
      std::cout << "The map is empty.\n";
      return;
    }

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_manager.map_size());

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of unique hashes
    uint64_t total_unique_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>* hash_histogram =
                new std::map<uint32_t, uint64_t>();
    
    // iterate over hashdb and set variables for calculating the histogram
    while (it != hashdb_manager.end()) {

      // update progress tracker
      progress_tracker.track();

      // get count for this hash
      uint32_t count = hashdb_manager.find_count(it->key);

      // update totals
      total_hashes += count;
      if (count == 1) {
        ++total_unique_hashes;
      }

      // update hash_histogram information
      // look for existing entry
      std::map<uint32_t, uint64_t>::iterator hash_histogram_it = hash_histogram->find(count);
      if (hash_histogram_it == hash_histogram->end()) {

        // this is the first hash found with this count value
        // so start a new element for it
        hash_histogram->insert(std::pair<uint32_t, uint64_t>(count, 1));

      } else {

        // increment existing value for number of hashes with this count
        uint64_t old_number = hash_histogram_it->second;
        hash_histogram->erase(count);
        hash_histogram->insert(std::pair<uint32_t, uint64_t>(
                                           count, old_number + 1));
      }

      // now move forward by count
      for (uint32_t i=0; i<count; i++) {
        ++it;
      }
    }

    // show final for progress tracker
    progress_tracker.done();

    // show totals
    std::cout << "total hashes: " << total_hashes << "\n"
              << "unique hashes: " << total_unique_hashes << "\n";

    // show hash histogram
//    std::cout << "Histogram of count, number of hashes with count:\n";
    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>::iterator hash_histogram_it2;
    for (hash_histogram_it2 = hash_histogram->begin();
         hash_histogram_it2 != hash_histogram->end(); ++hash_histogram_it2) {
      std::cout << "duplicates=" << hash_histogram_it2->first
                << ", distinct hashes=" << hash_histogram_it2->second
                << ", total=" << hash_histogram_it2->first *
                                 hash_histogram_it2->second << "\n";
    }
    delete hash_histogram;
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

    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);
    hashdb_iterator_t<T> it = hashdb_manager.begin();

    // there is nothing to report if the map is empty
    if (it == hashdb_manager.end()) {
      std::cout << "The map is empty.\n";
      return;
    }

    size_t line_number=0;
    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    while (it != hashdb_manager.end()) {
      uint32_t count = hashdb_manager.find_count(it->key);

      if (count == duplicates_number) {
        // show this hash in form "<index> \t <hexdigest> \n"
        std::cout << ++line_number << "\t" << it->key.hexdigest() << "\t" << count << "\n";
      }

      // now move forward by count
      for (uint32_t i=0; i<count; ++i) {
        ++it;
        progress_tracker.track();
      }
    }
    progress_tracker.done();
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir) {
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);
    hashdb_iterator_t<T> it = hashdb_manager.begin();

    // there is nothing to report if the hash database is empty
    if (it == hashdb_manager.end()) {
      std::cout << "The hash database is empty.\n";
      return;
    }

    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    while (it != hashdb_manager.end()) {
      std::cout << it->key.hexdigest() << "\t"
                << it->repository_name << "\t"
                << it->filename << "\t"
                << it->file_offset << "\n";
      ++it;
      progress_tracker.track();
    }
    progress_tracker.done();
  }

  // rebuild bloom
  static void rebuild_bloom(const hashdb_settings_t& new_bloom_settings,
                            const std::string& hashdb_dir) {

    // read existing settings
    hashdb_settings_t settings;
    settings = hashdb_settings_store_t::read_settings(hashdb_dir);

    // change the bloom filter settings
    settings.bloom1_is_used = new_bloom_settings.bloom1_is_used;
    settings.bloom1_M_hash_size = new_bloom_settings.bloom1_M_hash_size;
    settings.bloom1_k_hash_functions = new_bloom_settings.bloom1_k_hash_functions;
    settings.bloom2_is_used = new_bloom_settings.bloom2_is_used;
    settings.bloom2_M_hash_size = new_bloom_settings.bloom2_M_hash_size;
    settings.bloom2_k_hash_functions = new_bloom_settings.bloom2_k_hash_functions;

    // write back the changed settings
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    logger_t logger(hashdb_dir, "rebuild_bloom");
    logger.add("hashdb_dir", hashdb_dir);

    // log the new settings
    logger.add_hashdb_settings(settings);

    // remove existing bloom files
    std::string filename1 = hashdb_dir + "/bloom_filter_1";
    std::string filename2 = hashdb_dir + "/bloom_filter_2";
    remove(filename1.c_str());
    remove(filename2.c_str());

    // open the bloom filter manager
    bloom_filter_manager_t<T> bloom_filter_manager(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions);

    // open hashdb
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // add hashes to the bloom filter
    logger.add_timestamp("begin rebuild_bloom");
    hashdb_iterator_t<T> it = hashdb_manager.begin();
    progress_tracker_t progress_tracker(hashdb_manager.map_size(), &logger);
    while (it != hashdb_manager.end()) {
      // add the hash to the bloom filter
      bloom_filter_manager.add_hash_value(it->key);
      ++it;
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end rebuild_bloom");
    logger.close();
  }

  // functional analysis and testing: add_random
  static void add_random(const std::string& repository_name,
                     const std::string& hashdb_dir,
                     const std::string& count_string) {

    // initialize random seed
    srand (time(NULL));

    // convert count string to number
    uint64_t count;
    try {
      count = boost::lexical_cast<uint64_t>(count_string);
    } catch(...) {
      std::cerr << "Invalid count: '" << count_string << "'\n";
      exit(1);
    }

    // open resources
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, RW_MODIFY);
    hashdb_changes_t changes;

    // start logger
    logger_t logger(hashdb_dir, "add_random");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add("count", count);
    logger.add_timestamp("begin add_random");

    // start progress tracker
    progress_tracker_t progress_tracker(count, &logger);

    // get hash block size
    size_t hash_block_size = hashdb_manager.settings.hash_block_size;

    // insert count random hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // update progress tracker
      progress_tracker.track();

      // generate filename
      std::stringstream ss;
      ss << "file" << (i>>26);

      // generate file offset
      uint64_t file_offset = (i%(1<<26)) * hash_block_size;

      // add element
      hashdb_manager.insert(hashdb_element_t<T>(random_key<T>(),
                                                hash_block_size,
                                                repository_name,
                                                ss.str(), // filename
                                                file_offset),
                               changes);
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_random");
    logger.add_hashdb_changes(changes);
    logger.close();

    // also write changes to cout
    std::cout << changes << "\n";

    // give user a chance to check memory usage before leaving
    while (true) {
      std::cout << "Done.  Check Memory usage, if desired, then type 'q' to end: ";
      std::string response_string;
      std::getline(std::cin, response_string);
      if (response_string == "q" || response_string == "Q") {
        break;
      }
    }
  }

  // functional analysis and testing: scan_random
  // Performs two scans: first with no matches, then with all matches.
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& hashdb_dir_copy) {

    // open hashdb_dir to use for obtaining valid hash values
    hashdb_manager_t<T> hashdb_manager(hashdb_dir, READ_ONLY);

    // create the hashdb scan service for scanning with random hash
    hashdb_t__<T> hashdb(hashdb_dir);

    // create the hashdb scan service for scanning from the copy
    hashdb_t__<T> hashdb_copy(hashdb_dir_copy);

    // create space on the heap for the scan input and output vectors
    std::vector<T>* scan_input = new std::vector<T>;
    typename hashdb_t__<T>::scan_output_t* scan_output =
                                  new typename hashdb_t__<T>::scan_output_t();

    // initialize random seed
    srand (time(NULL));

    // start logger
    logger_t logger(hashdb_dir, "scan_random");
    logger.add("hashdb_dir", hashdb_dir);

    // scan sets of random hashes where hash values are unlikely to match
    logger.add_timestamp("begin scan_random with random hash on hashdb");
    for (int i=1; i<=100; i++) {
      // generate set of random hashes
      generate_scan_input(scan_input);
      std::stringstream ss1;
      ss1 << "generated random hash " << i;
      logger.add_timestamp(ss1.str());

      // scan set of random hashes
      hashdb.scan(*scan_input, *scan_output);
      std::stringstream ss2;
      ss2 << "scanned random hash " << i;
      logger.add_timestamp(ss2.str());
      std::cout << "scan random hash " << i << " of 100\n";

      // make sure no hashes were found
      if (scan_output->size() > 0) {
        std::cerr << "Unexpected event: match found, count "
                  << scan_output->size() << ", are the databases different?\n";
      }
    }

    // scan sets of random hashes where hash values all match

    // scan sets of random hashes where all hash values match
    logger.add_timestamp("begin scan_random with random matching hashes on hashdb copy");
    for (int j=1; j<=100; j++) {
      // generate set of random hashes that match
      generate_scan_input(&hashdb_manager, scan_input);
      std::stringstream ss1;
      ss1 << "generated random matching hash " << j;
      logger.add_timestamp(ss1.str());

      // scan set of random hashes that match
      hashdb.scan(*scan_input, *scan_output);
      std::stringstream ss2;
      ss2 << "scanned random matching hash " << j;
      logger.add_timestamp(ss2.str());
      std::cout << "scan random matching hash " << j << " of 100\n";

      // make sure every hash was found
      if (scan_output->size() != scan_input->size()) {
        std::cerr << "Unexpected event: match not found, count "
                  << scan_output->size() << ", are the databases different?\n";
      }
    }

    // close logger
    logger.add_timestamp("end scan_random");
    logger.close();
  }
};

#endif

