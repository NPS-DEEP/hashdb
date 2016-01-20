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
 * Provides hashdb commands.
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include "../src_libhashdb/hashdb.hpp"
#include "tab_hashdigest_reader.hpp"
#include "add.hpp"
#include "hex_helper.hpp"
//#include "expand_manager.hpp"
//#include "dfxml_scan_consumer.hpp"
//#include "dfxml_scan_expanded_consumer.hpp"
//#include "dfxml_hashdigest_writer.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

// return true if exists or was successfully created with same settings,
// false if not
static std::pair<bool, std::string> create_if_new(
                                  const std::string& hashdb_dir,
                                  const std::string& from_hashdb_dir,
                                  const std::string& command_string) {

  uint32_t sector_size;
  uint32_t block_size;
  uint32_t max_id_offset_pairs;
  uint32_t hash_prefix_bits;
  uint32_t hash_suffix_bytes;
  std::pair<bool, std::string> pair;

  // try to read hashdb_dir settings
  pair = hashdb::hashdb_settings(hashdb_dir, sector_size, block_size,
       max_id_offset_pairs, hash_prefix_bits, hash_suffix_bytes);
  if (pair.first == true) {
    // hashdb_dir already exists
    return pair;
  }

  // no hashdb_dir, so read from_hashdb_dir settings
  pair = hashdb::hashdb_settings(from_hashdb_dir, sector_size, block_size,
       max_id_offset_pairs, hash_prefix_bits, hash_suffix_bytes);
  if (pair.first == false) {
    // bad since from_hashdb_dir is not valid
    return pair;
  }

  // create hashdb_dir using from_hashdb_dir settings
  pair = hashdb::create_hashdb(hashdb_dir,
                               sector_size,
                               block_size,
                               max_id_offset_pairs,
                               hash_prefix_bits,
                               hash_suffix_bytes,
                               command_string);

  return pair;
}

static void print_header(const std::string& command_id,
                         const std::string& cmd) {
  std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
            << "# " << command_id << "\n"
            << "# command_line: " << cmd << "\n";
}

namespace commands {

  // new database
  void create(const std::string& hashdb_dir,
              const uint32_t sector_size,
              const uint32_t block_size,
              const uint32_t max_id_offset_pairs,
              const uint32_t hash_prefix_bits,
              const uint32_t hash_suffix_bytes,
              const std::string& cmd) {

    std::pair<bool, std::string> pair = hashdb::create_hashdb(
           hashdb_dir, sector_size, block_size, max_id_offset_pairs,
           hash_prefix_bits, hash_suffix_bytes, cmd);

    if (pair.first == true) {
      std::cout << "New database created.\n";
    } else {
      std::cout << "Error: " << pair.second << "\n";
    }
  }

  // ************************************************************
  // import
  // ************************************************************
  // import
  static void import(const std::string& hashdb_dir,
                     const std::string& import_dir,
                     const std::string& repository_name,
                     const std::string& whitelist_dir,
                     const std::string& cmd) {
    // import_dir.hpp
    std::cout << "TBD\n";
  }

  // import_tab
  static void import_tab(const std::string& hashdb_dir,
                     const std::string& tab_file,
                     const std::string& repository_name,
                     const std::string& cmd) {

    tab_hashdigest_reader_t reader(hashdb_dir, tab_file, repository_name, cmd);

    std::pair<bool, std::string> pair = reader.read();
    if (pair.first == true) {
      std::cout << "Import completed.\n";
    } else {
      std::cout << "Error: " << pair.second << "\n";
    }
  }

  // ************************************************************
  // database manipulation
  // ************************************************************
  // add
  static void add(const std::string& hashdb_dir,
                  const std::string& dest_dir,
                  const std::string& cmd) {
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {
      // add data for binary_hash from A to B
      add::add(pair.second, manager_a, manager_b);
      pair = manager_a.hash_next(pair.second);
    }
  }

  // add_multiple
  static void add_multiple(const std::vector<std::string>& hashdb_dirs,
                           const std::string& cmd) {

    typedef std::multimap<std::string, hashdb::scan_manager_t*>
                                                     ordered_producers_t;
    typedef std::pair<std::string, hashdb::scan_manager_t*>
                                                     hash_producer_pair_t;

    std::string binary_hash;
    std::pair<bool, std::string> hash_pair;
    std::pair<std::string, hashdb::scan_manager_t*> hash_producer_pair;

    // open the consumer from hashdb_dirs[0]
    std::vector<std::string>::const_iterator hashdb_dir_it =
                                                          hashdb_dirs.begin();
    hashdb::import_manager_t consumer(*hashdb_dir_it, cmd);

    // the multimap for processing hashes in order from the producers
    ordered_producers_t ordered_producers;

    // a producer
    hashdb::scan_manager_t* producer;

    // open the producers from hashdb_dirs[1+]
    while (++hashdb_dir_it != hashdb_dirs.end()) {

      // create a producer
      producer = new hashdb::scan_manager_t(*hashdb_dir_it);

      // read first hash
      hash_pair = producer->hash_begin();
      if (hash_pair.first == true) {
        // hash exists so add the hash, producer pair
        ordered_producers.insert(hash_producer_pair_t(
                                          hash_pair.second, producer));
      } else {
        // no hashes for this producer so close it
        delete producer;
      }
    }

    // add ordered hashes from producers until all hashes are consumed
    while (ordered_producers.size() != 0) {
      // get the hash, producer pair for the first hash
      ordered_producers_t::iterator it = ordered_producers.begin();
      binary_hash = it->first;
      producer = it->second;

      // add the hash to the consumer
      add::add(binary_hash, *producer, consumer);

      // remove this hash
      ordered_producers.erase(it);

      // get the next hash
      hash_pair = producer->hash_next(binary_hash);

      if (hash_pair.first) {
        // hash exists so add the hash, producer pair
        ordered_producers.insert(hash_producer_pair_t(
                                          hash_pair.second, producer));
      } else {
        // no hashes for this producer so close it
        delete producer;
      }
    }
  }

  // add_repository
  static void add_repository(const std::string& hashdb_dir,
                             const std::string& dest_dir,
                             const std::string& repository_name,
                             const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& dest_dir,
                        const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& dest_dir,
                             const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // subtract
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& dest_dir,
                       const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // subtract_hash
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& dest_dir,
                            const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // subtract_repository
  static void subtract_repository(const std::string& hashdb_dir,
                                  const std::string& dest_dir,
                                  const std::string& repository_name,
                                  const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir,
                          const std::string& dest_dir,
                          const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // ************************************************************
  // scan
  // ************************************************************
  // scan
  static void scan(const std::string& hashdb_dir,
                   const std::string& dfxml_file,
                   const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // scan_hash
  static void scan_hash(const std::string& hashdb_dir,
                        const std::string& hex_block_hash,
                        const std::string& cmd) {

    // get the binary hash
    std::string binary_hash = hex_to_bin(hex_block_hash);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << hex_block_hash << "'\n";
      exit(1);
    }

    // open DB
    hashdb::scan_manager_t scan_manager(hashdb_dir);

    // scan
    std::string* expanded_text = new std::string;
    bool found = scan_manager.find_expanded_hash(binary_hash, *expanded_text);

    if (found == true) {
      std::cout << *expanded_text << std::endl;
    } else {
      std::cout << "Hash not found for '" << hex_block_hash << "'\n";
    }
    delete expanded_text;
  }

  // ************************************************************
  // statistics
  // ************************************************************
  // size
  static void size(const std::string& hashdb_dir,
                   const std::string& cmd) {

    // open DB
    hashdb::scan_manager_t scan_manager(hashdb_dir);

    std::cout << scan_manager.size() << std::endl;
  }

  // sources
  static void sources(const std::string& hashdb_dir,
                      const std::string& cmd) {

    // open DB
    hashdb::scan_manager_t scan_manager(hashdb_dir);
    std::pair<bool, uint64_t> pair = scan_manager.source_begin();

    // note if the DB is empty
    if (pair.first == false) {
      std::cout << "There are no sources in this database.\n";
      return;
    }

    // print the sources
    std::string* expanded_text = new std::string;
    while (pair.first == true) {
      scan_manager.find_expanded_source(pair.second, *expanded_text);
      std::cout << *expanded_text << std::endl;
      pair = scan_manager.source_next(pair.second);
    }
    delete expanded_text;
  }

  // histogram
  static void histogram(const std::string& hashdb_dir,
                        const std::string& cmd) {

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print header information
    print_header("histogram-command-Version: 2", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size());

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of distinct hashes
    uint64_t total_distinct_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>* hash_histogram =
                new std::map<uint32_t, uint64_t>();
    
    // space for variables
    std::string low_entropy_label;
    uint64_t entropy;
    std::string block_label;
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;

    // iterate over hashdb and set variables for calculating the histogram
    std::pair<bool, std::string> pair = manager.hash_begin();

    // note if the DB is empty
    if (pair.first == false) {
      std::cout << "The map is empty.\n";
    }

    while (pair.first == true) {
      manager.find_hash(pair.second, low_entropy_label, entropy, block_label,
                        *id_offset_pairs);
      uint64_t count = id_offset_pairs->size();
      // update total hashes observed
      total_hashes += count;
      // update total distinct hashes
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

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track();
    }

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
    delete id_offset_pairs;
  }

  // duplicates
  static void duplicates(const std::string& hashdb_dir,
                         const std::string& number_string,
                         const std::string& cmd) {

    // convert duplicates string to number
    uint32_t number = atoi(number_string.c_str());

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_header("duplicates-command-Version: 2", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size());

    bool any_found = false;

    // space for variables
    std::string low_entropy_label;
    uint64_t entropy;
    std::string block_label;
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;
    std::string* expanded_text = new std::string;

    // iterate over hashdb and set variables for finding duplicates
    std::pair<bool, std::string> pair = manager.hash_begin();

    while (pair.first == true) {
      manager.find_hash(pair.second, low_entropy_label, entropy, block_label,
                        *id_offset_pairs);
      if (id_offset_pairs->size() == number) {
        manager.find_expanded_hash(pair.second, *expanded_text);
        std::cout << bin_to_hex(pair.second) << "\t" << *expanded_text << "\n";
      }

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track();
    }

    // say so if nothing was found
    if (!any_found) {
      std::cout << "No hashes were found with this count.\n";
      return;
    }

    delete id_offset_pairs;
    delete expanded_text;
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& hex_file_hash,
                         const std::string& cmd) {

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // look for source ID
    std::pair<bool, uint64_t> id_pair =
                          manager.find_source_id(hex_to_bin(hex_file_hash));

    if (id_pair.first == false) {
      std::cout << "There is no source with this file hash\n";
      return;
    }
    uint64_t source_id = id_pair.second;

    // print header information
    print_header("hash-table-command-Version: 3", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size());

    // space for variables
    std::string low_entropy_label;
    uint64_t entropy;
    std::string block_label;
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;
    std::string* expanded_text = new std::string;

    // iterate over hashdb and set variables for finding hashes with
    // this source ID
    std::pair<bool, std::string> pair = manager.hash_begin();

    while (pair.first == true) {
      manager.find_hash(pair.second, low_entropy_label, entropy, block_label,
                        *id_offset_pairs);
      for (hashdb::id_offset_pairs_t::const_iterator it =
            id_offset_pairs->begin(); it!= id_offset_pairs->end(); ++it) {
        if (it->first == source_id) {
          manager.find_expanded_hash(pair.second, *expanded_text);
          std::cout << bin_to_hex(pair.second) << "\t" << *expanded_text
                    << "\n";
        }
      }

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track();
    }
    delete id_offset_pairs;
    delete expanded_text;
  }

  // ************************************************************
  // performance analysis
  // ************************************************************
  // add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& hex_file_hash,
                         const std::string& count_string,
                         const std::string& cmd) {
    std::cout << "TBD\n";
  }

  // scan_random
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& count_string,
                          const std::string& cmd) {
    std::cout << "TBD\n";
  }
}






/*
  static void import_hashdb(const std::string& hashdb_dir,
                            const std::string& dfxml_file,
                            const std::string& repository_name) {

    // require that dfxml_file exists
    file_helper::require_dfxml_file(dfxml_file);

    // open hashdb RW manager
    lmdb_rw_manager_t rw_manager(hashdb_dir);

    logger_t logger(hashdb_dir, "import");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("dfxml_file", dfxml_file);
    logger.add("repository_name", repository_name);
    logger.add_timestamp("begin import");

    // start progress tracker
    progress_tracker_t progress_tracker(0, &logger);

    // create the DFXML import consumer
    dfxml_import_consumer_t import_consumer(&rw_manager, &progress_tracker);

    // run the dfxml hashdigest reader using the import hash consumer
    std::pair<bool, std::string> do_read_pair =
    dfxml_hashdigest_reader_t<dfxml_import_consumer_t>::
             do_read(dfxml_file,
                     repository_name,
                     rw_manager.settings.hash_block_size,
                     &import_consumer);

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end import");
    logger.add_hashdb_changes(rw_manager.changes);
    logger.close();

    if (do_read_pair.first == true) {
      // good, reader worked, also write changes to cout
      std::cout << rw_manager.changes << "\n";
    } else {
      // bad, reader failed
      std::cerr << do_read_pair.second << ".  Import aborted.\n";
      exit(1);
    }
  }

  // export
  static void do_export(const std::string& hashdb_dir,
                        const std::string& dfxml_file) {

    // lets require that dfxml_file does not exist yet
    file_helper::require_no_file(dfxml_file);

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // open the dfxml writer
    dfxml_hashdigest_writer_t writer(dfxml_file,
                                     ro_manager.settings.hash_block_size);

    // start the progress tracker
    progress_tracker_t progress_tracker(ro_manager.size());

    // export the hash entries
    lmdb_hash_it_data_t hash_it_data = ro_manager.find_begin();
    while (hash_it_data.is_valid) {
      progress_tracker.track();
      lmdb_source_data_t source_data = ro_manager.find_source(
                                          hash_it_data.source_lookup_index);
      writer.add_hashdb_element(hash_it_data.binary_hash,
                                source_data,
                                hash_it_data.file_offset,
                                hash_it_data.hash_label);
      hash_it_data = ro_manager.find_next(hash_it_data);
    }
    progress_tracker.done();
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {
    require_compatible(hashdb_dir1, hashdb_dir2);

    // open ro_manager1 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);

    // if hashdb2 does not exist, create it with settings from hashdb1
    create_if_new(hashdb_dir2, ro_manager1.settings);

    // open rw_manager2 for writing
    lmdb_rw_manager_t rw_manager2(hashdb_dir2);

    logger_t logger(hashdb_dir2, "add");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start copying
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      progress_tracker.track();
      copy_hash(hash_it_data1, ro_manager1, rw_manager2);
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
    }
    progress_tracker.done();

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add");
    logger.add_hashdb_changes(rw_manager2.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << rw_manager2.changes << "\n";
  }

  // add_multiple A and B to C
  static void add_multiple(const std::string& hashdb_dir1,
                           const std::string& hashdb_dir2,
                           const std::string& hashdb_dir3) {

    require_compatible(hashdb_dir1, hashdb_dir2, hashdb_dir3);

    // open 1 and 2 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);
    lmdb_ro_manager_t ro_manager2(hashdb_dir2);

    // if 3 does not exist, create it with settings from 1
    create_if_new(hashdb_dir3, ro_manager1.settings);

    // open 3 for writing
    lmdb_rw_manager_t rw_manager3(hashdb_dir3);

    logger_t logger(hashdb_dir3, "add_multiple");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin add_multiple");
    size_t total_count = ro_manager1.size() + ro_manager2.size();
    progress_tracker_t progress_tracker(total_count, &logger);

    // get iterators
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    lmdb_hash_it_data_t hash_it_data2 = ro_manager2.find_begin();

    while (hash_it_data1.is_valid && hash_it_data2.is_valid) {
      if (hash_it_data1.binary_hash <= hash_it_data2.binary_hash) {
        copy_hash(hash_it_data1, ro_manager1, rw_manager3);
        hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      } else {
        copy_hash(hash_it_data2, ro_manager2, rw_manager3);
        hash_it_data2 = ro_manager2.find_next(hash_it_data2);
      }
      progress_tracker.track();
    }

    // 1 or 2 has become depleted so insert remaining elements
    while (hash_it_data1.is_valid) {
      copy_hash(hash_it_data1, ro_manager1, rw_manager3);
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      progress_tracker.track();
    }

    while (hash_it_data2.is_valid) {
      copy_hash(hash_it_data2, ro_manager2, rw_manager3);
      hash_it_data2 = ro_manager2.find_next(hash_it_data2);
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_multiple");
    logger.add_hashdb_changes(rw_manager3.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << rw_manager3.changes << "\n";
  }

  // add A to B when repository matches
  static void add_repository(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& repository_name) {

    require_compatible(hashdb_dir1, hashdb_dir2);

    // open ro_manager1 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);

    // if hashdb2 does not exist, create it with settings from hashdb1
    create_if_new(hashdb_dir2, ro_manager1.settings);

    // open rw_manager2 for writing
    lmdb_rw_manager_t rw_manager2(hashdb_dir2);

    logger_t logger(hashdb_dir2, "add_repository");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add_repository");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start copying
    lmdb_hash_it_data_t hash_it_data = ro_manager1.find_begin();
    while (hash_it_data.is_valid) {
      progress_tracker.track();
      lmdb_source_data_t source_data = ro_manager1.find_source(
                                          hash_it_data.source_lookup_index);
      if (source_data.repository_name == repository_name) {
        rw_manager2.insert(hash_it_data.binary_hash,
                           hash_it_data.file_offset,
                           ro_manager1.settings.hash_block_size,
                           source_data,
                           hash_it_data.hash_label);
      }
      hash_it_data = ro_manager1.find_next(hash_it_data);
    }
    progress_tracker.done();

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_repository");
    logger.add_hashdb_changes(rw_manager2.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << rw_manager2.changes << "\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& hashdb_dir3) {

    require_compatible(hashdb_dir1, hashdb_dir2, hashdb_dir3);

    // open 1 and 2 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);
    lmdb_ro_manager_t ro_manager2(hashdb_dir2);

    // if 3 does not exist, create it with settings from 1
    create_if_new(hashdb_dir3, ro_manager1.settings);

    // open 3 for writing
    lmdb_rw_manager_t rw_manager3(hashdb_dir3);

    logger_t logger(hashdb_dir3, "intersect");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start intersect
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      lmdb_source_data_t source_data1 = ro_manager1.find_source(
                                          hash_it_data1.source_lookup_index);
      if (ro_manager2.find_exact(hash_it_data1.binary_hash,
                                 source_data1,
                                 hash_it_data1.file_offset,
                                 hash_it_data1.hash_label)) {

        rw_manager3.insert(hash_it_data1.binary_hash,
                           hash_it_data1.file_offset,
                           ro_manager1.settings.hash_block_size,
                           source_data1,
                           hash_it_data1.hash_label);
      }
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end intersect");
    logger.add_hashdb_changes(rw_manager3.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << rw_manager3.changes << "\n";
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& hashdb_dir3) {

    require_compatible(hashdb_dir1, hashdb_dir2, hashdb_dir3);

    // open 1 and 2 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);
    lmdb_ro_manager_t ro_manager2(hashdb_dir2);

    // if 3 does not exist, create it with settings from 1
    create_if_new(hashdb_dir3, ro_manager1.settings);

    // open 3 for writing
    lmdb_rw_manager_t rw_manager3(hashdb_dir3);

    logger_t logger(hashdb_dir3, "intersect_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect_hash");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start intersect
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      lmdb_hash_it_data_t hash_it_data2 =
                         ro_manager2.find_first(hash_it_data1.binary_hash);

      if (hash_it_data1.binary_hash == hash_it_data2.binary_hash) {
        std::string binary_hash = hash_it_data1.binary_hash;

        // copy 1 to 3
        while (hash_it_data1.binary_hash == binary_hash) {
          copy_hash(hash_it_data1, ro_manager1, rw_manager3);
          hash_it_data1 = ro_manager1.find_next(hash_it_data1);
          progress_tracker.track();
        }

        // copy 2 to 3
        while (hash_it_data2.binary_hash == binary_hash) {
          copy_hash(hash_it_data2, ro_manager2, rw_manager3);
          hash_it_data2 = ro_manager2.find_next(hash_it_data2);
        }
      } else {
        // 1 not incremented by match so increment here
        hash_it_data1 = ro_manager1.find_next(hash_it_data1);
        progress_tracker.track();
      }
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end intersect_hash");
    logger.add_hashdb_changes(rw_manager3.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << rw_manager3.changes << "\n";
  }

  // subtract: hashdb1 - hashdb 2 -> hashdb3 for exact match
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& hashdb_dir3) {

    require_compatible(hashdb_dir1, hashdb_dir2, hashdb_dir3);

    // open 1 and 2 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);
    lmdb_ro_manager_t ro_manager2(hashdb_dir2);

    // if 3 does not exist, create it with settings from 1
    create_if_new(hashdb_dir3, ro_manager1.settings);

    // open 3 for writing
    lmdb_rw_manager_t rw_manager3(hashdb_dir3);

    logger_t logger(hashdb_dir3, "subtract");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin subtract");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start subtract
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      lmdb_source_data_t source_data1 = ro_manager1.find_source(
                                          hash_it_data1.source_lookup_index);
      if (!ro_manager2.find_exact(hash_it_data1.binary_hash,
                                  source_data1,
                                  hash_it_data1.file_offset,
                                  hash_it_data1.hash_label)) {

        rw_manager3.insert(hash_it_data1.binary_hash,
                           hash_it_data1.file_offset,
                           ro_manager1.settings.hash_block_size,
                           source_data1,
                           hash_it_data1.hash_label);
      }
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end subtract");
    logger.add_hashdb_changes(rw_manager3.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << rw_manager3.changes << "\n";
  }

  // subtract_hash: hashdb1 - hashdb 2 -> hashdb3
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& hashdb_dir3) {

    require_compatible(hashdb_dir1, hashdb_dir2, hashdb_dir3);

    // open 1 and 2 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);
    lmdb_ro_manager_t ro_manager2(hashdb_dir2);

    // if 3 does not exist, create it with settings from 1
    create_if_new(hashdb_dir3, ro_manager1.settings);

    // open 3 for writing
    lmdb_rw_manager_t rw_manager3(hashdb_dir3);

    logger_t logger(hashdb_dir3, "subtract_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin subtract_hash");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start subtract
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      size_t count = ro_manager2.find_count(hash_it_data1.binary_hash);
      if (count == 0) {
        // 1 not in 2 so add 1 to 3
        copy_hash(hash_it_data1, ro_manager1, rw_manager3);
      }
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end subtract_hash");
    logger.add_hashdb_changes(rw_manager3.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << rw_manager3.changes << "\n";
  }

  // add A to B when repository does not match
  static void subtract_repository(const std::string& hashdb_dir1,
                                  const std::string& hashdb_dir2,
                                  const std::string& repository_name) {

    require_compatible(hashdb_dir1, hashdb_dir2);

    // open ro_manager1 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);

    // if hashdb2 does not exist, create it with settings from hashdb1
    create_if_new(hashdb_dir2, ro_manager1.settings);

    // open rw_manager2 for writing
    lmdb_rw_manager_t rw_manager2(hashdb_dir2);

    logger_t logger(hashdb_dir2, "subtract_repository");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin subtract_repository");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start copying
    lmdb_hash_it_data_t hash_it_data = ro_manager1.find_begin();
    while (hash_it_data.is_valid) {
      progress_tracker.track();
      lmdb_source_data_t source_data = ro_manager1.find_source(
                                          hash_it_data.source_lookup_index);
      if (!(source_data.repository_name == repository_name)) {
        rw_manager2.insert(hash_it_data.binary_hash,
                           hash_it_data.file_offset,
                           ro_manager1.settings.hash_block_size,
                           source_data,
                           hash_it_data.hash_label);
      }
      hash_it_data = ro_manager1.find_next(hash_it_data);
    }
    progress_tracker.done();

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end subtract_repository");
    logger.add_hashdb_changes(rw_manager2.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << rw_manager2.changes << "\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir1,
                          const std::string& hashdb_dir2) {
    require_compatible(hashdb_dir1, hashdb_dir2);

    // open ro_manager1 for reading
    lmdb_ro_manager_t ro_manager1(hashdb_dir1);

    // if hashdb2 does not exist, create it with settings from hashdb1
    create_if_new(hashdb_dir2, ro_manager1.settings);

    // open rw_manager2 for writing
    lmdb_rw_manager_t rw_manager2(hashdb_dir2);

    logger_t logger(hashdb_dir2, "deduplicate");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin deduplicate");
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

    // start copying
    lmdb_hash_it_data_t hash_it_data1 = ro_manager1.find_begin();
    while (hash_it_data1.is_valid) {
      size_t count = ro_manager1.find_count(hash_it_data1.binary_hash);
      if (count == 1) {
        copy_hash(hash_it_data1, ro_manager1, rw_manager2);
      }
      hash_it_data1 = ro_manager1.find_next(hash_it_data1);
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end deduplicate");
    logger.add_hashdb_changes(rw_manager2.changes);
    logger.close();

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << rw_manager2.changes << "\n";
  }

  // scan
  static void scan(const std::string& hashdb_dir,
                   const std::string& dfxml_file) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // create the dfxml scan consumer
    dfxml_scan_consumer_t scan_consumer(&ro_manager);

    // print header information
    print_helper::print_header("scan-command-Version: 2");

    // run the dfxml hashdigest reader using the scan consumer
    std::string repository_name = "not used";
    std::pair<bool, std::string> do_read_pair =
    dfxml_hashdigest_reader_t<dfxml_scan_consumer_t>::
                        do_read(dfxml_file,
                                repository_name,
                                ro_manager.settings.hash_block_size,
                                &scan_consumer);
    if (do_read_pair.first == false) {
      // bad, reader failed
      std::cerr << do_read_pair.second << ".  Scan aborted.\n";
      exit(1);
    }
  }

  // scan expanded
  static void scan_expanded(const std::string& hashdb_dir,
                            const std::string& dfxml_file,
                            uint32_t max_sources) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // create the dfxml scan consumer
    dfxml_scan_expanded_consumer_t scan_expanded_consumer(&ro_manager, max_sources);

    // print header information
    print_helper::print_header("scan_expanded-command-Version: 3");

    // run the dfxml hashdigest reader using the scan consumer
    std::string repository_name = "not used";
    std::pair<bool, std::string> do_read_pair =
    dfxml_hashdigest_reader_t<dfxml_scan_expanded_consumer_t>::
                        do_read(dfxml_file,
                                repository_name,
                                ro_manager.settings.hash_block_size,
                                &scan_expanded_consumer);
    if (do_read_pair.first == false) {
      // bad, reader failed
      std::cerr << do_read_pair.second << ".  Scan aborted.\n";
      exit(1);
    }
  }

  // scan hash
  static void scan_hash(const std::string& hashdb_dir,
                   const std::string& hash_string) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // get the binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(hash_string);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << hash_string << "'\n";
      exit(1);
    }

    // scan
    size_t count = ro_manager.find_count(binary_hash);

    // print the hash
    std::cout << "[\"" << lmdb_helper::binary_hash_to_hex(binary_hash)
              << "\",{\"count\":" << count << "}]" << std::endl;
  }

  // scan expanded hash
  static void scan_expanded_hash(const std::string& hashdb_dir,
                            const std::string& hash_string,
                            uint32_t max_sources) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // get the binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(hash_string);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << hash_string << "'\n";
      exit(1);
    }

    // check for no match
    if (ro_manager.find_count(binary_hash) == 0) {
      std::cout << "There are no matches.\n";
      return;
    }

    // print header information
    print_helper::print_header("scan_expanded_hash-command-Version: 3");

    // open the expand manager
    expand_manager_t expand_manager(&ro_manager, max_sources);
    expand_manager.expand_hash(binary_hash);
  }

  // show hashdb size values
  static void size(const std::string& hashdb_dir) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    std::cout << "hash store size: " << ro_manager.size() << "\n"
              << "source store size: " << ro_manager.source_store_size()
              << "\n";
  }

  // print sources referenced in this database
  static void sources(const std::string& hashdb_dir) {
    // open source store DB
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // read source entries
    lmdb_source_it_data_t source_it_data = source_store.find_first();
    if (source_it_data.is_valid == false) {
      std::cout << "There are no sources in this database.\n";
      return;
    }
    while (source_it_data.is_valid) {
      print_helper::print_source_fields(source_it_data);
      source_it_data = source_store.find_next(source_it_data.source_lookup_index);
    }
  }

  // show hashdb hash histogram
  static void histogram(const std::string& hashdb_dir) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (ro_manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_helper::print_header("histogram-command-Version: 2");

    // start progress tracker
    progress_tracker_t progress_tracker(ro_manager.size());

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of distinct hashes
    uint64_t total_distinct_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>* hash_histogram =
                new std::map<uint32_t, uint64_t>();
    
    // iterate over hashdb and set variables for calculating the histogram
    lmdb_hash_it_data_t hash_it_data = ro_manager.find_begin();
    while (hash_it_data.is_valid) {

      // get count for hash
      const size_t count = ro_manager.find_count(hash_it_data.binary_hash);
      if (count == 0) {
        // bad state
        assert(0);
      }

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
        hash_it_data = ro_manager.find_next(hash_it_data);

        // update progress tracker
        progress_tracker.track();
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
    uint32_t duplicates_number = atoi(duplicates_string.c_str());

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (ro_manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_helper::print_header("duplicates-command-Version: 2");

    // start progress tracker
    progress_tracker_t progress_tracker(ro_manager.size());

    // look through all hashes for entries with this count
    bool any_found = false;
    lmdb_hash_it_data_t hash_it_data = ro_manager.find_begin();
    while (hash_it_data.is_valid) {

      // get count for hash
      const size_t count = ro_manager.find_count(hash_it_data.binary_hash);

      if (count == duplicates_number) {
        any_found = true;

        // print the hash
        print_helper::print_hash(hash_it_data.binary_hash, count);
      }

      // now move forward by count
      for (uint32_t i=0; i<count; i++) {
        hash_it_data = ro_manager.find_next(hash_it_data);

        // update progress tracker
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
                         const std::string& source_id_string) {

    // convert source ID to number
    uint64_t source_id = atol(source_id_string.c_str());

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (ro_manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // also the source ID must exist
    if (!ro_manager.has_source(source_id)) {
      std::cout << "The requested source ID is not in the database.\n";
      return;
    }

    // print header information
    print_helper::print_header("hash-table-command-Version: 3");

    // for completeness, print source information for this source
    // print as a comment
    std::cout << "# ";
    lmdb_source_data_t source_data = ro_manager.find_source(source_id);
    print_helper::print_source_fields(lmdb_source_it_data_t(source_id, source_data, true));

    // start progress tracker
    progress_tracker_t progress_tracker(ro_manager.size());

    // create the hash table lines in a set for ordered output when done
    std::set<std::string>* lines = new std::set<std::string>;

    // look through all hashes for entries with this source ID
    lmdb_hash_it_data_t hash_it_data = ro_manager.find_begin();
    while (hash_it_data.is_valid) {

      if (hash_it_data.source_lookup_index == source_id) {
        // prepare the line formatted as an identified_blocks.txt feature:
        // file offset <tab> hash <tab> count
        size_t count = ro_manager.find_count(hash_it_data.binary_hash);
        std::stringstream ss;
        ss << hash_it_data.file_offset << "\t"
           << lmdb_helper::binary_hash_to_hex(hash_it_data.binary_hash) << "\t"
           << "{\"count\":" << count;
        ss << "}";
        lines->insert(ss.str());
      }

      // update progress tracker
      progress_tracker.track();

      // next
      hash_it_data = ro_manager.find_next(hash_it_data);
    }
    progress_tracker.done();

    // say so if nothing was found
    if (lines->size() == 0) {
      std::cout << "No hashes were found with this source ID.\n";
      return;
    }

    for (std::set<std::string>::const_iterator it = lines->begin();
         it != lines->end(); ++it) {
      std::cout << *it << "\n";
    }
    delete lines;
  }

  // expand identified_blocks.txt
  static void expand_identified_blocks(const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (ro_manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_helper::print_header("expand_identified_blocks-command-Version: 3");

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);

    // get the source expand manager
    expand_manager_t expand_manager(&ro_manager, requested_max);

    // read identified blocks from input and write out matches
    // identified_blocks feature consists of offset_string, key, count and flags
    while (!reader.at_eof()) {
      feature_line_t feature_line = reader.read();
      expand_manager.expand_feature_line(feature_line);
    }
  }

  // explain identified_blocks.txt
  static void explain_identified_blocks(
                            const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (ro_manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_helper::print_header("explain_identified_blocks-command-Version: 3");

    // open the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);

    // open the source explain manager
    explain_manager_t explain_manager(&ro_manager, requested_max);

    // ingest identified blocks from input
    // identified_blocks feature consists of offset_string, key, count and flags
    while (!reader.at_eof()) {
      feature_line_t feature_line = reader.read();
      explain_manager.ingest_hash(feature_line);
    }

    // print identified hashes
    std::cout << "# hashes\n";
    explain_manager.print_identified_hashes();

    // print identified sources
    std::cout << "# sources\n";
    explain_manager.print_identified_sources();
  }

  // functional analysis and testing: add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& count_string,
                         const std::string& repository_name) {

    // initialize random seed
    srand (time(NULL));

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // open resources
    // open DB
    lmdb_rw_manager_t rw_manager(hashdb_dir);

    // start logger
    logger_t logger(hashdb_dir, "add_random");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add("count", count);
    logger.add_timestamp("begin add_random");

    // start progress tracker
    progress_tracker_t progress_tracker(count, &logger);

    // get hash block size
    size_t hash_block_size = rw_manager.settings.hash_block_size;

    // set source data for the insert
    lmdb_source_data_t source_data(repository_name, "add_random", 0, "");

    // insert count random hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // update progress tracker
      progress_tracker.track();

      // generate file offset or 0 if hash_block_size is 0
      uint64_t file_offset = i * hash_block_size;

      // add element
      rw_manager.insert(lmdb_helper::random_binary_hash(),
                        file_offset,
                        rw_manager.settings.hash_block_size,
                        source_data,
                        "");
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_random");
    logger.add_hashdb_changes(rw_manager.changes);
    logger.add("hash_store_size", rw_manager.size());
    logger.close();

    // also write changes to cout
    std::cout << rw_manager.changes << "\n";
    std::cout << "hash store size: " << rw_manager.size() << "\n";
  }

  // functional analysis and testing: scan_random
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& count_string) {

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // initialize random seed
    srand (time(NULL));

    // start logger
    logger_t logger(hashdb_dir, "scan_random");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("count", count);

    // scan sets of random hashes where hash values are unlikely to match
    logger.add_timestamp("begin scan_random with random hash on hashdb");
    for (uint64_t i=1; i<=count; ++i) {
      std::string binary_hash = lmdb_helper::random_binary_hash();
      size_t match_count = ro_manager.find_count(binary_hash);
      if (match_count > 0) {
        std::cout << "Match found, hash '"
                  << lmdb_helper::binary_hash_to_hex(binary_hash)
                  << "', count: " << match_count << "\n";
      }

      // periodically log scan count
      if (i % 1000000 == 0) {
        std::stringstream ss;
        ss << "Scanning random hash " << i << " of " << count;
        std::cout << ss.str() << std::endl;
        logger.add_timestamp(ss.str());
      }
    }

    // close logger
    logger.add_timestamp("end scan_random");
    logger.close();
  }
*/

#endif

