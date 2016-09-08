// Author:  Bruce Allen
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

namespace hashdb {

/**
 * Holds state about changes applied to the hash database.
 */
class lmdb_changes_t {

  public:
  // hash_data
  size_t hash_data_source_inserted;
  size_t hash_data_offset_inserted;
  size_t hash_data_data_changed;
  size_t hash_data_duplicate_offset_detected;
  size_t hash_data_mismatched_sub_count_detected;

  // hash
  size_t hash_prefix_inserted;
  size_t hash_suffix_inserted;
  size_t hash_count_changed;
  size_t hash_not_changed;

  // source_data
  size_t source_data_inserted;
  size_t source_data_changed;
  size_t source_data_same;

  // source_id
  size_t source_id_inserted;
  size_t source_id_already_present;

  // source_name
  size_t source_name_inserted;
  size_t source_name_already_present;

  lmdb_changes_t() :
            hash_data_source_inserted(0),
            hash_data_offset_inserted(0),
            hash_data_data_changed(0),
            hash_data_duplicate_offset_detected(0),
            hash_data_mismatched_sub_count_detected(0),
            hash_prefix_inserted(0),
            hash_suffix_inserted(0),
            hash_count_changed(0),
            hash_not_changed(0),
            source_data_inserted(0),
            source_data_changed(0),
            source_data_same(0),
            source_id_inserted(0),
            source_id_already_present(0),
            source_name_inserted(0),
            source_name_already_present(0) {
  }

  void report_changes(std::ostream& os) const {

    os << "# hashdb changes:\n";
    // log changes
    if (hash_data_source_inserted) {
      os << "#     hash_data_source_inserted: "
         << hash_data_source_inserted<< "\n";
    }
    if (hash_data_offset_inserted) {
      os << "#     hash_data_offset_inserted: "
         << hash_data_offset_inserted<< "\n";
    }
    if (hash_data_data_changed) {
      os << "#     hash_data_data_changed: "
         << hash_data_data_changed << "\n";
    }
    if (hash_data_duplicate_offset_detected) {
      os << "#     hash_data_duplicate_offset_detected: "
         << hash_data_duplicate_offset_detected << "\n";
    }
    if (hash_data_mismatched_sub_count_detected) {
      os << "#     hash_data_mismatched_sub_count_detected: "
         << hash_data_mismatched_sub_count_detected << "\n";
    }
    if (hash_prefix_inserted) {
      os << "#     hash_prefix_inserted: " << hash_prefix_inserted<< "\n";
    }
    if (hash_suffix_inserted) {
      os << "#     hash_suffix_inserted: " << hash_suffix_inserted<< "\n";
    }
    if (hash_count_changed) {
      os << "#     hash_count_changed: " << hash_count_changed<< "\n";
    }
    if (hash_not_changed) {
      os << "#     hash_not_changed: " << hash_not_changed<< "\n";
    }
    if (source_data_inserted) {
      os << "#     source_data_inserted: " << source_data_inserted << "\n";
    }
    if (source_data_changed) {
      os << "#     source_data_changed: " << source_data_changed<< "\n";
    }
    if (source_data_same) {
      os << "#     source_data_same: " << source_data_same << "\n";
    }
    if (source_id_inserted) {
      os << "#     source_id_inserted: " << source_id_inserted << "\n";
    }
    if (source_id_already_present) {
      os << "#     source_id_already_present: " << source_id_already_present << "\n";
    }
    if (source_name_inserted) {
      os << "#     source_name_inserted: " << source_name_inserted << "\n";
    }
    if (source_name_already_present) {
      os << "#     source_name_already_present: " << source_name_already_present << "\n";
    }
    if (hash_data_source_inserted == 0 &&
        hash_data_offset_inserted == 0 &&
        hash_data_data_changed == 0 &&
        hash_data_duplicate_offset_detected== 0 &&
        hash_data_mismatched_sub_count_detected== 0 &&
        hash_prefix_inserted == 0 &&
        hash_suffix_inserted == 0 &&
        hash_count_changed == 0 &&
        hash_not_changed == 0 &&
        source_data_inserted == 0 &&
        source_data_changed == 0 &&
        source_data_same == 0 &&
        source_id_inserted == 0 &&
        source_id_already_present == 0 &&
        source_name_inserted == 0 &&
        source_name_already_present == 0) {
       os << "No changes.\n";
    }
  }
};

} // end namespace hashdb

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb::lmdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif

