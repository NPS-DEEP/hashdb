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

#ifndef LMDB_CHANGES_HPP
#define LMDB_CHANGES_HPP

#include <sstream>
#include <iostream>

/**
 * Holds state about changes applied to the hash database.
 */
class lmdb_changes_t {

  public:
  size_t source_file_hash;
  size_t source_file_hash_false;
  size_t source_name;
  size_t source_name_false;
  size_t source_data;
  size_t source_data_false;
  size_t source_data_different;
  size_t hash;
  size_t hash_false;
  size_t hash_data;
  size_t hash_data_false;
  size_t hash_data_different;
  size_t hash_source;
  size_t hash_source_false;
  size_t hash_source_max;

  lmdb_changes_t() :
           source_file_hash(0),
           source_file_hash_false(0),
           source_name(0),
           source_name_false(0),
           source_data(0),
           source_data_false(0),
           source_data_different(0),
           hash(0),
           hash_false(0),
           hash_data(0),
           hash_data_false(0),
           hash_data_different(0),
           hash_source(0),
           hash_source_false(0),
           hash_source_max(0) {
  }

  void report_changes(std::ostream& os) const {

    // log any insert changes to stdout
    std::cout << "source_file_hash: " << source_file_hash << "\n"
              << "source_file_hash_false: " << source_file_hash_false << "\n"
              << "source_name: " << source_name << "\n"
              << "source_name_false: " << source_name_false << "\n"
              << "source_data: " << source_data << "\n"
              << "source_data_false: " << source_data_false << "\n"
              << "source_data_different: " << source_data_different << "\n"
              << "hash: " << hash << "\n"
              << "hash_false: " << hash_false << "\n"
              << "hash_data: " << hash_data << "\n"
              << "hash_data_false: " << hash_data_false << "\n"
              << "hash_data_different: " << hash_data_different << "\n"
              << "hash_source: " << hash_source << "\n"
              << "hash_source_false: " << hash_source_false << "\n"
              << "hash_source_max: " << hash_source_max << "\n"
              ;
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class lmdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif

