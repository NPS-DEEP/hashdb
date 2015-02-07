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
#include "lmdb_hash_store.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_source_it_data.hpp"
#include "lmdb_helper.h"
#include "bloom_filter_manager.hpp"
#include "lmdb_change_manager.hpp"
#include "lmdb_reader_manager.hpp"
#include "logger.hpp"
#include "dfxml_hashdigest_reader.hpp"
#include "dfxml_import_consumer.hpp"
#include "dfxml_scan_consumer.hpp"
#include "dfxml_scan_expanded_consumer.hpp"
#include "dfxml_hashdigest_writer.hpp"
#include "tab_hashdigest_reader.hpp"
#include "tcp_server_manager.hpp"
#include "hashdb.hpp"
#include "random_key.hpp"
#include "progress_tracker.hpp"
#include "feature_file_reader.hpp"
#include "feature_line.hpp"
#include "json_formatter.hpp"

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
// no vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// no vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// no vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
  // support expalin_identified_blocks
  typedef std::map<hash_t, std::string> hashes_t;

  private:

  // copy source metadata missing in db2 from db1 to db2
  static void copy_source_metadata(const hashdb_manager_t& hashdb1,
                                   hashdb_manager_t& hashdb2) {

    // iterate through source IDs in db2
    for (source_lookup_index_manager_t::source_lookup_index_iterator_t it =
                             hashdb2.begin_source_lookup_index();
                             it != hashdb2.end_source_lookup_index(); ++it) {

      // get the db2 source pair
      uint64_t source_id2 = it->key;

      // see if db2 already has the source metadata
      std::pair<bool, source_metadata_t> metadata2_pair =
                                    hashdb2.find_source_metadata(source_id2);
      if (metadata2_pair.first == true) {
        // db2 already has metadata so leave it alone
        continue;
      }

      // try to get metadata from db1

      // get the source string pair from db2
      std::pair<bool, std::pair<std::string, std::string> >
                       source_string2_pair = hashdb2.find_source(source_id2);
      if (source_string2_pair.first == false) {
        std::cerr << "Invalid source lookup index value " << source_id2
                  << " encountered during metadata copy.\nDatabase "
                  << hashdb2.hashdb_dir << "may be corrupt.  Aborting.\n";
        exit(1);
      }

      // get equivalent source ID in db1
      std::pair<bool, uint64_t> source_id1_pair = hashdb1.find_source_id(
                                      source_string2_pair.second.first,
                                      source_string2_pair.second.second);

      if (source_id1_pair.first == false) {
        // db1 doesn't have the source, so drop it
        continue;
      }

      // get metadata from db1
      uint64_t source_id1 = source_id1_pair.second;
      std::pair<bool, source_metadata_t> metadata1_pair =
                                    hashdb1.find_source_metadata(source_id1);
      if (metadata1_pair.first == false) {
        // db1 has the source but not the metadata, so drop it
        continue;
      }

      // copy the metadata from db1 to db2
      hashdb2.insert_source_metadata(source_id2,
                                     metadata1_pair.second.filesize,
                                     metadata1_pair.second.hashdigest);
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

  // print header information
  static void print_header(const std::string& command_id) {
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# " << command_id << "\n"
              << "# command_line: " << command_line_t::command_line_string << "\n";
  }

  // print the scan output vector
  static void print_scan_output(
              const std::vector<std::string>& scan_input,
              const hashdb_t::scan_output_t& scan_output) {

    // check that there are matches
    if (scan_output.size() == 0) {
      std::cout << "There are no matches.\n";
      return;
    }

    // print matches
    for (hashdb_t::scan_output_t::const_iterator it=scan_output.begin(); it != scan_output.end(); ++it) {
      std::cout << "[\""
                << lmdb_helper::binary_hash_to_hex(scan_input[it->first])
                << "\",{\"count\":" << it->second << "}]\n";
    }
  }

  // print source fields
  static void print_source_fields(const hashdb_manager_t& hashdb_manager,
                                  uint64_t source_lookup_index,
                                  std::ostream& os) {
    // get the repository name and filename
    std::pair<bool, std::pair<std::string, std::string> > source_pair =
                             hashdb_manager.find_source(source_lookup_index);

    // get source metadata, if available
    std::pair<bool, source_metadata_t> metadata_pair =
                     hashdb_manager.find_source_metadata(source_lookup_index);

    // print the source ID
    os << "\"source_id\":" << source_lookup_index;

    // print the source
    if (source_pair.first == true) {
      os << ",\"repository_name\":\""
         << json_formatter_t::escape_json(source_pair.second.first)
         << "\",\"filename\":\""
         << json_formatter_t::escape_json(source_pair.second.second)
         << "\"";
    }

    if (metadata_pair.first == true) {
      // print the metadata
      os << ",\"filesize\":" << metadata_pair.second.filesize
         << ",\"file_hashdigest\":\"" << metadata_pair.second.hashdigest.hexdigest()
         << "\"";
    }
  }

  // ingest table of relevant hashes and table of their sources
  static void identify_hashes_and_sources(
                            const lmdb_hash_store_t& hash_store,
                            const lmdb_source_store_t& source_store,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max,
                            hashes_t& hashes,
                            std::set<uint64_t>& source_lookup_indexes) {

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);
    while (!reader.at_eof()) {

      feature_line_t feature_line = reader.read();

      // get the binary hash from the hex hash
      std::string binary_hash == lmdb_helper::hex_to_binary_hash(feature_line.feature);

      // add hash to hash set
      std::pair<std::map<std::string, std::string>::iterator, bool> insert_pair =
                         hashes.insert(std::pair<std::string, std::string>(
                         binary_hash, feature_line.context));

      // do not re-process hashes that are already in the hash set
      if (insert_pair.second == false) {
        continue;
      }
  
      // do not add sources for this hash if count > requested max
      if (hashdb_manager.find_count(binary_hash) > requested_max) {
        continue;
      }

      // loop over entries containing this hash value
      lmdb_hash_it_data_t hash_it_data = hash_store.find_begin(binary_hash);

      while (hash_it_data.is_valid) {

        // add the source lookup index to the source lookup index set
        source_lookup_indexes.insert(hash_it_data.sorce_lookup_index);

        hash_store.find_next();
      }
    }
  }

  // print table of relavent hashes
  static void print_identified_hashes(
                            const hashdb_manager_t& hashdb_manager,
                            const hashes_t& hashes,
                            const std::set<uint64_t>& source_lookup_indexes) {

    if (hashes.size() == 0) {
      std::cout << "# There are no hashes to report.\n";
      return;
    }

    // iterate through block hashes
    for (hashes_t::const_iterator it = hashes.begin(); it != hashes.end(); ++it) {

      // prepare block hash line to print
      std::stringstream ss;
      ss << "[\"" << lmdb_helper::binary_hash_to_hex(it->first) << "\"";

      // get the context field
      std::string context = it->second;

      // add the reduced context field
      ss << "," << context;

      // add the source array open bracket
      ss << ",[";

/*zz TBD
      // get the multimap iterator for this hash value
      hash_store_key_iterator_range_t it_pair = hashdb_manager.find(it->first);

      // the user did something wrong if there is no range
      if (it_pair.first == it_pair.second) {
        std::cout << "# Invalid hash, incorrect file or database, " << it->first.hexdigest() << "\n";
        continue;
      }

      // track when to put in the comma
      bool found_identified_source = false;

      // print sources associated with this hash value
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get the source lookup index and file offset for this entry
        uint64_t source_lookup_index = hashdb_manager.source_id(it_pair.first);
        if (source_lookup_indexes.find(source_lookup_index) == source_lookup_indexes.end()) {
          // do not report sources that are not identified
          continue;
        }

        // consume identified source
        if (found_identified_source) {
          // prepend comma
          ss << ",";
        } else {
          // a printable source has been found
          found_identified_source = true;
        }

        // get the file offset
        uint64_t file_offset = hashdb_manager.file_offset(it_pair.first);

        // add source_id and file_offset entry
        ss << "{\"source_id\":" << source_lookup_index
           << ",\"file_offset\":" << file_offset
           << "}";
      }

      // done printing the source array
      ss << "]";

      // done printing this block hash
      ss << "]";

      // the block hash is interesting if it has at least one referenced source
      if (found_identified_source) {
        std::cout << ss.str() << "\n";
      }
    }
*/
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
      print_source_fields(hashdb_manager, *it, std::cout);
      std::cout << "}\n";
    }
  }

// no ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// no ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// no ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  public:

  // create
  static void create(const hashdb_settings_t& settings,
                     const std::string& hashdb_dir) {

    // create the hashdb directory
    hashdb_directory_manager_t::create_new_hashdb_dir(hashdb_dir);

    // write the settings
    hashdb_settings_store_t::write_settings(hashdb_dir, settings);

    // create the new stores
    lmdb_hash_store_t(hashdb_dir, RW_NEW);
    lmdb_name_store_t(hashdb_dir, RW_NEW);
    lmdb_source_store_t(hashdb_dir, RW_NEW);

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

    // open change manager
    change_manager_t change_manager(hashdb_dir);

    logger_t logger(hashdb_dir, "import");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add_timestamp("begin import");

    // start progress tracker
    progress_tracker_t progress_tracker(0, &logger);

    // create the DFXML import consumer
    dfxml_import_consumer_t import_consumer(&change_manager, &progress_tracker);

    // run the dfxml hashdigest reader using the import hash consumer
    std::pair<bool, std::string> do_read_pair =
    dfxml_hashdigest_reader_t<dfxml_import_consumer_t>::
             do_read(dfxml_file, repository_name, &import_consumer);

    // close tracker
    progress_tracker.done();

    if (do_read_pair.first == true) {
      // good, reader worked

      // close logger
      logger.add_timestamp("end import");
      logger.add_hashdb_changes(hashdb_manager.changes);
      logger.close();
      history_manager_t::append_log_to_history(hashdb_dir);

      // also write changes to cout
      std::cout << hashdb_manager.changes << "\n";
    } else {
      // bad, reader failed
      // close logger
      logger.add_timestamp("end import, import failed");
      logger.close();
      history_manager_t::append_log_to_history(hashdb_dir);

      std::cerr << do_read_pair.second << ".  Import aborted.\n";
    }
  }

  // import_tab
  static void import_tab(const std::string& hashdb_dir,
                     const std::string& tab_file,
                     const std::string& repository_name,
                     uint32_t sector_size) {

    // require that tab file exists
    if (access(tab_file.c_str(), F_OK) != 0) {
      std::cerr << "tab import file '" << tab_file
                << "' does not exist.  Aborting.\n";
      exit(1);
    }

    change_manager_t change_manager(hashdb_dir);

    logger_t logger(hashdb_dir, "import_tab");
    logger.add("tab_file", tab_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    logger.add_timestamp("begin import_tab");

    // start progress tracker
    progress_tracker_t progress_tracker(0, &logger);

    // create the tab reader
    tab_hashdigest_reader_t tab_hashdigest_reader(
            &change_manager, &progress_tracker, repository_name, sector_size);

    // read the tab input
    std::pair<bool, std::string> import_tab_pair =
                                tab_hashdigest_reader.read(tab_file);

    // close tracker
    progress_tracker.done();

    if (import_tab_pair.first == true) {
      // good, reader worked

      // close logger
      logger.add_timestamp("end import_tab");
      logger.add_hashdb_changes(hashdb_manager.changes);
      logger.close();
      history_manager_t::append_log_to_history(hashdb_dir);

      // also write changes to cout
      std::cout << hashdb_manager.changes << "\n";
    } else {
      // bad, reader failed
      // close logger
      logger.add_timestamp("end import_tab, import failed");
      logger.close();
      history_manager_t::append_log_to_history(hashdb_dir);

      std::cerr << import_tab_pair.second << ".  Import from tab file aborted.\n";
    }
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

    // open data resources
    lmdb_hash_store_t hash_store(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // start progress tracker
    progress_tracker_t progress_tracker(hash_store.size());

    // start iterating
    lmdb_hash_it_data_t hash_it_data = hash_store.find_begin();
    while(hash_it_data.is_valid) {
      progress_tracker.track();

      // get source data
      lmdb_source_data_t source_data = source_store.find(
                                hash_it_data.source_lookup_index)
      writer.add_hashdb_element(hash_it_data.binary_hash, 
                                hash_it_data.file_offset,
                                source_data);

      // next
      hash_store.find_next(hash_it_data);
    }
    progress_tracker.done();
  }

  // add A to B
  static void add(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {

    // open hashdb_dir1 for reading
    lmdb_hash_store_t hash_store(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open hashdb_dir2 for writing
    change_manager_t chang_manager(hashdb_dir2);

    logger_t logger(hashdb_dir2, "add");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add");

    // start progress tracker
    progress_tracker_t progress_tracker(hash_store.size());

    // start iterating
    lmdb_hash_it_data_t hash_it_data = hash_store.find_begin();
    while(hash_it_data.is_valid) {
      progress_tracker.track();

      // get source data
      lmdb_source_data_t source_data = source_store.find(
                                hash_it_data.source_lookup_index);

      // add
      change_manager.add_hashdb_element(hash_it_data.binary_hash, 
                                hash_it_data.file_offset,
                                source_data);

      // next
      hash_store.find_next(hash_it_data);
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add");
    logger.add_hashdb_changes(hashdb_manager2.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << hashdb_manager2.changes << "\n";
  }

  // add_multiple A and B to C
  static void add_multiple(const std::string& hashdb_dir1,
                           const std::string& hashdb_dir2,
                           const std::string& hashdb_dir3) {

    // open hashdb_dir1 and hashdb_dir2 for reading
    lmdb_hash_store_t hash_store1(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store1(hashdb_dir, READ_ONLY);
    lmdb_hash_store_t hash_store2(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store2(hashdb_dir, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb_dir3 for writing
    change_manager_t chang_manager(hashdb_dir3);

    logger_t logger(hashdb_dir3, "add_multiple");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin add_multiple");

    // start iterating
    lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_begin();
    lmdb_hash_it_data_t hash_it_data2 = hash_store2.find_begin();

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_manager1.map_size() +
                                        hashdb_manager2.map_size(), &logger);

    while (hash_it_data1.is_valid && hash_it_data2.is_valid) {
      if (hash_it_data1.binary_hash <= hash_it_data2.binary_hash) {
        lmdb_source_data_t source_data1 = source_store1.find(
                                         hash_it_data1.source_lookup_index);
        change_manager.add_hash_element(
                                hash_it_data1.file_offset,
                                source_data1);
        hash_it_data1 = hash_store1.find_next(hash_it_data1);
      } else {
        lmdb_source_data_t source_data2 = source_store2.find(
                                         hash_it_data2.source_lookup_index);
        change_manager.add_hash_element(
                                hash_it_data2.file_offset,
                                source_data2);
        hash_it_data2 = hash_store2.find_next(hash_it_data2);
      }
      progress_tracker.track();
    }

    // hashdb1 or hashdb2 has become depleted so insert remaining elements
    while (hash_it_data1.is_valid) {
        lmdb_source_data_t source_data1b = source_store1.find(
                                         hash_it_data1.source_lookup_index);
        change_manager.add_hash_element(
                                hash_it_data1.file_offset,
                                source_data1b);
        hash_it_data1 = hash_store1.find_next(hash_it_data1);
    }
    while (hash_it_data2.is_valid) {
        lmdb_source_data_t source_data2b = source_store2.find(
                                         hash_it_data2.source_lookup_index);
        change_manager.add_hash_element(
                                hash_it_data2.file_offset,
                                source_data2b);
        hash_it_data2 = hash_store2.find_next(hash_it_data2);
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_multiple");
    logger.add_hashdb_changes(change_manager.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << hashdb_manager3.changes << "\n";
  }

  // add A to B when repository matches
  static void add_repository(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& repository_name) {

    // open hashdb_dir1 for reading
    lmdb_hash_store_t hash_store(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open hashdb_dir2 for writing
    change_manager_t chang_manager(hashdb_dir2);

    logger_t logger(hashdb_dir2, "add_repository");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin add_repository");

    // start progress tracker
    progress_tracker_t progress_tracker(hash_store.size());

    // start iterating
    lmdb_hash_it_data_t hash_it_data = hash_store.find_begin();
    while(hash_it_data.is_valid) {
      progress_tracker.track();

      // get source data
      lmdb_source_data_t source_data = source_store.find(
                                hash_it_data.source_lookup_index);

      // conditionally add
      if(repository_name == source_data.repository_name) {
        change_manager.add_hashdb_element(hash_it_data.binary_hash, 
                                hash_it_data.file_offset,
                                source_data);
      }

      // next
      hash_store.find_next(hash_it_data);
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end add_repository");
    logger.add_hashdb_changes(hashdb_manager2.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << hashdb_manager2.changes << "\n";
  }

  // intersect
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& hashdb_dir3) {

    // open hashdb_dir1 and hashdb_dir2 for reading
    lmdb_hash_store_t hash_store1(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store1(hashdb_dir, READ_ONLY);
    lmdb_hash_store_t hash_store2(hashdb_dir, READ_ONLY);
    lmdb_name_store_t name_store2(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store2(hashdb_dir, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb_dir3 for writing
    change_manager_t chang_manager(hashdb_dir3);

    logger_t logger(hashdb_dir3, "intersect");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect");

    // start iterating over 1
    lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_begin();
    while(hash_it_data1.is_valid) {
      progress_tracker.track();

      // get source 1
      lmdb_source_data_t source_data1 = source_store1.find(
                                hash_it_data1.source_lookup_index);

      // get name 2
      std::pair<bool, uint64_t> pair = name_store2.find(
                                source_data1.repository_name,
                                source_data1.filename);

      if (pair.first) {
        // see if exact match
        lmdb_hash_it_data_t hash_it_data2 = hash_store2.find(
                                hash_it_data1.binary_hash,
                                pair.second,  // source lookup index
                                hash_it_data1.file_offset);

        if (hash_it_data2.is_valid) {
          // there is an exact match so add
          change_manager.insert(hash_it_data1.binary_hash,
                                hash_it_data1.file_offset,
                                source_data1);
        }
      }

      // next
      lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_next();
    }

    // add any missing source fields
    logger.add_timestamp("add source fields 1");
    change_manager.add_source_data(source_store1);
    logger.add_timestamp("add source fields 2");
    change_manager.add_source_data(source_store2);

    // close logger
    logger.add_timestamp("end intersect");
    logger.add_hashdb_changes(change_manager.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << manager3.changes << "\n";
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& hashdb_dir3) {

    // open hashdb_dir1 and hashdb_dir2 for reading
    lmdb_hash_store_t hash_store1(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store1(hashdb_dir, READ_ONLY);
    lmdb_hash_store_t hash_store2(hashdb_dir, READ_ONLY);
    lmdb_name_store_t name_store2(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store2(hashdb_dir, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb_dir3 for writing
    change_manager_t chang_manager(hashdb_dir3);

    logger_t logger(hashdb_dir3, "intersect_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin intersect_hash");

    // start iterating over 1
    lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_begin();
    while(hash_it_data1.is_valid) {
      progress_tracker.track();

      lmdb_hash_it_data_t hash_it_data2 =
                        hash_store2.find_first(hash_it_data1.binary_hash);
      if (hash_it_data2.is_valid) {

        // match so add all hash entries from both
        hash_it_data1 = add_hashes(hash_store1, source_store1, hash_it_data1.binary_hash);
        hash_it_data2 = add_hashes(hash_store2, source_store2, hash_it_data2.binary_hash);
      }
    }

    // close logger
    logger.add_timestamp("end intersect_hash");
    logger.add_hashdb_changes(manager3.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << manager3.changes << "\n";
  }

  // subtract: hashdb1 - hashdb 2 -> hashdb3 for exact match
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& hashdb_dir3) {


    // open hashdb_dir1 and hashdb_dir2 for reading
    lmdb_hash_store_t hash_store1(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store1(hashdb_dir, READ_ONLY);
    lmdb_hash_store_t hash_store2(hashdb_dir, READ_ONLY);
    lmdb_name_store_t name_store2(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store2(hashdb_dir, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb_dir3 for writing
    change_manager_t chang_manager(hashdb_dir3);

    logger_t logger(hashdb_dir3, "subtract");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin subtract");

    // start iterating over 1
    lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_begin();
    while(hash_it_data1.is_valid) {
      progress_tracker.track();

      // get source 1
      lmdb_source_data_t source_data1 = source_store1.find(
                                hash_it_data1.source_lookup_index);

      // get name 2
      std::pair<bool, uint64_t> pair = name_store2.find(
                                source_data1.repository_name,
                                source_data1.filename);

      if (pair.first) {
        // see if exact match
        lmdb_hash_it_data_t hash_it_data2 = hash_store2.find(
                                hash_it_data1.binary_hash,
                                pair.second,  // source lookup index
                                hash_it_data1.file_offset);

        if (!hash_it_data2.is_valid) {
          // there is no exact match so add
          change_manager.insert(hash_it_data1.binary_hash,
                                hash_it_data1.file_offset,
                                source_data1);
        }
      }

      // next
      lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_next();
    }

    // close logger
    logger.add_timestamp("end subtract");
    logger.add_hashdb_changes(change_manager.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << manager3.changes << "\n";
  }

  // subtract_hash: hashdb1 - hashdb 2 -> hashdb3
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& hashdb_dir3) {

    // open hashdb_dir1 and hashdb_dir2 for reading
    lmdb_hash_store_t hash_store1(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store1(hashdb_dir, READ_ONLY);
    lmdb_hash_store_t hash_store2(hashdb_dir, READ_ONLY);
    lmdb_name_store_t name_store2(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store2(hashdb_dir, READ_ONLY);

    // if hashdb3 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir3)) {
      create(hashdb_manager1.settings, hashdb_dir3);
    }

    // open hashdb_dir3 for writing
    change_manager_t chang_manager(hashdb_dir3);

    logger_t logger(hashdb_dir3, "subtract_hash");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    logger.add_timestamp("begin subtract_hash");

    // start iterating over 1
    lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_begin();
    while(hash_it_data1.is_valid) {
      progress_tracker.track();

      // look for hash match in hash store 2
      lmdb_hash_it_data_t hash_it_data2 = hash_store2.find_first(hash_it_data1.binary_hash);

      // copy hash 1 if not in 2
      if (!hash_it_data2.is_valid) {
        lmdb_source_data_t source_data1 = source_store1.find(hash_it_data1.source_lookup_index);
        change_manager.insert(hash_it_data1.binary_hash, hash_it_data1.file_offset, source_data1);
      }

      // next
      lmdb_hash_it_data_t hash_it_data1 = hash_store1.find_next();
    }

    // close logger
    logger.add_timestamp("end subtract_hash");
    logger.add_hashdb_changes(manager3.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << manager3.changes << "\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir1,
                          const std::string& hashdb_dir2) {

    // open hashdb_dir1 for reading
    lmdb_hash_store_t hash_store(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // if hashdb2 does not exist, create it with settings from hashdb1
    if (!hashdb_directory_manager_t::is_hashdb_dir(hashdb_dir2)) {
      create(hashdb_manager1.settings, hashdb_dir2);
    }

    // open hashdb_dir2 for writing
    change_manager_t chang_manager(hashdb_dir2);

    logger_t logger(hashdb_dir2, "deduplicate");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add_timestamp("begin deduplicate");

    // start progress tracker
    progress_tracker_t progress_tracker(hash_store.size());

    // start deduplicate
    lmdb_hash_it_data_t hash_it_data = hash_store.find_begin();
    while(hash_it_data.is_valid) {
      progress_tracker.track();

      // keep if count is 1
      hash_store.find_count(hash_it_data.binary_hash);
      if (count == 1) {

        // get source data
        lmdb_source_data_t source_data = source_store.find(
                                hash_it_data.source_lookup_index);

        // add
        change_manager.add_hashdb_element(hash_it_data.binary_hash, 
                                hash_it_data.file_offset,
                                source_data);

      // next
      hash_store.find_next(hash_it_data);
    }

    // close tracker
    progress_tracker.done();

    // close logger
    logger.add_timestamp("end deduplicate");
    logger.add_hashdb_changes(hashdb_manager2.changes);
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);

    // merge history
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << hashdb_manager2.changes << "\n";
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

    // create the dfxml scan consumer
    dfxml_scan_consumer_t scan_consumer(scan_input);

    // run the dfxml hashdigest reader using the scan consumer
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_consumer_t>::
                        do_read(dfxml_file, repository_name, &scan_consumer);

    // perform the scan
    hashdb.scan(*scan_input, *scan_output);

    // print header information
    print_header("scan-command-Version: 2");

    // show the matches
    print_scan_output(*scan_input, *scan_output);

    // delete heap allocation
    delete scan_input;
    delete scan_output;
  }

  // scan expanded
  static void scan_expanded(const std::string& hashdb_dir,
                            const std::string& dfxml_file,
                            uint32_t requested_max) {

    // open hash store
    lmdb_hash_store_t hash_store(hashdb_dir, READ_ONLY);
    lmdb_source_store_t source_store(hashdb_dir, READ_ONLY);

    // create the dfxml scan_expanded consumer
    dfxml_scan_expanded_consumer_t scan_expanded_consumer(
                                  &hash_store, &source_store, requested_max);
    // print header information
    print_header("scan_expanded-command-Version: 2");

    // run the dfxml hashdigest reader using the scan consumers
    std::string repository_name = "not used";
    dfxml_hashdigest_reader_t<dfxml_scan_expanded_consumer_t>::
                              do_read(dfxml_file, repository_name,
                              &scan_expanded_consumer);
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

    // print header information
    print_header("scan_hash-command-Version: 2");

    // show the matches
    print_scan_output(*scan_input, *scan_output);

    // delete heap allocation
    delete scan_input;
    delete scan_output;
  }

  // scan expanded hash
  static void scan_expanded_hash(const std::string& hashdb_dir,
                            const std::string& hash_string,
                            uint32_t requested_max) {

/* zz TBD
    // validate the hash string
    std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(hash_string);
    if (hash_pair.first == false) {
      std::cerr << "Invalid hash value '" << hash_string << "'.  Aborting.\n";
      exit(1);
    }

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // find matching range for this key
    hash_store_key_iterator_range_t it_pair = hashdb_manager.find(hash_pair.second);

    // check for no match
    if (it_pair.first == it_pair.second) {
      std::cout << "There are no matches.\n";
      return;
    }

    // print header information
    print_header("scan_expanded_hash-command-Version: 2");

    // print the expanded hash
    json_formatter_t json_formatter(&hashdb_manager, requested_max);
    json_formatter.print_expanded(it_pair);
    std::cout << "\n";
*/
  }

  // server
  static void server(const std::string& hashdb_dir,
                     const std::string& port_number_string) {
std::cerr << "server not implemented\n";
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

    // print header information
    print_header("sources-command-Version: 2");

    // report each entry
    for (; it != hashdb_manager.end_source_lookup_index(); ++it) {
      std::cout << "{";
      print_source_fields(hashdb_manager, it->key, std::cout);
      std::cout << "}\n";
    }
  }

  // show hashdb hash histogram
  static void histogram(const std::string& hashdb_dir) {
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
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& source_id_string) {

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
  }

  // expand identified_blocks.txt
  static void expand_identified_blocks(const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {

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

    // print header information
    print_header("explain_identified_blocks-command-Version: 2");

    // print identified hashes
    std::cout << "# hashes\n";
    print_identified_hashes(hashdb_manager, *hashes, *source_lookup_indexes);

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
    hash_store_key_iterator_t it = hashdb_manager.begin_key();
    if (settings.bloom1_is_used) {
      progress_tracker_t progress_tracker(hashdb_manager.map_size(), &logger);
      while (it != hashdb_manager.end_key()) {
        // add the hash to the bloom filter
        bloom_filter_manager.add_hash_value(key(it));
        ++it;
        progress_tracker.track();
      }

      // close tracker
      progress_tracker.done();
    }

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
    history_manager_t::append_log_to_history(hashdb_dir);

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
  }

  /**
   * Scans for random hash values that are unlikely to match.
   * Disable the Bloom filter for B-Tree timing.
   */
  // functional analysis and testing: scan_random
  static void scan_random(const std::string& hashdb_dir) {

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
        uint32_t hash_index = scan_output->at(0).first;
        std::cerr << "Unexpected event: " << scan_output->size()
                  << " matches found in set, example hash: "
                  << scan_input->at(hash_index).hexdigest() << "\n";
      }
    }

    // close logger
    logger.add_timestamp("end scan_random");
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir);
  }
};

#endif

