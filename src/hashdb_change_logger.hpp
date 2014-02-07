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
 * The hashdb manager provides access to the hashdb.
 */

#ifndef HASHDB_CHANGE_LOGGER_HPP
#define HASHDB_CHANGE_LOGGER_HPP

#include "bloom_filter.hpp"
#include "command_line.hpp"
#include <sstream>
#include <iostream>

/**
 * The hashdb change logger holds all possible change values,
 * and is used for reporting changes to the database.
 */
class hashdb_change_logger_t {
  private:
  const std::string hashdb_dir;
  const std::string title;
  dfxml_writer x;
  bool closed;

  // don't allow copy
  hashdb_change_logger_t(const hashdb_change_logger_t& changes);

  public:
  uint32_t hashes_inserted;
  uint32_t hashes_not_inserted_invalid_file_offset;
  uint32_t hashes_not_inserted_wrong_hash_block_size;
  uint32_t hashes_not_inserted_wrong_hashdigest_type;
  uint32_t hashes_not_inserted_exceeds_max_duplicates;
  uint32_t hashes_not_inserted_duplicate_source;

  uint32_t hashes_removed;
  uint32_t hashes_not_removed_invalid_file_offset;
  uint32_t hashes_not_removed_wrong_hash_block_size;
  uint32_t hashes_not_removed_wrong_hashdigest_type;
  uint32_t hashes_not_removed_no_hash;
  uint32_t hashes_not_removed_different_source;

  hashdb_change_logger_t(const std::string p_hashdb_dir,
                         const std::string p_title) :
                     hashdb_dir(p_hashdb_dir),
                     title(p_title),
                     // open the dfxml writer
                     x(hashdb_filenames_t::log_filename(hashdb_dir), false),
                     closed(false),

                     hashes_inserted(0),
                     hashes_not_inserted_invalid_file_offset(0),
                     hashes_not_inserted_wrong_hash_block_size(0),
                     hashes_not_inserted_wrong_hashdigest_type(0),
                     hashes_not_inserted_exceeds_max_duplicates(0),
                     hashes_not_inserted_duplicate_source(0),

                     hashes_removed(0),
                     hashes_not_removed_invalid_file_offset(0),
                     hashes_not_removed_wrong_hash_block_size(0),
                     hashes_not_removed_wrong_hashdigest_type(0),
                     hashes_not_removed_no_hash(0),
                     hashes_not_removed_different_source(0) {

    // log the preamble
    x.push("log");

    std::stringstream ss;
    ss << "command_type='" << title << "'";
    x.push("command", ss.str());
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line_t::command_line_string);
  }

  ~hashdb_change_logger_t() {
    if (!closed) {
      close();
    }
  }

  /**
   * You can close the logger and use the log before the logger is disposed
   * by calling close.
   * Do not use the logger after closing it; this will corrupt the log file.
   */
  void close() {
    if (closed) {
      // already closed
      std::cout << "hashdb_change_logger.close warning: already closed\n";
      return;
    }

    // log any insert changes to x
    x.push("hashdb_changes");

    if (hashes_inserted)
      x.xmlout("hashes_inserted", hashes_inserted);
    if (hashes_not_inserted_invalid_file_offset)
      x.xmlout("hashes_not_inserted_invalid_file_offset", hashes_not_inserted_invalid_file_offset);
    if (hashes_not_inserted_wrong_hash_block_size)
      x.xmlout("hashes_not_inserted_wrong_hash_block_size", hashes_not_inserted_wrong_hash_block_size);
    if (hashes_not_inserted_wrong_hashdigest_type)
      x.xmlout("hashes_not_inserted_wrong_hashdigest_type", hashes_not_inserted_wrong_hashdigest_type);
    if (hashes_not_inserted_exceeds_max_duplicates)
      x.xmlout("hashes_not_inserted_exceeds_max_duplicates", hashes_not_inserted_exceeds_max_duplicates);
    if (hashes_not_inserted_duplicate_source)
      x.xmlout("hashes_not_inserted_duplicate_source", hashes_not_inserted_duplicate_source);

    // log any remove changes to x
    if (hashes_removed)
      x.xmlout("hashes_removed", hashes_removed);
    if (hashes_not_removed_invalid_file_offset)
      x.xmlout("hashes_not_removed_invalid_file_offset", hashes_not_removed_invalid_file_offset);
    if (hashes_not_removed_wrong_hash_block_size)
      x.xmlout("hashes_not_removed_wrong_hash_block_size", hashes_not_removed_wrong_hash_block_size);
    if (hashes_not_removed_wrong_hashdigest_type)
      x.xmlout("hashes_not_removed_wrong_hashdigest_type", hashes_not_removed_wrong_hashdigest_type);
    if (hashes_not_removed_no_hash)
      x.xmlout("hashes_not_removed_no_hash", hashes_not_removed_no_hash);
    if (hashes_not_removed_different_source)
      x.xmlout("hashes_not_removed_different_source", hashes_not_removed_different_source);

    x.pop();

    // log closure for x
    x.add_rusage();
    x.pop(); // command
    x.pop(); // log

    // log any insert changes to stdout
    std::cout << "hashdb changes:\n";
    if (hashes_inserted)
     std::cout << "    hashes inserted=" << hashes_inserted << "\n";
    if (hashes_not_inserted_invalid_file_offset)
     std::cout << "    hashes not inserted, invalid file offset=" << hashes_not_inserted_invalid_file_offset << "\n";
    if (hashes_not_inserted_wrong_hash_block_size)
     std::cout << "    hashes not inserted, wrong hash block size=" << hashes_not_inserted_wrong_hash_block_size << "\n";
    if (hashes_not_inserted_wrong_hashdigest_type)
     std::cout << "    hashes not inserted, wrong hashdigest type=" << hashes_not_inserted_wrong_hashdigest_type << "\n";
    if (hashes_not_inserted_exceeds_max_duplicates)
     std::cout << "    hashes not inserted, exceeds max duplicates=" << hashes_not_inserted_exceeds_max_duplicates << "\n";
    if (hashes_not_inserted_duplicate_source)
     std::cout << "    hashes not inserted, duplicate source=" << hashes_not_inserted_duplicate_source << "\n";

    // log any remove changes to stdout
      std::cout << "hashdb changes:\n";
    if (hashes_removed)
      std::cout << "    hashes removed=" << hashes_removed << "\n";
    if (hashes_not_removed_invalid_file_offset)
      std::cout << "    hashes not removed, invalid file offset=" << hashes_not_removed_invalid_file_offset << "\n";
    if (hashes_not_removed_wrong_hash_block_size)
      std::cout << "    hashes not removed, wrong hash block size=" << hashes_not_removed_wrong_hash_block_size << "\n";
    if (hashes_not_removed_wrong_hashdigest_type)
      std::cout << "    hashes not removed, wrong hashdigest type=" << hashes_not_removed_wrong_hashdigest_type << "\n";
    if (hashes_not_removed_no_hash)
      std::cout << "    hashes not removed, no hash=" << hashes_not_removed_no_hash << "\n";
    if (hashes_not_removed_different_source)
      std::cout << "    hashes not removed, different source=" << hashes_not_removed_different_source << "\n";
    ;

    // mark this logger as closed
    x.flush();
    closed = true;
  }

  /**
   * Emit a named timestamp.
   */
  void add_timestamp(const std::string& name) {
    if (closed) {
      // already closed
      std::cout << "hashdb_change_logger.add_timestamp warning: already closed\n";
      return;
    }

    x.add_timestamp(name);
  }

  /**
   * Add hashdb_settings to the log.
   */
  void add_hashdb_settings(const settings_t& settings) {
    if (closed) {
      // already closed
      std::cout << "hashdb_change_logger.add_hashdb_settings warning: already closed\n";
      return;
    }

    settings.report_settings(x);
  }

  /**
   * Add hashdb_db_manager state to the log.
   */
/*
  void add_hashdb_db_manager_state(const hashdb_db_manager_t manager) {
    manager.report_status(x);

    // zz maybe also to stdout
  }
*/
  void add_hashdb_db_manager_state() {
    if (closed) {
      // already closed
      std::cout << "hashdb_change_logger.add_hashdb_db_manager_state warning: already closed\n";
      return;
    }

    x.xmlout("state", "TBD");
  }

  /**
   * Add a tag, value pair for any type supported by xmlout.
   */
  template<typename T>
  void add(const std::string& tag, const T& value) {
    if (closed) {
      // already closed
      std::cout << "hashdb_change_logger.add warning: already closed\n";
      return;
    }

    x.xmlout(tag, value);
  }
};

#endif

