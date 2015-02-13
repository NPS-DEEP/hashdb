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
#include "globals.hpp"
#include "file_helper.hpp"
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "history_manager.hpp"
#include "lmdb_hash_store.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_source_it_data.hpp"
#include "lmdb_helper.h"
#include "bloom_filter_manager.hpp"
#include "lmdb_rw_manager.hpp"
#include "lmdb_ro_manager.hpp"
#include "lmdb_rw_new.hpp"
#include "expand_manager.hpp"
#include "logger.hpp"
#include "dfxml_hashdigest_reader.hpp"
#include "dfxml_import_consumer.hpp"
#include "dfxml_scan_consumer.hpp"
#include "dfxml_scan_expanded_consumer.hpp"
#include "dfxml_hashdigest_writer.hpp"
#include "tab_hashdigest_reader.hpp"
#include "hashdb.hpp"
#include "random_binary_hash.hpp"
#include "progress_tracker.hpp"
#include "feature_file_reader.hpp"
#include "feature_line.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
//zz #include <boost/filesystem.hpp> // for scan_random command

/**
 * Defines the static commands that hashdb_manager can execute.
 * This totally static class is a class rather than an namespace
 * so it can have private members.
 */

class commands_t {
  private:
  static void create_if_new(const std::string& hashdb_dir,
                            const hashdb_settings_t& settings) {
    if (!file_helper::is_hashdb_dir(hashdb_dir)) {
      create(hashdb_dir, settings);
    }
  }

  static void require_different(const std::string hashdb_dir1,
                                const std::string hashdb_dir2) {

    // databases should not be the same one
    if (hashdb_dir1 == hashdb_dir2) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb_dir1 << "', '"
                << hashdb_dir2 << "'\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  static void require_different(const std::string hashdb_dir1,
                                const std::string hashdb_dir2,
                                const std::string hashdb_dir3) {

    // databases should not be the same one
    if (hashdb_dir1 == hashdb_dir2
     || hashdb_dir2 == hashdb_dir3
     || hashdb_dir3 == hashdb_dir1) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb_dir1 << "', '"
                << hashdb_dir2 << "', '"
                << hashdb_dir3 << "'\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  // print header information
  static void print_header(const std::string& command_id) {
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# " << command_id << "\n"
              << "# command_line: " << globals_t::command_line_string << "\n";
  }

  // helper for hash copy
  static inline void copy_hash(lmdb_hash_it_data_t hash_it_data,
                        const lmdb_ro_manager_t& ro_manager,
                        lmdb_rw_manager_t& rw_manager) {
    lmdb_source_data_t source_data = ro_manager.find_source(
                                       hash_it_data.source_lookup_index);
    rw_manager.insert(hash_it_data.binary_hash,
                      source_data,
                      hash_it_data.file_offset);
  }

  public:

  // create
  static void create(const std::string& hashdb_dir,
                     const hashdb_settings_t& settings) {

    // create the hashdb directory
    lmdb_rw_new::create(hashdb_dir, settings);

    // log the creation event
    logger_t logger(hashdb_dir, "create");
    logger.add("hashdb_dir", hashdb_dir);
    logger.add_hashdb_settings(settings);
    logger.close();

    std::cout << "Database created.\n";
  }

  // import
  static void import(const std::string& hashdb_dir,
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
      // close logger
      std::cerr << do_read_pair.second << ".  Import aborted.\n";
    }
  }

  // import_tab
  static void import_tab(const std::string& hashdb_dir,
                     const std::string& tab_file,
                     const std::string& repository_name,
                     uint32_t sector_size) {

    // require that tab file exists
    file_helper::require_tab_file(tab_file);

    // open hashdb RW manager
    lmdb_rw_manager_t rw_manager(hashdb_dir);

    logger_t logger(hashdb_dir, "import_tab");
    logger.add("tab_file", tab_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add_timestamp("begin import_tab");

    // start progress tracker
    progress_tracker_t progress_tracker(0, &logger);

    // create the tab reader
    tab_hashdigest_reader_t tab_hashdigest_reader(
            &rw_manager, &progress_tracker, repository_name, sector_size);

    // read the tab input
    std::pair<bool, std::string> import_tab_pair =
                                tab_hashdigest_reader.read(tab_file);

    // close tracker
    progress_tracker.done();

    if (import_tab_pair.first == true) {
      // good, reader worked

      // close logger
      logger.add_timestamp("end import_tab");
      logger.add_hashdb_changes(rw_manager.changes);
      logger.close();

      // also write changes to cout
      std::cout << rw_manager.changes << "\n";
    } else {
      // bad, reader failed
      // close logger
      logger.add_timestamp("end import_tab, import failed");
      logger.close();

      std::cerr << import_tab_pair.second << ".  Import from tab file aborted.\n";
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
                                hash_it_data.file_offset);
      hash_it_data = ro_manager.find_next(hash_it_data);
    }
    progress_tracker.done();
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {
    require_different(hashdb_dir1, hashdb_dir2);

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

    require_different(hashdb_dir1, hashdb_dir2, hashdb_dir3);

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
    progress_tracker_t progress_tracker(ro_manager1.size(), &logger);

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
      progress_tracker.track();
    }

    while (hash_it_data2.is_valid) {
      copy_hash(hash_it_data2, ro_manager1, rw_manager3);
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

    require_different(hashdb_dir1, hashdb_dir2);

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
                           source_data,
                           hash_it_data.file_offset);
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

    require_different(hashdb_dir1, hashdb_dir2, hashdb_dir3);

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
                                 hash_it_data1.file_offset)) {

        rw_manager3.insert(hash_it_data1.binary_hash,
                           source_data1,
                           hash_it_data1.file_offset);
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

    require_different(hashdb_dir1, hashdb_dir2, hashdb_dir3);

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

    require_different(hashdb_dir1, hashdb_dir2, hashdb_dir3);

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
                                  hash_it_data1.file_offset)) {

        rw_manager3.insert(hash_it_data1.binary_hash,
                           source_data1,
                           hash_it_data1.file_offset);
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

    require_different(hashdb_dir1, hashdb_dir2, hashdb_dir3);

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

    require_different(hashdb_dir1, hashdb_dir2);

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
                           source_data,
                           hash_it_data.file_offset);
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
    require_different(hashdb_dir1, hashdb_dir2);

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
    print_header("scan-command-Version: 2");

    // run the dfxml hashdigest reader using the scan consumer
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_consumer_t>::
                        do_read(dfxml_file,
                                repository_name,
                                ro_manager.settings.hash_block_size,
                                &scan_consumer);
  }

  // scan expanded
  static void scan_expanded(const std::string& hashdb_dir,
                            const std::string& dfxml_file,
                            uint32_t max_sources) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // create the dfxml scan consumer
    dfxml_scan_expanded_consumer_t scan_expanded_consumer(&ro_manager, max_sources);

    // run the dfxml hashdigest reader using the scan consumer
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_expanded_consumer_t>::
                        do_read(dfxml_file,
                                repository_name,
                                ro_manager.settings.hash_block_size,
                                &scan_expanded_consumer);
  }

  // scan hash
  static void scan_hash(const std::string& hashdb_dir,
                   const std::string& hash_string) {

    // open DB
    lmdb_ro_manager_t ro_manager(hashdb_dir);

    // get the binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(hash_string);

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

    // check for no match
    if (ro_manager.find_count(binary_hash) == 0) {
      std::cout << "There are no matches.\n";
      return;
    }

    // open the expand manager
    expand_manager_t expand_manager(&ro_manager, max_sources);
    expand_manager.print_header();
    expand_manager.expand(binary_hash);
  }

  // server
  static void server(const std::string& hashdb_dir,
                     const std::string& port_number_string) {
/*

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
*/
  }

  // show hashdb size values
  static void size(const std::string& hashdb_dir) {
/*
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
*/
  }

  // print sources referenced in this database
  static void sources(const std::string& hashdb_dir) {
/*

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // get the source lookup index iterator
    source_lookup_index_manager_t::source_lookup_index_iterator_t it = hashdb_manager.begin_source_lookup_index();

    // see if the source lookup index map is empty
    if (it == hashdb_manager.end_source_lookup_index()) {
      std::cout << "There are no sources in this database.\n";
      return;
    }

    // print header information
    print_header("sources-command-Version: 2");

    // report each entry
    for (; it != hashdb_manager.end_source_lookup_index(); ++it) {
      std::cout << "{";
      print_source_fields(hashdb_manager, it->key, std::cout);
      std::cout << "}\n";
    }
*/
  }

  // show hashdb hash histogram
  static void histogram(const std::string& hashdb_dir) {
/*
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);
    hash_store_key_iterator_t it = hashdb_manager.begin_key();

    // there is nothing to report if the map is empty
    if (it == hashdb_manager.end_key()) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_header("histogram-command-Version: 2");

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
    while (it != hashdb_manager.end_key()) {

      // update progress tracker
      progress_tracker.track();

      // get count for this hash
      uint32_t count = hashdb_manager.find_count(key(it));

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
*/
  }

  // show hashdb duplicates for a given duplicates count
  static void duplicates(const std::string& hashdb_dir,
                         const std::string& duplicates_string) {
/*

    // convert duplicates string to number
    uint32_t duplicates_number;
    try {
      duplicates_number = boost::lexical_cast<uint32_t>(duplicates_string);
    } catch(...) {
      std::cerr << "Invalid number of duplicates: '" << duplicates_string << "'\n";
      exit(1);
    }

    // print header information
    print_header("duplicates-command-Version: 2");

    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);
    hash_store_key_iterator_t it = hashdb_manager.begin_key();

    // look through all hashes for entries with this count
    bool any_found = false;
    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    while (it != hashdb_manager.end_key()) {
      uint32_t count = hashdb_manager.find_count(key(it));

      if (count == duplicates_number) {
        any_found = true;
        // this is the same format as that used in print_scan_output
        std::cout << "[\"" << key(it).hexdigest() << "\",{\"count\":" << count << "}]\n";
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
*/
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& source_id_string) {
/*

    // convert count string to number
    uint64_t source_id;
    try {
      source_id = boost::lexical_cast<uint64_t>(source_id_string);
    } catch(...) {
      std::cerr << "Invalid source_id: '" << source_id_string << "'\n";
      exit(1);
    }

    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // check that the source ID exists
    std::pair<bool, std::pair<std::string, std::string> >
                source_string2_pair = hashdb_manager.find_source(source_id);
    if (source_string2_pair.first == false) {
      std::cerr << "The database does not contain source lookup index value " << source_id
                << ".  Aborting.\n";
      exit(1);
    }

    // print header information
    print_header("hash_table-command-Version: 2");

    // print source information
    std::cout << "{";
    print_source_fields(hashdb_manager, source_id, std::cout);
    std::cout << "}\n";

    // show hashes for the requested source
    bool any_found = false;
    // start progress tracker and iterate through keys in O(n) search
    progress_tracker_t progress_tracker(hashdb_manager.map_size());
    hash_store_key_iterator_t it = hashdb_manager.begin_key();
    while (it != hashdb_manager.end_key()) {
      if (hashdb_manager.source_id(it) == source_id) {

        // show the hash and its offset
        std::cout << "[\"" << key(it).hexdigest() << "\",{\"file_offset\":"
                  << hashdb_manager.file_offset(it) << "}]\n";
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
*/
  }

  // expand identified_blocks.txt
  static void expand_identified_blocks(const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {
/*

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);

    // get the json formatter
    json_formatter_t json_formatter(&hashdb_manager, requested_max);

    // print header information
    print_header("expand_identified_blocks-command-Version: 2");

    // read identified blocks from input and write out matches
    // identified_blocks feature consists of offset_string, key, count and flags
    while (!reader.at_eof()) {
      feature_line_t feature_line = reader.read();

      // get the hash from the hash string
      std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(feature_line.feature);
      if (hash_pair.first == false) {
        // bad hash value
        continue;
      }
      hash_t hash = hash_pair.second;

      // find matching range for this key
      hash_store_key_iterator_range_t it_pair = hashdb_manager.find(hash);

      // the user did something wrong if the range was not found
      if (it_pair.first == it_pair.second) {
        std::cout << "# Invalid hash, incorrect file or database, " << hash.hexdigest() << "\n";
        continue;
      }

      // write the forensic path
      std::cout << feature_line.forensic_path << "\t";

      // write the hashdigest
      std::cout << feature_line.feature << "\t";

      // write the opening of the new context
      std::cout << "[";

      // write the old context
      std::cout << feature_line.context;

      // write the separator
      std::cout << ",";

      // write the expanded source
      json_formatter.print_expanded(it_pair);

      // write the closure of the new context
      std::cout << "]\n";
    }
*/
  }

  // explain identified_blocks.txt
  static void explain_identified_blocks(
                            const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {
/*

    commands_explain::explain_identified_blocks(hashdb_dir,
                                                identified_blocks_file,
                                                requested_max);
*/
  }

  // rebuild bloom
  static void rebuild_bloom(const hashdb_settings_t& new_bloom_settings,
                            const std::string& hashdb_dir) {
/*

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
    hash_store_key_iterator_t it = hashdb_manager.begin_key();
    progress_tracker_t progress_tracker(hashdb_manager.map_size(), &logger);
    while (it != hashdb_manager.end_key()) {
      // add the hash to the bloom filter
      bloom_filter_manager.add_hash_value(key(it));
      ++it;
      progress_tracker.track();
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end rebuild_bloom");
    logger.close();

    std::cout << "rebuild_bloom complete.\n";
*/
  }

  // upgrade hashdb
  static void upgrade(const std::string& hashdb_dir) {
/*

    // start logger
    logger_t logger(hashdb_dir, "upgrade");
    logger.add("hashdb_dir", hashdb_dir);

    // open resources
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);

    // close logger
    logger.add_timestamp("end upgrade");
    logger.close();

    std::cout << "Upgrade complete.\n";
*/
  }

  // functional analysis and testing: add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& count_string,
                         const std::string& repository_name) {
/*

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
                                             file_offset));
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_random");
    logger.add_hashdb_changes(hashdb_manager.changes);
    logger.close();

    // also write changes to cout
    std::cout << hashdb_manager.changes << "\n";

    // give user a chance to check memory usage before leaving
    while (true) {
      std::cout << "Done.  Check Memory usage, if desired, then type 'q' to end: ";
      std::string response_string;
      std::getline(std::cin, response_string);
      if (response_string == "q" || response_string == "Q") {
        break;
      }
    }
*/
  }

  /**
   * Scans for random hash values that are unlikely to match.
   * Disable the Bloom filter for B-Tree timing.
   */
  // functional analysis and testing: scan_random
  static void scan_random(const std::string& hashdb_dir) {
/*

    // open hashdb_dir to use for obtaining valid hash values
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // open the hashdb scan service for scanning with random hash
    hashdb_t__<hash_t> hashdb;
    std::pair<bool, std::string> open_pair = hashdb.open_scan(hashdb_dir);
    if (open_pair.first == false) {
      std::cerr << open_pair.second << "\nAborting.\n";
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
      scan_input->clear();
      for (int j=0; j<100000; j++) {
        scan_input->push_back(random_key());
      }

      // make timestamp
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

    // close logger
    logger.add_timestamp("end scan_random");
    logger.close();
*/
  }
};

#endif

