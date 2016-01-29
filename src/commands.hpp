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
#include "import_tab.hpp"
#include "import_json.hpp"
#include "export_json.hpp"
#include "scan_hashes.hpp"
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

  std::pair<bool, std::string> pair;
  hashdb::settings_t settings;

  // try to read hashdb_dir settings
  pair = hashdb::read_settings(hashdb_dir, settings);
  if (pair.first == true) {
    // hashdb_dir already exists
    return pair;
  }

  // no hashdb_dir, so read from_hashdb_dir settings
  pair = hashdb::read_settings(from_hashdb_dir, settings);
  if (pair.first == false) {
    // bad since from_hashdb_dir is not valid
    return pair;
  }

  // create hashdb_dir using from_hashdb_dir settings
  pair = hashdb::create_hashdb(hashdb_dir, settings, command_string);

  return pair;
}

// require hashdb_dir else fail
static void require_hashdb_dir(const std::string& hashdb_dir) {
  std::pair<bool, std::string> pair;
  hashdb::settings_t settings;
  pair = hashdb::read_settings(hashdb_dir, settings);
  if (pair.first == false) {
    std::cout << "Error: " << pair.second << "\n";
    exit(1);
  }
}

static void print_header(const std::string& command_id,
                         const std::string& cmd) {
  std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
            << "# " << command_id << "\n"
            << "# command_line: " << cmd << "\n";
}

namespace commands {

  // ************************************************************
  // new database
  // ************************************************************
  void create(const std::string& hashdb_dir,
              const hashdb::settings_t& settings,
              const std::string& cmd) {

    std::pair<bool, std::string> pair = hashdb::create_hashdb(
                                             hashdb_dir, settings, cmd);

    if (pair.first == true) {
      std::cout << "New database created.\n";
    } else {
      std::cout << "Error: " << pair.second << "\n";
    }
  }

  // ************************************************************
  // import/export
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

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the tab file for reading
    std::ifstream in(tab_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << tab_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    import_tab_t::read(hashdb_dir, tab_file, repository_name, cmd, in);

    // done
    in.close();
    std::cout << "import_tab completed.\n";
  }

  // import_json
  static void import_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the JSON file for reading
    std::ifstream in(json_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << json_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // import the hashdb
    import_json_t::read(hashdb_dir, cmd, in);

    // done
    in.close();
    std::cout << "import_json completed.\n";
  }

  // export_json
  static void export_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the JSON file for writing
    std::ofstream out(json_file.c_str());
    if (!out.is_open()) {
      std::cout << "Error: Cannot open " << json_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // export the hashdb
    export_json_t::write(hashdb_dir, cmd, out);

    // done
    out.close();
    std::cout << "export_json completed.\n";
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
                   const std::string& hashes_file,
                   const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the hashes file for reading
    std::ifstream in(hashes_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << hashes_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    scan_hashes_t::read(hashdb_dir, cmd, in);

    // done
    in.close();
    std::cout << "# scan completed.\n";
  }

  // scan_hash
  static void scan_hash(const std::string& hashdb_dir,
                        const std::string& hex_block_hash,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

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

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    std::cout << manager.sizes() << std::endl;
  }

  // sources
  static void sources(const std::string& hashdb_dir,
                      const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print the sources
    export_json_t::print_sources(hashdb_dir);
  }

  // histogram
  static void histogram(const std::string& hashdb_dir,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

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

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

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
        // show hash with requested duplicates number
        manager.find_expanded_hash(pair.second, *expanded_text);
        std::cout << bin_to_hex(pair.second) << "\t" << *expanded_text << "\n";
        any_found = true;
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

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

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
                         const std::string& count_string,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // initialize random seed
    srand (time(NULL));

    // open manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count);

    // set up the source
    std::pair<bool, uint64_t> id_pair =
                       manager.insert_source_id(hex_to_bin("00"));
    if (id_pair.first == false) {
      std::cerr << id_pair.second << "\n";
      exit(1);
    }
    manager.insert_source_name(id_pair.second, "add_random_repository_name",
                               "add_random_filename");
    manager.insert_source_data(id_pair.second, hex_to_bin("00"), 0, "", 0);

    // insert count random hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // update progress tracker
      progress_tracker.track();

      // add hash
      manager.insert_hash(random_binary_hash(), id_pair.second, 0,"",0,"");
    }
  }

  // scan_random
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& count_string,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // initialize random seed
    srand (time(NULL));

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count);

    // space for match
    std::string* expanded_text = new std::string;

    // scan random hashes where hash values are unlikely to match
    for (uint64_t i=1; i<=count; ++i) {
      std::string binary_hash = random_binary_hash();

      bool found = manager.find_expanded_hash(binary_hash, *expanded_text);

      if (found) {
        std::cout << "Match found, hash "
                  << bin_to_hex(binary_hash)
                  << ": " << expanded_text << "\n";
      }

      // update progress tracker
      progress_tracker.track();
    }
  }
}

#endif

