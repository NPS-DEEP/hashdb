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
#include <dfxml_writer.h>

/**
 * The hashdb change logger holds all possible change values,
 * and is used for reporting changes to the database.
 */
class hashdb_changes_t {

  public:
  uint32_t hashes_inserted;
  uint32_t hashes_not_inserted_mismatched_hash_block_size;
  uint32_t hashes_not_inserted_invalid_sector_size;
  uint32_t hashes_not_inserted_duplicate_element;
  uint32_t hashes_not_inserted_skip_low_entropy;
  uint32_t hashes_not_inserted_skip_whitelist;

  hashdb_changes_t() :

                     hashes_inserted(0),
                     hashes_not_inserted_mismatched_hash_block_size(0),
                     hashes_not_inserted_invalid_sector_size(0),
                     hashes_not_inserted_duplicate_element(0),
                     hashes_not_inserted_skip_low_entropy(0),
                     hashes_not_inserted_skip_whitelist(0) {
  }

  void report_changes(dfxml_writer& x) const {

    // log any insert changes to x
    x.push("hashdb_changes");

    if (hashes_inserted)
      x.xmlout("hashes_inserted", hashes_inserted);
    if (hashes_not_inserted_mismatched_hash_block_size)
      x.xmlout("hashes_not_inserted_mismatched_hash_block_size", hashes_not_inserted_mismatched_hash_block_size);
    if (hashes_not_inserted_invalid_sector_size)
      x.xmlout("hashes_not_inserted_invalid_sector_size", hashes_not_inserted_invalid_sector_size);
    if (hashes_not_inserted_duplicate_element)
      x.xmlout("hashes_not_inserted_duplicate_element", hashes_not_inserted_duplicate_element);
    if (hashes_not_inserted_skip_low_entropy)
      x.xmlout("hashes_not_inserted_skip_low_entropy", hashes_not_inserted_skip_low_entropy);
    if (hashes_not_inserted_skip_whitelist)
      x.xmlout("hashes_not_inserted_skip_whitelist", hashes_not_inserted_skip_whitelist);

    x.pop();
  }

  void report_changes(std::ostream& os) const {

    // log any insert changes to stdout
    std::cout << "hashdb changes (insert):\n"
              << "    hashes inserted: " << hashes_inserted << "\n"
                << "    hashes not inserted (mismatched hash block size): " << hashes_not_inserted_mismatched_hash_block_size << "\n"
              << "    hashes not inserted (invalid sector size): " << hashes_not_inserted_invalid_sector_size << "\n"
              << "    hashes not inserted (duplicate element): " << hashes_not_inserted_duplicate_element << "\n"
              << "    hashes not inserted (skip low entropy): " << hashes_not_inserted_skip_low_entropy<< "\n"
              << "    hashes not inserted (skip whitelist): " << hashes_not_inserted_skip_whitelist<< "\n";
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb_changes_t& changes) {
  changes.report_changes(os);
  return os;
}

#endif

