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
  // hash_data
  size_t hash_data_inserted;
  size_t hash_data_metadata_different;
  size_t hash_data_not_inserted_duplicate_source;
  size_t hash_data_not_inserted_invalid_file_offset;
  size_t hash_data_not_inserted_max_id_offset_pairs;

  // hash
  size_t hash_inserted;
  size_t hash_not_inserted;

  // source_data
  size_t source_data_inserted;
  size_t source_data_different;
  size_t source_data_not_inserted;

  // source_id
  size_t source_id_inserted;
  size_t source_id_not_inserted;

  // source_name
  size_t source_name_inserted;
  size_t source_name_not_inserted;

  lmdb_changes_t() :
            hash_data_inserted(0),
            hash_data_metadata_different(0),
            hash_data_not_inserted_duplicate_source(0),
            hash_data_not_inserted_invalid_file_offset(0),
            hash_data_not_inserted_max_id_offset_pairs(0),
            hash_inserted(0),
            hash_not_inserted(0),
            source_data_inserted(0),
            source_data_different(0),
            source_data_not_inserted(0),
            source_id_inserted(0),
            source_id_not_inserted(0),
            source_name_inserted(0),
            source_name_not_inserted(0) {
  }

  void report_changes(std::ostream& os) const {

    os << "hashdb changes:\n";
    // log changes
    if (hash_data_inserted) {
      os << "    hash_data_inserted: " << hash_data_inserted << "\n";
    }
    if (hash_data_metadata_different) {
      os << "    hash_data_metadata_different: "
         << hash_data_metadata_different << "\n";
    }
    if (hash_data_not_inserted_duplicate_source) {
      os << "    hash_data_not_inserted_duplicate_source: "
         << hash_data_not_inserted_duplicate_source<< "\n";
    }
    if (hash_data_not_inserted_invalid_file_offset) {
      os << "    hash_data_not_inserted_invalid_file_offset: "
         << hash_data_not_inserted_invalid_file_offset<< "\n";
    }
    if (hash_data_not_inserted_max_id_offset_pairs) {
      os << "    hash_data_not_inserted_max_id_offset_pairs: "
         << hash_data_not_inserted_max_id_offset_pairs<< "\n";
    }
    if (hash_inserted) {
      os << "    hash_inserted: " << hash_inserted<< "\n";
    }
    if (hash_not_inserted) {
      os << "    hash_not_inserted: " << hash_not_inserted<< "\n";
    }
    if (source_data_inserted) {
      os << "    source_data_inserted: " << source_data_inserted << "\n";
    }
    if (source_data_different) {
      os << "    source_data_different: " << source_data_different<< "\n";
    }
    if (source_data_not_inserted) {
      os << "    source_data_not_inserted: " << source_data_not_inserted << "\n";
    }
    if (source_id_inserted) {
      os << "    source_id_inserted: " << source_id_inserted << "\n";
    }
    if (source_id_not_inserted) {
      os << "    source_id_not_inserted: " << source_id_not_inserted << "\n";
    }
    if (source_name_inserted) {
      os << "    source_name_inserted: " << source_name_inserted << "\n";
    }
    if (source_name_not_inserted) {
      os << "    source_name_not_inserted: " << source_name_not_inserted << "\n";
    }
    if (hash_data_inserted == 0 &&
        hash_data_metadata_different == 0 &&
        hash_data_not_inserted_duplicate_source == 0 &&
        hash_data_not_inserted_invalid_file_offset == 0 &&
        hash_data_not_inserted_max_id_offset_pairs == 0 &&
        hash_inserted == 0 &&
        hash_not_inserted == 0 &&
        source_data_inserted == 0 &&
        source_data_different == 0 &&
        source_data_not_inserted == 0 &&
        source_id_inserted == 0 &&
        source_id_not_inserted == 0 &&
        source_name_inserted == 0 &&
        source_name_not_inserted == 0) {
       os << "No changes.\n";
    }
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class lmdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif

