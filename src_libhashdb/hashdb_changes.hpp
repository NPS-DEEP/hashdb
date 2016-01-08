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
 * Holds state about changes applied to the hash database.
 * The hashdb_manager updates this information while performing actions,
 * then reports it using the logger.
 */

#ifndef HASHDB_CHANGES_HPP
#define HASHDB_CHANGES_HPP

#include <sstream>
#include <iostream>

/**
 * The hashdb change logger holds all possible change values,
 * and is used for reporting changes to the database.
 */
class hashdb_changes_t {

  public:
  uint32_t file_binary_hash;
  uint32_t file_binary_hash_false;
  uint32_t source_name;
  uint32_t source_name_false;
  uint32_t source_data;
  uint32_t source_data_false;
  uint32_t hash;
  uint32_t hash_false;
  uint32_t hash_data;
  uint32_t hash_data_false;
  uint32_t hash_source;
  uint32_t hash_source_false;

  hashdb_changes_t() :
           file_binary_hash(0),
           file_binary_hash_false(0),
           source_name(0),
           source_name_false(0),
           source_data(0),
           source_data_false(0),
           hash(0),
           hash_false(0),
           hash_data(0),
           hash_data_false(0),
           hash_source(0),
           hash_source_false(0) {
  }

  void report_changes(std::ostream& os) const {

    // log any insert changes to stdout
    std::cout << "file_binary_hash: " << file_binary_hash << "\n"
              << "file_binary_hash_false: " << file_binary_hash_false << "\n"
              << "source_name: " << source_name << "\n"
              << "source_name_false: " << source_name_false << "\n"
              << "source_data: " << source_data << "\n"
              << "source_data_false: " << source_data_false << "\n"
              << "hash: " << hash << "\n"
              << "hash_false: " << hash_false << "\n"
              << "hash_data: " << hash_data << "\n"
              << "hash_data_false: " << hash_data_false << "\n"
              << "hash_source: " << hash_source << "\n"
              << "hash_source_false: " << hash_source_false << "\n"
              ;
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif

