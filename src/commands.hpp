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
//#include "hashdb_types.h"
#include "command_line.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "history_manager.hpp"
#include "hashdb_manager.hpp"
#include "logger.hpp"
#include "dfxml_hashdigest_reader_manager.hpp"
#include "dfxml_hashdigest_writer.hpp"

/*
#include "dfxml_hashdigest_reader.hpp"
#include "hashdb_exporter.hpp"
#include "query_by_socket_server.hpp"
#include "dfxml/src/dfxml_writer.h"
#include "source_lookup_encoding.hpp"
*/

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

class commands_t {
  public:

  // create
  static void create(const hashdb_settings_t& settings,
                     const std::string& hashdb_dir) {
    hashdb_settings_manager_t::write_settings(hashdb_dir, settings);

    // get hashdb files to exist because other file modes require them
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_NEW);

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
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);
    dfxml_hashdigest_reader_manager_t reader_manager(dfxml_file, repository_name);

    dfxml_hashdigest_reader_manager_t::const_iterator it = reader_manager.begin();

    hashdb_changes_t changes;

    logger.add_timestamp("begin import");

    while (it != reader_manager.end()) {
      hashdb_manager.insert(*it, changes);
      ++it;
    }
    logger.add_timestamp("end import");
    logger.add_hashdb_changes(changes);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // export
  static void do_export(const std::string& hashdb_dir,
                        const std::string& dfxml_file) {
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);
    dfxml_hashdigest_writer_t writer(dfxml_file);
    hashdb_iterator_t it = hashdb_manager.begin();
    while (it != hashdb_manager.end()) {
      writer.add_hashdb_element(*it);
      ++it;
    }
  }

  // copy
  static void copy(const std::string& hashdb_dir1,
                   const std::string& hashdb_dir2) {

    logger_t logger(hashdb_dir2, "copy");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin copy");
    while (it1 != hashdb_manager1.end()) {
      hashdb_manager2.insert(*it1, changes);
      ++it1;
    }
    logger.add_timestamp("end copy");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // merge
  static void merge(const std::string& hashdb_dir1,
                    const std::string& hashdb_dir2,
                    const std::string& hashdb_dir3) {

    logger_t logger(hashdb_dir3, "merge");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    logger.add("hashdb_dir3", hashdb_dir3);
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, READ_ONLY);
    hashdb_manager_t hashdb_manager3(hashdb_dir3, RW_MODIFY);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_iterator_t it2 = hashdb_manager2.begin();
    hashdb_iterator_t it1_end = hashdb_manager1.end();
    hashdb_iterator_t it2_end = hashdb_manager2.end();

    hashdb_changes_t changes;
    logger.add_timestamp("begin merge");

    // while elements are in both, insert ordered by key, prefering it1 first
    while ((it1 != it1_end) && (it2 != it2_end)) {
      if (it1->hashdigest <= it2->hashdigest) {
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


    logger.add_timestamp("end merge");
    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir3);
    history_manager_t::merge_history_to_history(hashdb_dir2, hashdb_dir3);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // remove
  static void remove(const std::string& hashdb_dir1,
                     const std::string& hashdb_dir2) {

    logger_t logger(hashdb_dir2, "remove");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin remove");
    while (it1 != hashdb_manager1.end()) {
      hashdb_manager2.remove(*it1, changes);
      ++it1;
    }
    logger.add_timestamp("end remove");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // remove all
  static void remove_all(const std::string& hashdb_dir1,
                         const std::string& hashdb_dir2) {
    // note: this could be optimised by using map_iterator on hashdb_dir1
    // instead of using hashdb_iterator on hashdb_dir1.

    logger_t logger(hashdb_dir2, "remove all");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin remove all");
    while (it1 != hashdb_manager1.end()) {
      hashdigest_t hashdigest(it1->hashdigest, it1->hashdigest_type);
      hashdb_manager2.remove_key(hashdigest, changes);
      ++it1;
    }
    logger.add_timestamp("end remove all");

    logger.add_hashdb_changes(changes);

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_dir2);
    history_manager_t::merge_history_to_history(hashdb_dir1, hashdb_dir2);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // remove dfxml
  static void remove_dfxml(const std::string& repository_name,
                           const std::string& dfxml_file,
                           const std::string& hashdb_dir) {

    logger_t logger(hashdb_dir, "remove dfxml");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    logger.add("repository_name", repository_name);
    dfxml_hashdigest_reader_manager_t reader_manager(dfxml_file, repository_name);
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);

    dfxml_hashdigest_reader_manager_t::const_iterator it = reader_manager.begin();

    hashdb_changes_t changes;

    logger.add_timestamp("begin remove dfxml");

    while (it != reader_manager.end()) {
      hashdb_manager.remove(*it, changes);
      ++it;
    }
    logger.add_timestamp("end remove dfxml");
    logger.add_hashdb_changes(changes);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // remove all dfxml
  static void remove_all_dfxml(const std::string& dfxml_file,
                               const std::string& hashdb_dir) {

    logger_t logger(hashdb_dir, "remove all dfxml");
    logger.add("dfxml_file", dfxml_file);
    logger.add("hashdb_dir", hashdb_dir);
    std::string repository_name = "not used by remove_all_dfxml";
    dfxml_hashdigest_reader_manager_t reader_manager(dfxml_file, repository_name);
    hashdb_manager_t hashdb_manager(hashdb_dir, RW_MODIFY);

    dfxml_hashdigest_reader_manager_t::const_iterator it = reader_manager.begin();

    hashdb_changes_t changes;

    logger.add_timestamp("begin remove dfxml");

    while (it != reader_manager.end()) {
      hashdigest_t hashdigest(it1->hashdigest, it1->hashdigest_type);
      hashdb_manager2.remove_key(hashdigest, changes);
      ++it;
    }
    logger.add_timestamp("end remove dfxml");
    logger.add_hashdb_changes(changes);

    // also write changes to cout
    std::cout << changes << "\n";
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir1,
                          const std::string& hashdb_dir2) {

    // note: this could be optimised by using map_iterator on hashdb_dir1
    // instead of using hashdb_iterator on hashdb_dir1.

    logger_t logger(hashdb_dir2, "deduplicate");
    logger.add("hashdb_dir1", hashdb_dir1);
    logger.add("hashdb_dir2", hashdb_dir2);
    hashdb_manager_t hashdb_manager1(hashdb_dir1, READ_ONLY);
    hashdb_manager_t hashdb_manager2(hashdb_dir2, RW_MODIFY);

    hashdb_iterator_t it1 = hashdb_manager1.begin();
    hashdb_changes_t changes;
    logger.add_timestamp("begin copy");
    while (it1 != hashdb_manager1.end()) {
      hashdb_manager2.insert(*it1, changes);
      ++it1;
    }
    logger.add_timestamp("end copy");

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
  }

  // server
  static void server(const std::string& hashdb_dir,
                     const std::string& path_or_socket) {
  }

  // query hash
  static void query_hash(const std::string& path_or_socket,
                         const std::string& hashdb_dir) {
  }

  // get hash source
  static void get_hash_source(const std::string& path_or_socket,
                              const std::string& identified_blocks,
                              const std::string& identified_sources) {
  }

  // get hashdb info
  static void get_hashdb_info(const std::string& path_or_socket) {
  }



/*
  static void open_log(const std::string& hashdb_dir,
                       const std::string& command) {

    if (x != 0) {
      // misuse
      assert(0);
    }

    // get the log filename
    std::string log_filename = hashdb_filenames_t::log_filename(hashdb_dir);

    // start command logger
    x = new dfxml_writer(log_filename, false);

    // log the preamble
    x->push("log");
    std::stringstream ss;
    ss << "command_type='" << command << "'";
    x->push("command", ss.str());
//    x->add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line_t::command_line_string);
    x->add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line_t::command_line_string);
  }

  static void close_log() {
    if (x == 0) {
      // misuse
      assert(0);
    }

    // log closure
    x->add_rusage();
    x->pop(); // command
    x->pop(); // log
    delete x;
  }
*/

/*
  static void describe_none_inserted_dfxml() {
    std::cout << "No hashes were inserted from DFXML.  Possible causes:\n"
              << "    The \"-p\" option was not used by md5deep.\n"
              << "    The \"--hash_block_size\" value used does not match that used by md5deep.\n"
              << "    The hashes were already inserted.\n"
              << "    There are too many duplicates for this hash value.\n"
    ;
  }

  static void describe_none_removed_dfxml() {
    std::cout << "No hashes were removed from DFXML.  Possible causes:\n"
              << "    No hashes were provided because the \"-p\" option was not used by md5deep.\n"
              << "    The hashes were already removed.\n"
              << "    The repository name is incorrect, see the \"-r\" option.\n"
              << "    The hashes were never inserted because they would have been duplicates.\n"
    ;
  }

  class inserter_consumer_t {
    private:
    hashdb_db_manager_t* hashdb_db_manager;
    hashdb_change_logger_t* logger;
    public:
    inserter_consumer_t(hashdb_db_manager_t& p_hashdb_db_manager,
                        hashdb_change_logger_t& p_logger) :
                                 hashdb_db_manager(&p_hashdb_db_manager),
                                 logger(&p_logger) {
    }
    void consume(const hashdb_element_t& hashdb_element) {
      hashdb_db_manager->insert_hash_element(hashdb_element, *logger);
    }
  };

  class remover_consumer_t {
    private:
    hashdb_db_manager_t* hashdb_db_manager;
    hashdb_change_logger_t* logger;
    public:
    remover_consumer_t(hashdb_db_manager_t& p_hashdb_db_manager,
                        hashdb_change_logger_t& p_logger) :
                                 hashdb_db_manager(&p_hashdb_db_manager),
                                 logger(&p_logger) {
    }
    void consume(const hashdb_element_t& hashdb_element) {
      hashdb_db_manager->remove_hash_element(hashdb_element, *logger);
    }
  };
*/

  // ************************************************************
  // commands
  // ************************************************************
/*
  public:

  // * Import from DFXML to new hashdb
  static void do_copy_new_dfxml(const std::string& dfxml_infile,
                                const std::string& repository_name,
                                const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_COPY_NEW_DFXML);

    // log control parameters
    logger.add("dfxml_infile", dfxml_infile);
    logger.add("dfxml_infile", dfxml_infile);
    logger.add("repository_name", repository_name);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the new hashdb
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_NEW);

    // log the hashdb settings
    logger.add_hashdb_settings(hashdb_out.hashdb_settings);

    logger.add_timestamp("opened new hashdb");

    // create the inserter consumer
    inserter_consumer_t inserter_consumer(hashdb_out, logger);

    // perform the import using the inserter consumer
    dfxml_hashdigest_reader_t<inserter_consumer_t>::do_read(
                          dfxml_infile, repository_name, &inserter_consumer);

    logger.add_timestamp("done");

    // explain if nothing happened
    if (logger.hashes_inserted == 0) {
      describe_none_inserted_dfxml();
    }

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
  }

  // * Import from DFXML to existing hashdb
  static void do_copy_dfxml(const std::string& dfxml_infile,
                            const std::string& repository_name,
                            const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_COPY_DFXML);

    // log control parameters
    logger.add("dfxml_infile", dfxml_infile);
    logger.add("repository_name", repository_name);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the hashdb to be modified
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_MODIFY);

    logger.add_timestamp("opened hashdb");

    // create the inserter consumer
    inserter_consumer_t inserter_consumer(hashdb_out, logger);

    // perform the import using the inserter consumer
    dfxml_hashdigest_reader_t<inserter_consumer_t>::do_read(
                           dfxml_infile, repository_name, &inserter_consumer);

    logger.add_timestamp("done");

    // explain if nothing happened
    if (logger.hashes_inserted == 0) {
      describe_none_inserted_dfxml();
    }

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
  }

  // * copy hashdb to new hashdb
  static void do_copy_new(const std::string& hashdb_indir,
                          const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_COPY_NEW);

    // log control parameters
    logger.add("hashdb_indir", hashdb_indir);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the existing hashdb
    hashdb_db_manager_t hashdb_in(hashdb_indir, READ_ONLY);

    logger.add_timestamp("opened source hashdb");

    // open the new hashdb
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_NEW);

    // log the hashdb settings
    logger.add_hashdb_settings(hashdb_out.hashdb_settings);

    logger.add_timestamp("opened new hashdb");

    // establish iterators
    hashdb_db_manager_t::hashdb_iterator_t it = hashdb_in.begin();
    hashdb_db_manager_t::hashdb_iterator_t it_end = hashdb_in.end();

    // copy all elements
    while (it != it_end) {
      hashdb_out.insert_hash_element(*it, logger);
      ++it;
    }

    logger.add_timestamp("done");

    // provide summary
    logger.close();
    history_manager_t::merge_history_to_history(hashdb_indir, hashdb_outdir);
    history_manager_t::append_log_to_history(hashdb_outdir);
  }

  // * copy hashdb to existing hashdb
  static void do_copy(const std::string& hashdb_indir,
                      const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_COPY);

    // log control parameters
    logger.add("hashdb_indir", hashdb_indir);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the existing hashdb
    hashdb_db_manager_t hashdb_in(hashdb_indir, READ_ONLY);

    logger.add_timestamp("opened source hashdb");

    // open the hashdb
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_MODIFY);

    logger.add_timestamp("opened hashdb");

    // establish iterators
    hashdb_db_manager_t::hashdb_iterator_t it = hashdb_in.begin();
    hashdb_db_manager_t::hashdb_iterator_t it_end = hashdb_in.end();

    // copy all elements
    while (it != it_end) {
      hashdb_out.insert_hash_element(*it, logger);
      ++it;
    }

    logger.add_timestamp("done");

    // provide summary
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
    history_manager_t::merge_history_to_history(hashdb_indir, hashdb_outdir);
  }

  // * copy hashdb to new hashdb discarding duplicates
  static void do_copy_new_exclude_duplicates(const std::string& hashdb_indir,
                                             const std::string& hashdb_outdir,
                                             size_t exclude_duplicates_count) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_COPY_EXCLUDE_DUPLICATES);

    // log control parameters
    logger.add("hashdb_indir", hashdb_indir);
    logger.add("hashdb_outdir", hashdb_outdir);
    logger.add("exclude_duplicates_count", exclude_duplicates_count);

    // open the existing hashdb
    hashdb_db_manager_t hashdb_in(hashdb_indir, READ_ONLY);

    logger.add_timestamp("opened source hashdb");

    // open the new hashdb
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_NEW);

    // log the hashdb settings
    logger.add_hashdb_settings(hashdb_out.hashdb_settings);

    logger.add_timestamp("opened new hashdb");

    // establish iterators
    hashdb_db_manager_t::hashdb_iterator_t it = hashdb_in.begin();
    hashdb_db_manager_t::hashdb_iterator_t it_end = hashdb_in.end();

    // copy all elements that do not have duplicates
    // this solution can be reworked for efficiency
    uint64_t hashes_not_inserted_excluded_duplicates = 0;
    uint64_t source_lookup_record;
    while (it != it_end) {
      md5_t md5 = it->first;
      bool has = hashdb_in.has_source_lookup_record(md5, source_lookup_record);
      if (!has) {
        // program error
        assert(0);
      }
      if (source_lookup_encoding::get_count(source_lookup_record)
                                               < exclude_duplicates_count) {
        // keep
        hashdb_out.insert_hash_element(*it, logger);
      } else {
        // throw away duplicate
        ++hashes_not_inserted_excluded_duplicates;
      }
      ++it;
    }

    logger.add_timestamp("done");

    // provide summary
    logger.add("hashes_not_inserted_excluded_duplicates", hashes_not_inserted_excluded_duplicates);
    std::cout << "hashes not inserted, excluded duplicates=" << hashes_not_inserted_excluded_duplicates << "\n";

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
    history_manager_t::merge_history_to_history(hashdb_indir, hashdb_outdir);
  }

  // * remove DFXML hashes from existing hashdb
  static void do_remove_dfxml(const std::string& dfxml_infile,
                              const std::string& repository_name,
                              const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_REMOVE_DFXML);

    // log control parameters
    logger.add("dfxml_infile", dfxml_infile);
    logger.add("repository_name", repository_name);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the hashdb to be modified
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_MODIFY);

    logger.add_timestamp("opened hashdb");

    // create the remover consumer
    remover_consumer_t remover_consumer(hashdb_out, logger);

    // perform the removal using the remover consumer
    dfxml_hashdigest_reader_t<remover_consumer_t>::do_read(
                            dfxml_infile, repository_name, &remover_consumer);

    logger.add_timestamp("done");

    // explain if nothing happened
    if (logger.hashes_removed == 0) {
      describe_none_removed_dfxml();
    }

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
  }

  // * remove hashes in hashdb from existing hashdb
  static void do_remove(const std::string& hashdb_indir,
                        const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_REMOVE);

    // log control parameters
    logger.add("hashdb_indir", hashdb_indir);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open the existing hashdb
    hashdb_db_manager_t hashdb_in(hashdb_indir, READ_ONLY);

    logger.add_timestamp("opened source hashdb");

    // open the hashdb to be modified
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_MODIFY);

    logger.add_timestamp("opened hashdb");

    // establish iterators
    hashdb_db_manager_t::hashdb_iterator_t it = hashdb_in.begin();
    hashdb_db_manager_t::hashdb_iterator_t it_end = hashdb_in.end();

    // copy all elements
    while (it != it_end) {
      hashdb_out.remove_hash_element(*it, logger);
      ++it;
    }

    logger.add_timestamp("done");

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
  }

  // * Merge hashdb indir1 and hashdb indir2 into new hashdb outdir
  static void do_merge(const std::string& hashdb_indir1,
                       const std::string& hashdb_indir2,
                       const std::string& hashdb_outdir) {

    hashdb_change_logger_t logger(hashdb_outdir, COMMAND_MERGE);

    // log control parameters
    logger.add("hashdb_indir1", hashdb_indir1);
    logger.add("hashdb_indir2", hashdb_indir2);
    logger.add("hashdb_outdir", hashdb_outdir);

    // open hashdb 1
    hashdb_db_manager_t hashdb_in1(hashdb_indir1, READ_ONLY);

    logger.add_timestamp("opened hashdb input");

    // open hashdb 2
    hashdb_db_manager_t hashdb_in2(hashdb_indir2, READ_ONLY);

    logger.add_timestamp("opened second hashdb input");

    // create the new hashdb output
    hashdb_db_manager_t hashdb_out(hashdb_outdir, RW_NEW);

    // log the hashdb settings
    logger.add_hashdb_settings(hashdb_out.hashdb_settings);

    logger.add_timestamp("created hashdb output");

    // establish iterators
    hashdb_db_manager_t::hashdb_iterator_t it1 = hashdb_in1.begin();
    hashdb_db_manager_t::hashdb_iterator_t it2 = hashdb_in2.begin();
    hashdb_db_manager_t::hashdb_iterator_t it1_end = hashdb_in1.end();
    hashdb_db_manager_t::hashdb_iterator_t it2_end = hashdb_in2.end();

    // while elements are in both, insert ordered by key
    while ((it1 != it1_end) and (it2 != it2_end)) {
      if (it1->first < it2->first) {
        hashdb_out.insert_hash_element(*it1, logger);
        ++it1;
      } else {
        hashdb_out.insert_hash_element(*it2, logger);
        ++it2;
      }
    }

    // insert all remaining elements
    while (it1 != it1_end) {
      hashdb_out.insert_hash_element(*it1, logger);
      ++it1;
    }
    while (it2 != it2_end) {
      hashdb_out.insert_hash_element(*it2, logger);
      ++it2;
    }

    logger.add_timestamp("done");

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_outdir);
    history_manager_t::merge_history_to_history(hashdb_indir1, hashdb_outdir);
    history_manager_t::merge_history_to_history(hashdb_indir2, hashdb_outdir);
  }

  // * Rebuild the Bloom filters
  static void do_rebuild_bloom(const std::string& hashdb_indir) {

    hashdb_change_logger_t logger(hashdb_indir, COMMAND_REBUILD_BLOOM);

    // log control parameters
    logger.add("hashdb_indir", hashdb_indir);

    // get hashdb tuning settings
    settings_t hashdb_settings;
    hashdb_settings_reader_t::read_settings(hashdb_indir+"settings.xml", hashdb_settings);

    // report the settings
    logger.add_hashdb_settings(hashdb_settings);

    logger.add_timestamp("opened hashdb settings");

    // calculate the bloom filter filenames
    std::string bloom1_path =
             hashdb_filenames_t::bloom1_filename(hashdb_indir);
    std::string bloom2_path =
             hashdb_filenames_t::bloom2_filename(hashdb_indir);

    // open the new bloom filters
    bloom_filter_t bloom1(bloom1_path,
                          RW_NEW,
                          hashdb_settings.bloom1_is_used,
                          hashdb_settings.bloom1_k_hash_functions,
                          hashdb_settings.bloom1_M_hash_size);
    bloom_filter_t bloom2(bloom2_path,
                          RW_NEW,
                          hashdb_settings.bloom2_is_used,
                          hashdb_settings.bloom2_k_hash_functions,
                          hashdb_settings.bloom2_M_hash_size);

    // only do this if there is a Bloom filter to work on
    if (bloom1.is_used || bloom2.is_used) {

      // open just the hash store reading
      std::string hash_store_path =
                 hashdb_filenames_t::hash_store_filename(hashdb_indir);
      hash_store_t hash_store(
                 hash_store_path,
                 READ_ONLY,
                 hashdb_settings.map_type,
                 hashdb_settings.map_shard_count);

      logger.add_timestamp("opened hash store input");

      // iterate over all hashes
      hash_store_t::hash_store_iterator_t it = hash_store.begin();
      hash_store_t::hash_store_iterator_t end = hash_store.end();
      while (it != end) {

        // get element's md5
        md5_t md5 = it->first;

        // add hash to filters
        if (bloom1.is_used) {
          bloom1.add_hash_value(md5);
        }
        if (bloom2.is_used) {
          bloom2.add_hash_value(md5);
        }

        ++it;
      }
    }

    logger.add_timestamp("done");

    // provide summary
    logger.add_hashdb_settings(hashdb_settings);

    // append log to history
    logger.close();
    history_manager_t::append_log_to_history(hashdb_indir);
  }
 
  // * export hashdb to dfxml
  static void do_export(const std::string& hashdb_infile,
                        const std::string& dfxml_outfile) {

    // open the hashdb to be exported
    hashdb_db_manager_t hashdb_in(hashdb_infile, READ_ONLY);

    // create the hashdb exporter object
    hashdb_exporter_t exporter(dfxml_outfile);

    // perform the export
    exporter.do_export(hashdb_in);
  }

  // * information about hashdb including settings and usage statistics
  static void do_info(const std::string& hashdb_indir) {

    // get info
    std::string info;
    int status = hashdb_db_info_provider_t::get_hashdb_info(hashdb_indir, info);
    if (status != 0) {
      std::cerr << "Error in info command.\n";
      exit (-1);
    }

    // report info to stdout
    std::cout << info;
  }

  // * provide server service
  static void do_server(const std::string& hashdb_indir,
                        const std::string& socket_endpoint) {

    // open the hashdb input
    hashdb_db_manager_t hashdb_in(hashdb_indir, READ_ONLY);

    // create server object
    query_by_socket_server_t server(&hashdb_in, socket_endpoint);

    while (std::cin) {
      // process hash sets indefinitely
      int status = server.process_request();
      if (status != 0) {
        std::cerr << "Error in server service while processing request.  Request lost.\n";
      }
    }
  }

bool commands_t::is_new = false;
dfxml_writer* commands_t::x = 0;

const std::string commands_t::COMMAND_COPY_NEW_DFXML("copy_new_dfxml");
const std::string commands_t::COMMAND_COPY_NEW("copy_new");
const std::string commands_t::COMMAND_COPY_DFXML("copy_dfxml");
const std::string commands_t::COMMAND_COPY("copy");
const std::string commands_t::COMMAND_COPY_EXCLUDE_DUPLICATES("copy_exclude_duplicates");
const std::string commands_t::COMMAND_REMOVE_DFXML("remove_dfxml");
const std::string commands_t::COMMAND_REMOVE("remove");
const std::string commands_t::COMMAND_MERGE("merge");
const std::string commands_t::COMMAND_REBUILD_BLOOM("rebuild_bloom");
*/

};

#endif

