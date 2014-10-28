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
#include "dfxml_hashdigest_reader.hpp"
#include "dfxml_import_hash_consumer.hpp"
#include "dfxml_import_source_metadata_consumer.hpp"
#include "dfxml_scan_hash_consumer.hpp"
#include "dfxml_scan_source_metadata_consumer.hpp"
#include "dfxml_scan_expanded_smc.hpp"
#include "dfxml_hashdigest_writer.hpp"
#include "source_metadata_manager.hpp"
#include "tcp_server_manager.hpp"
#include "hashdb.hpp"
#include "random_key.hpp"
#include "progress_tracker.hpp"
#include "hash_t_selector.h"
#include "feature_file_reader.hpp"
#include "feature_line.hpp"
#include "json_helper.hpp"

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

class commands_t {
  // support expalin_identified_blocks
  typedef std::map<hash_t, std::string> hashes_t;

  private:
  // perform intersection, optimized for speed
  static void intersect_optimized(const hashdb_manager_t& smaller_hashdb,
                                  const hashdb_manager_t& larger_hashdb,
                                  hashdb_manager_t& hashdb3,
                                  hashdb_changes_t& changes,
                                  logger_t* logger) {

    // get iterator for smaller db
    hashdb_iterator_t smaller_it = smaller_hashdb.begin();

    // iterate over smaller db
    progress_tracker_t progress_tracker(smaller_hashdb.map_size() +
                                        larger_hashdb.map_size(), logger);
    while (smaller_it != smaller_hashdb.end()) {

      // get iterator for this hash in the larger db
      std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
                                        larger_hashdb.find(smaller_it->key);

      // iterate through hash matches in larger db to find exact source matches
      for (; it_pair.first != it_pair.second; ++it_pair.first) {
        if (*smaller_it == *(it_pair.first)) {
          // the hashdb_element is an exact match so add it to hashdb3
          hashdb3.insert(*smaller_it, changes);
        }
        progress_tracker.track();
      }
      progress_tracker.track();
      ++smaller_it;
    }
    progress_tracker.done();
  }

  // perform hash intersection, optimized for speed
  static void intersect_hash_optimized(const hashdb_manager_t& smaller_hashdb,
                                       const hashdb_manager_t& larger_hashdb,
                                       hashdb_manager_t& hashdb3,
                                       hashdb_changes_t& changes,
                                       logger_t* logger) {

    // get iterator for smaller db
    hashdb_iterator_t smaller_it = smaller_hashdb.begin();

    // iterate over smaller db
    progress_tracker_t progress_tracker(smaller_hashdb.map_size() +
                                        larger_hashdb.map_size(), logger);
    while (smaller_it != smaller_hashdb.end()) {

      // see if hashdigest is in larger db
      uint32_t larger_count = larger_hashdb.find_count(smaller_it->key);

      if (larger_count > 0) {
        // there is a match in larger db
        hash_t matching_key = smaller_it->key;

        // add hashes from smaller
        uint32_t smaller_count = smaller_hashdb.find_count(matching_key);
        for (uint32_t i = 0; i<smaller_count; ++i) {
          hashdb_element_t hashdb_element = *smaller_it;
          hashdb3.insert(hashdb_element, changes);
          progress_tracker.track();
          ++smaller_it;
        }

        // add hashes from larger
        std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
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
  static void generate_scan_input(std::vector<hash_t>* scan_input) {
    scan_input->clear();
    for (int i=0; i<100000; i++) {
      scan_input->push_back(random_key());
    }
  }

  // generate random scan input that matches the hashdb
  static void generate_scan_input(hashdb_manager_t* hashdb_manager,
                      std::vector<hash_t>* scan_input) {

    // the hashdb must not be empty
    if (hashdb_manager->map_size() == 0) {
      std::cerr << "Map is empty.  Aborting.\n";
      exit(1);
    }

    scan_input->clear();
    for (int i=0; i<100000; i++) {
    hash_t temp = random_key();

      std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
//                                     hashdb_manager->find(random_key());
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

  static void require_compatibility(const hashdb_manager_t& hashdb1,
                                    const hashdb_manager_t& hashdb2) {

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

  static void require_compatibility(const hashdb_manager_t& hashdb1,
                                    const hashdb_manager_t& hashdb2,
                                    const hashdb_manager_t& hashdb3) {

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

  // print the scan output vector
  static void print_scan_output(
              const std::vector<hash_t>& scan_input,
              const typename hashdb_t__<hash_t>::scan_output_t& scan_output) {

    // check that there are matches
    if (scan_output.size() == 0) {
      std::cout << "There are no matches.\n";
      return;
    }

    // print matches
    for (hashdb_t__<hash_t>::scan_output_t::const_iterator it=scan_output.begin(); it != scan_output.end(); ++it) {
      std::cout << "[\"" << scan_input[it->first] << "\",{\"count\":" << it->second << "}]\n";
    }
  }

  // ingest table of relevant hashes and table of their sources
  static void identify_hashes_and_sources(
                            const hashdb_manager_t& hashdb_manager,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max,
                            hashes_t& hashes,
                            std::set<uint64_t>& source_lookup_indexes) {

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);
    while (!reader.at_eof()) {

      feature_line_t feature_line = reader.read();

      // validate the hash string
      std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(feature_line.feature);
      if (hash_pair.first == false) {
        // bad hash value
        continue;
      }
      hash_t hash = hash_pair.second;

      // skip if hash already seen
      if (hashes.find(hash) != hashes.end()) {
        continue;
      }

      // skip if hash count > requested max
      if (hashdb_manager.find_count(hash) > requested_max) {
        continue;
      }

      // the hash is interesting
      hashes.insert(std::pair<hash_t, std::string>(hash, feature_line.context));

      // get the iterator for this hash value
      std::pair<hashdb_manager_t::multimap_iterator_t,
                hashdb_manager_t::multimap_iterator_t> it_pair =
                                  hashdb_manager.find_native(hash);

      // note the source lookup index for all sources associated with this hash
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get the source lookup index for this entry
        uint64_t source_lookup_encoding = (it_pair.first)->second;
        uint64_t source_lookup_index = source_lookup_encoding::get_source_lookup_index(source_lookup_encoding);

        // add the source lookup index to the source lookup index set
        source_lookup_indexes.insert(source_lookup_index);
      }
    }
  }

  // remove "count":NN[,]? field from string
  static void remove_count_field(std::string& context) {
    // find the '"count":' field, the '}' and any ','.
    size_t pos_count = context.find("\"count\":");
    size_t pos_closebrace =  context.find("}");
    size_t pos_comma = context.find(",");

    // validate input
    if (pos_count == std::string::npos) {
      // no count field present.
      std::cerr << "Unexpected input: no count field found.\n";
      return;
    }
    if (pos_closebrace == std::string::npos) {
      // no close brace present.
      std::cerr << "Unexpected input: no close brace found.\n";
      return;
    }

    // remove the count field
    if (pos_comma < pos_closebrace) {
      // remove count field up to and including the comma
      context.erase(pos_count, pos_comma - pos_count+1);
    } else {
      // remove count field up to but not including the close brace
      context.erase(pos_count, pos_closebrace - pos_count);
    }
  }

  // print table of relevant hashes
  static void print_identified_hashes(
                            const hashdb_manager_t& hashdb_manager,
                            hashes_t& hashes) {

    if (hashes.size() == 0) {
      std::cout << "# There are no hashes to report.\n";
      return;
    }

    // iterate through block hashes
    for (hashes_t::iterator it = hashes.begin(); it != hashes.end(); ++it) {

      // print this block hash
      std::cout << "[\"" << it->first << "\"";

      // get the context field without the count, specifically, just the flags
      std::string context = it->second;
      remove_count_field(context);

      // print the reduced context field
      std::cout << "," << context;

      // print the source array open bracket
      std::cout << ",[";

      // get the multimap iterator for this hash value
      std::pair<hashdb_manager_t::multimap_iterator_t,
                hashdb_manager_t::multimap_iterator_t> it_pair =
                                  hashdb_manager.find_native(it->first);

      // track when to put in the comma
      bool is_first_round = true;

      // print sources associated with this hash value
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get the source lookup index and file offset for this entry
        uint64_t source_lookup_encoding = (it_pair.first)->second;
        uint64_t source_lookup_index = source_lookup_encoding::get_source_lookup_index(source_lookup_encoding);
        uint64_t file_offset = source_lookup_encoding::get_file_offset(source_lookup_encoding);

        // maybe add comma
        if (is_first_round == true) {
          // not first round anymore
          is_first_round = false;
        } else {
          // prepend comma
          std::cout << ",";
        }

        // add source_id and file_offset entry
        std::cout << "{\"source_id\":" << source_lookup_index
                  << ",\"file_offset\":" << file_offset
                  << "}";
      }

      // done printing the source array
      std::cout << "]";

      // done printing this block hash
      std::cout << "]\n";
    }
  }

  // print table of relevant sources
  static void print_identified_sources(
                            const hashdb_manager_t& hashdb_manager,
                            std::set<uint64_t>& source_lookup_indexes) {

    if (source_lookup_indexes.size() == 0) {
      std::cout << "# There are no sources to report.\n";
      return;
    }

    // iterate through sources
    for (std::set<uint64_t>::iterator it = source_lookup_indexes.begin();
                                   it!=source_lookup_indexes.end(); ++it) {

      // print the source
      std::cout << "{";
      json_helper_t::print_source_fields(hashdb_manager, *it);
      std::cout << "}\n";
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
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_NEW);

    // log the creation event
    logger_t logger(hashdb_dir, "create");
    logger.add("hashdb_dir", hashdb_dir);

    // close logger
    logger.add_hashdb_settings(settings);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);

    std::cout << "Database created.\n";
  }

  // import
  static void import(const std::string& hashdb_dir,
                     const std::string& dfxml_file,
                     const std::string& repository_name) {

    // require that dfxml_file exists
    if (access(dfxml_file.c_str(), F_OK) != 0) {
      std::cerr << "DFXML File '" << dfxml_file
                << "' does not exist.  Aborting.\n";
      exit(1);
    }

    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);
    hashdb_changes_t changes;

    logger_t logger(hashdb_dir, "import");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add_timestamp("begin import");

    // start progress tracker
    progress_tracker_t progress_tracker(0, &logger);

    // create the hash consumer
    dfxml_import_hash_consumer_t hash_consumer(
                                &hashdb_manager, &changes, &progress_tracker);

    // create the source metadata consumer
    dfxml_import_source_metadata_consumer_t source_metadata_consumer(
                                &hashdb_manager, &changes);

    // run the dfxml hashdigest reader using the import hash consumer
    dfxml_hashdigest_reader_t<dfxml_import_hash_consumer_t,
                              dfxml_import_source_metadata_consumer_t>::
             do_read(dfxml_file, repository_name,
                     &hash_consumer, &source_metadata_consumer);

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end import");
    logger.add_hashdb_changes(changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // export
  static void do_export(const std::string& hashdb_dir,
                        const std::string& dfxml_file) {
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // lets require that dfxml_file does not exist yet
    if (access(dfxml_file.c_str(), F_OK) == 0) {
      std::cerr << "File '" << dfxml_file << "' already exists.  Aborting.\n";
      exit(1);
    }

    // open the dfxml_file
    dfxml_hashdigest_writer_t writer(dfxml_file);

    // export hash entries from hashdb_manager
    hashdb_iterator_t it = hashdb_manager.begin();
    progress_tracker_t progress_tracker(hashdb_manager.map_size());

    while (it != hashdb_manager.end()) {
      progress_tracker.track();
      writer.add_hashdb_element(*it);
      ++it;
    }
    progress_tracker.done();

    // export source lookup information from source_lookup_index_manager
    // and source_metadata_manager
    source_lookup_index_manager_t source_lookup_index_manager(
                                               hashdb_dir, READ_ONLY);
    source_lookup_index_manager_t::source_lookup_index_iterator_t it2 =
                                       source_lookup_index_manager.begin();
    while (it2 != source_lookup_index_manager.end()) {
      uint64_t source_lookup_index = it2->key;

      // get the repository name and filename, if available
      std::pair<bool, std::pair<std::string, std::string> > source_pair =
                     hashdb_manager.find_source_pair(source_lookup_index);

      // get source metadata, if available
      std::pair<bool, source_metadata_t> metadata_pair =
                     hashdb_manager.find_source_metadata(source_lookup_index);

      // write out the source and its metadata
      writer.add_source_metadata(source_pair, metadata_pair);
      ++it2;
    }
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {

    // open hashdb_manager1 for reading
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open hashdb_manager2 for writing
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2);

    hashdb_changes_t changes;
    hashdb_iterator_t it1 = hashdb_manager1.begin();

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
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // add_multiple A and B to C
  static void add_multiple(const std::string& hashdb_dir1,
                           const std::string& hashdb_dir2,
                           const std::string& hashdb_dir3) {

    // open hashdb_manager1 and hashdb_manager2 for reading
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb3 for writing
    hashdb_manager_t hashdb_manager3(hashdb_dir3, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2, hashdb_manager3);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_iterator_t it2 = hashdb_manager2.begin();
    hashdb_iterator_t it1_end = hashdb_manager1.end();
    hashdb_iterator_t it2_end = hashdb_manager2.end();

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
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // add A to B when repository matches
  static void add_repository(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& repository_name) {

    // open hashdb_manager1 for reading
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open hashdb_manager2 for writing
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2);

    hashdb_changes_t changes;
    hashdb_iterator_t it1 = hashdb_manager1.begin();

    logger_t logger(hashdb_dir2, "add_repository");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add_repository");
    progress_tracker_t progress_tracker(hashdb_manager1.map_size(), &logger);
    while (it1 != hashdb_manager1.end()) {
      progress_tracker.track();

      // add hashdb_element when the repository name matches
      if (it1->repository_name == repository_name) {
        hashdb_manager2.insert(*it1, changes);
      }

      ++it1;
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_repository");
    logger.add_hashdb_changes(changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& hashdb_dir3) {

    // open hashdb_manager1 and hashdb_manager2 for reading
    const hashdb_manager_t manager1(hashdb_dir1, READ_ONLY);
    const hashdb_manager_t manager2(hashdb_dir2, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(manager1.settings, hashdb_dir3);
    }
    // open hashdb3 for writing
    hashdb_manager_t manager3(hashdb_dir3, RW_MODIFY);

    // resources
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
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& hashdb_dir3) {

    // open hashdb_manager1 and hashdb_manager2 for reading
    const hashdb_manager_t manager1(hashdb_dir1, READ_ONLY);
    const hashdb_manager_t manager2(hashdb_dir2, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(manager1.settings, hashdb_dir3);
    }
    // open hashdb3 for writing
    hashdb_manager_t manager3(hashdb_dir3, RW_MODIFY);

    // resources
    require_compatibility(manager1, manager2, manager3);
    hashdb_changes_t changes;

    logger_t logger(hashdb_dir3, "intersect_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect_hash");

    // optimize processing based on smaller db
    if (manager1.map_size() <= manager2.map_size()) {
      intersect_hash_optimized(manager1, manager2, manager3, changes, &logger);
    } else {
      intersect_hash_optimized(manager2, manager1, manager3, changes, &logger);
    }

    // close logger
    logger.add_timestamp("end intersect_hash");
    logger.add_hashdb_changes(changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // subtract: hashdb1 - hashdb 2 -> hashdb3 for exact match
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& hashdb_dir3) {

    // open hashdb_manager1 and hashdb_manager2 for reading
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }
    // open hashdb3 for writing
    hashdb_manager_t hashdb_manager3(hashdb_dir3, RW_MODIFY);

    require_compatibility(hashdb_manager1, hashdb_manager2, hashdb_manager3);
    hashdb_changes_t changes;

    hashdb_iterator_t it1 = hashdb_manager1.begin();

    logger_t logger(hashdb_dir3, "subtract");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin subtract");

    progress_tracker_t progress_tracker(hashdb_manager1.map_size(), &logger);
    while (it1 != hashdb_manager1.end()) {
      
      // look for exact match in hashdb_manager2
      bool exact_match = false;
      std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
                                          hashdb_manager2.find(it1->key);
      for (; it_pair.first != it_pair.second; ++it_pair.first) {
        if (*(it_pair.first) == *it1) {
          exact_match = true;
          break;
        }
      }

      // maybe add the hashdb_element to hashdb_manager3
      if (!exact_match) {
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
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // subtract_hash: hashdb1 - hashdb 2 -> hashdb3
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& hashdb_dir3) {

    // open hashdb_manager1 and hashdb_manager2 for reading
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }
    // open hashdb3 for writing
    hashdb_manager_t hashdb_manager3(hashdb_dir3, RW_MODIFY);

    require_compatibility(hashdb_manager1, hashdb_manager2, hashdb_manager3);
    hashdb_changes_t changes;

    hashdb_iterator_t it1 = hashdb_manager1.begin();

    logger_t logger(hashdb_dir3, "subtract_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin subtract_hash");

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
    logger.add_timestamp("end subtract_hash");
    logger.add_hashdb_changes(changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

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
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open resources for hashdb2
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);
    require_compatibility(hashdb_manager1, hashdb_manager2);

    // iterate over hashes.
    hashdb_iterator_t it1 = hashdb_manager1.begin();
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
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";

  }

  // scan
  static void scan(const std::string& path_or_socket,
                   const std::string& dfxml_file) {

    // open the hashdb scan service
    hashdb_t__<hash_t> hashdb;
    std::pair<bool, std::string> open_pair = hashdb.open_scan(path_or_socket);
    if (open_pair.first == false) {
      std::cerr << open_pair.second << "\nAborting.\n";
    }

    // create space on the heap for the scan input and output vectors
    std::vector<hash_t>* scan_input = new std::vector<hash_t>;
    hashdb_t__<hash_t>::scan_output_t* scan_output = new hashdb_t__<hash_t>::scan_output_t();

    // create space for the source metadata elements even though they will not
    // be used
    std::vector<source_metadata_element_t>* scan_source_metadata_input =
                      new std::vector<source_metadata_element_t>;

    // create the hash consumer
    dfxml_scan_hash_consumer_t hash_consumer(scan_input);

    // create the source metadata consumer even though not used
    dfxml_scan_source_metadata_consumer_t source_metadata_consumer(
                                                  scan_source_metadata_input);

    // run the dfxml hashdigest reader using the scan consumers
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_hash_consumer_t,
                              dfxml_scan_source_metadata_consumer_t>::
                              do_read(dfxml_file, repository_name,
                              &hash_consumer, &source_metadata_consumer);

    // perform the scan
    hashdb.scan(*scan_input, *scan_output);

    // show the matches
    print_scan_output(*scan_input, *scan_output);

    // delete heap allocation
    delete scan_input;
    delete scan_source_metadata_input;
    delete scan_output;
  }

  // scan expanded
  static void scan_expanded(const std::string& hashdb_dir,
                            const std::string& dfxml_file) {

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // create space on the heap for the scan input vector
    std::vector<hash_t>* hashes = new std::vector<hash_t>;

    // create the hash consumer
    dfxml_scan_hash_consumer_t hash_consumer(hashes);

    // create the source metadata consumer
    dfxml_scan_expanded_smc_t source_metadata_consumer(
                                           &hashdb_manager, hashes);

    // print file header information
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# scan_expanded-command-Version: 1\n";

    // run the dfxml hashdigest reader using the scan consumers
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_hash_consumer_t,
                              dfxml_scan_expanded_smc_t>::
                              do_read(dfxml_file, repository_name,
                              &hash_consumer, &source_metadata_consumer);

    // delete heap allocation
    delete hashes;
  }

  // scan hash
  static void scan_hash(const std::string& path_or_socket,
                   const std::string& hash_string) {

    // validate the hash string
    std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(hash_string);
    if (hash_pair.first == false) {
      std::cerr << "Invalid hash value '" << hash_string << "'.  Aborting.\n";
      exit(1);
    }

    // open the hashdb scan service
    hashdb_t__<hash_t> hashdb;
    std::pair<bool, std::string> open_pair = hashdb.open_scan(path_or_socket);
    if (open_pair.first == false) {
      std::cerr << open_pair.second << "\nAborting.\n";
    }

    // create space on the heap for the scan input and output vectors
    std::vector<hash_t>* scan_input = new std::vector<hash_t>;
    hashdb_t__<hash_t>::scan_output_t* scan_output = new hashdb_t__<hash_t>::scan_output_t();

    // put the hash into the scan hash input for scanning
    scan_input->push_back(hash_pair.second);

    // perform the scan
    hashdb.scan(*scan_input, *scan_output);

    // show the matches
    print_scan_output(*scan_input, *scan_output);

    // delete heap allocation
    delete scan_input;
    delete scan_output;
  }

  // scan expanded hash
  static void scan_expanded_hash(const std::string& hashdb_dir,
                            const std::string& hash_string) {

    // validate the hash string
    std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(hash_string);
    if (hash_pair.first == false) {
      std::cerr << "Invalid hash value '" << hash_string << "'.  Aborting.\n";
      exit(1);
    }

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // find matching range for this key
    std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
    hashdb_manager.find(hash_pair.second);

    // check for no match
    if (it_pair.first == it_pair.second) {
      std::cout << "There are no matches.\n";
      return;
    }

    // go through each hashdb_element source for this hash
    for (; it_pair.first != it_pair.second; ++it_pair.first) {

      // print the hash
      std::cout << "[\"" << hash_string << "\", {";

      // get source lookup index
      std::pair<bool, uint64_t> source_pair =
               hashdb_manager.find_source_lookup_index(
                     it_pair.first->repository_name, it_pair.first->filename);
      if (source_pair.first == false) {
        // program error
        assert(0);
      }

      // print source fields
      json_helper_t::print_source_fields(hashdb_manager, source_pair.second);

      // close the JSON line
      std::cout << "}]\n";
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
    tcp_server_manager_t tcp_server_manager(hashdb_dir, port_number);
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
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // there is nothing to report if the database is empty
    if (hashdb_manager.map_size() == 0
     && hashdb_manager.source_lookup_store_size() == 0
     && hashdb_manager.repository_name_lookup_store_size() == 0
     && hashdb_manager.filename_lookup_store_size() == 0) {
      std::cout << "The hash database is empty.\n";
      return;
    }

    // print size values
    std::cout << "hash store: "
              << hashdb_manager.map_size() << "\n"
              << "source lookup store: "
              << hashdb_manager.source_lookup_store_size() << "\n"
              << "source repository name store: "
              << hashdb_manager.repository_name_lookup_store_size() << "\n"
              << "source filename store: "
              << hashdb_manager.filename_lookup_store_size() << "\n"
              << "source metadata store: "
              << hashdb_manager.source_metadata_lookup_store_size() << "\n";
/* 
    std::cout << "{\"hash_store\":"
              << hashdb_manager.map_size() << ", "
              << "\"source_lookup_store\":"
              << hashdb_manager.source_lookup_store_size() << ", "
              << "\"source_repository_name_store\":"
              << hashdb_manager.repository_name_lookup_store_size() << ", "
              << "\"source_filename_store\":"
              << hashdb_manager.filename_lookup_store_size() << ", "
              << "\"source_metadata_store\":"
              << hashdb_manager.source_metadata_lookup_store_size() << "}\n";
*/
  }

  // print sources referenced in this database
  static void sources(const std::string& hashdb_dir) {

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // get the source lookup index iterator
    source_lookup_index_manager_t::source_lookup_index_iterator_t it = hashdb_manager.begin_source_lookup_index();

    // see if the source lookup index map is empty
    if (it == hashdb_manager.end_source_lookup_index()) {
      std::cout << "There are no sources in this database.\n";
      return;
    }

    // report each entry
    for (; it != hashdb_manager.end_source_lookup_index(); ++it) {
      std::cout << "{";
      json_helper_t::print_source_fields(hashdb_manager, it->key);
      std::cout << "}\n";
    }
  }

  // show hashdb hash histogram
  static void histogram(const std::string& hashdb_dir) {
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);
    hashdb_iterator_t it = hashdb_manager.begin();

    // there is nothing to report if the map is empty
    if (it == hashdb_manager.end()) {
      std::cout << "The map is empty.\n";
      return;
    }

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_manager.map_size());

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of distinct hashes
    uint64_t total_distinct_hashes = 0;

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
        ++total_distinct_hashes;
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
    std::cout << "{\"total_hashes\": " << total_hashes << ", "
              << "\"total_distinct_hashes\": " << total_distinct_hashes << "}\n";

    // show hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>::iterator hash_histogram_it2;
    for (hash_histogram_it2 = hash_histogram->begin();
         hash_histogram_it2 != hash_histogram->end(); ++hash_histogram_it2) {
      std::cout << "{\"duplicates\":" << hash_histogram_it2->first
                << ", \"distinct_hashes\":" << hash_histogram_it2->second
                << ", \"total\":" << hash_histogram_it2->first *
                                 hash_histogram_it2->second << "}\n";
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

    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);
    hashdb_iterator_t it = hashdb_manager.begin();

    // look through all hashes for entries with this count
    bool any_found = false;
    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    while (it != hashdb_manager.end()) {
      uint32_t count = hashdb_manager.find_count(it->key);

      if (count == duplicates_number) {
        any_found = true;
        // this is the same format as that used in print_scan_output
        std::cout << "[\"" << it->key.hexdigest() << "\",{\"count\":" << count << "}]\n";
      }

      // now move forward by count
      for (uint32_t i=0; i<count; ++i) {
        ++it;
        progress_tracker.track();
      }
    }
    progress_tracker.done();

    // say so if nothing was found
    if (!any_found) {
      std::cout << "No hashes were found with this count.\n";
      return;
    }
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& repository_name,
                         const std::string& filename) {

    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // see if source exists
    std::pair<bool, uint64_t> lookup_pair =
           hashdb_manager.find_source_lookup_index(repository_name, filename);
    if (lookup_pair.first == false) {
      // the database does not have the requested source
      std::cout << "Nothing for this source was found.\n"
                << "Are the repository name and filename correct?\n";
      return;
    }

    // print source information
    std::cout << "{";
    json_helper_t::print_source_fields(hashdb_manager, lookup_pair.second);
    std::cout << "}\n";

    // show hashes for the requested source
    bool any_found = false;
    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    hashdb_iterator_t it = hashdb_manager.begin();
    while (it != hashdb_manager.end()) {
      if (it->repository_name == repository_name &&
          it->filename == filename) {
        // show the hash and its offset
        std::cout << "[\"" << it->key.hexdigest() << "\",{\"file_offset\":"
                  << it->file_offset << "}]\n";
        any_found = true;
      }
      ++it;
      progress_tracker.track();
    }
    progress_tracker.done();

    // there may be nothing to report
    if (!any_found) {
      std::cout << "There are no hashes in the database from this source.\n";
    }
  }

  // expand identified_blocks.txt
  static void expand_identified_blocks(const std::string& hashdb_dir,
                            const std::string& identified_blocks_file) {

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);

    // read identified blocks from input and write out matches
    // identified_blocks_feature consists of: offset_string, key, count
    while (!reader.at_eof()) {
      feature_line_t feature_line = reader.read();

      // get the hash from the hash string
      std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(feature_line.feature);
      if (hash_pair.first == false) {
        // bad hash value
        continue;
      }
      hash_t hash = hash_pair.second;

      // get the context without the json braces that enclose it
      size_t len = feature_line.context.size();
      std::string context;
      if (feature_line.context.find("{") == 0 &&
          feature_line.context.rfind("}") == len - 1) {
        // remove the json braces
        context = feature_line.context.substr(1, len - 2);
      } else {
        // warn and use as is
        std::cerr << "unexpected syntax in context: '"
                  << feature_line.context << "'\n";
        context = feature_line.context;
      }

      // find matching range for this key
      std::pair<hashdb_iterator_t, hashdb_iterator_t> it_pair =
      hashdb_manager.find(hash);

      // go through each hashdb_element source for this hash
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get source lookup index
        std::pair<bool, uint64_t> source_pair =
               hashdb_manager.find_source_lookup_index(
                     it_pair.first->repository_name, it_pair.first->filename);
        if (source_pair.first == false) {
          // program error
          assert(0);
        }

        // write match to output:
        // offset tab hashdigest tab { context, source information }
        std::cout << feature_line.forensic_path << "\t"
                  << feature_line.feature << "\t"
                  << "{" << context << ",";
        json_helper_t::print_source_fields(hashdb_manager, source_pair.second);
        std::cout << "}\n";
      }
    }
  }

  // explain identified_blocks.txt
  static void explain_identified_blocks(
                            const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // create a hash set for tracking hashes that will be used
    hashes_t* hashes = new hashes_t;

    // create a source lookup index set for tracking source lookup indexes
    std::set<uint64_t>* source_lookup_indexes = new std::set<uint64_t>;

    // ingest table of relevant hashes and table of relevant sources
    identify_hashes_and_sources(hashdb_manager, identified_blocks_file,
                                requested_max, 
                                *hashes, *source_lookup_indexes);

    // print file header information
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# explain_identified_blocks-command-Version: 1\n";

    // print identified hashes
    std::cout << "# hashes\n";
    print_identified_hashes(hashdb_manager, *hashes);

    // print identified sources
    std::cout << "# sources\n";
    print_identified_sources(hashdb_manager, *source_lookup_indexes);

    // clean up
    delete hashes;
    delete source_lookup_indexes;
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
    bloom_filter_manager_t bloom_filter_manager(hashdb_dir, RW_NEW,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions);

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // add hashes to the bloom filter
    logger.add_timestamp("begin rebuild_bloom");
    hashdb_iterator_t it = hashdb_manager.begin();
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
    history_manager_t::append_log_to_history(hashdb_dir);

    std::cout << "rebuild_bloom complete.\n";
  }

  // upgrade hashdb
  static void upgrade(const std::string& hashdb_dir) {

    // start logger
    logger_t logger(hashdb_dir, "upgrade");
    logger.add("hashdb_dir", hashdb_dir);

    // open resources
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);

    // close logger
    logger.add_timestamp("end upgrade");
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);

    std::cout << "Upgrade complete.\n";
  }

  // functional analysis and testing: add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& count_string,
                         const std::string& repository_name) {

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
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);
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

      // generate file offset or 0 if hash_block_size is 0
      uint64_t file_offset = (i%(1<<26)) * hash_block_size;

      // add element
      hashdb_manager.insert(hashdb_element_t(random_key(),
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
    history_manager_t::append_log_to_history(hashdb_dir);

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
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // open the hashdb scan service for scanning with random hash
    hashdb_t__<hash_t> hashdb;
    std::pair<bool, std::string> open_pair = hashdb.open_scan(hashdb_dir);
    if (open_pair.first == false) {
      std::cerr << open_pair.second << "\nAborting.\n";
    }

    // open the hashdb scan service for scanning from the copy
    hashdb_t__<hash_t> hashdb_copy;
    std::pair<bool, std::string> open_pair_copy = hashdb.open_scan(hashdb_dir_copy);
    if (open_pair_copy.first == false) {
      std::cerr << open_pair_copy.second << "\nAborting.\n";
    }

    // create space on the heap for the scan input and output vectors
    std::vector<hash_t>* scan_input = new std::vector<hash_t>;
    hashdb_t__<hash_t>::scan_output_t* scan_output =
                             new hashdb_t__<hash_t>::scan_output_t();

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

    // scan copy for sets of random hashes where all hash values match
    logger.add_timestamp("begin scan_random with random matching hashes on hashdb copy");
    for (int j=1; j<=100; j++) {
      // generate set of random hashes that match
      generate_scan_input(&hashdb_manager, scan_input);
      std::stringstream ss1;
      ss1 << "generated random matching hash " << j;
      logger.add_timestamp(ss1.str());

      // scan set of random hashes that match
      hashdb_copy.scan(*scan_input, *scan_output);
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
    history_manager_t::append_log_to_history(hashdb_dir);
  }
};

#endif

