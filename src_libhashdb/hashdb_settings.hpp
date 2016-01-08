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
 * This file manages the hashdb settings.
 */

#ifndef    HASHDB_SETTINGS_HPP
#define    HASHDB_SETTINGS_HPP

#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hashdb tuning options
struct hashdb_settings_t {

  static const uint32_t expected_data_store_version = 3;
  uint32_t data_store_version;
  uint32_t sector_size;
  uint32_t block_size;
  // bloom
  bool     bloom_is_used;
  uint32_t bloom_M_hash_size;      // size of the bloom filter hash, in bits
  uint32_t bloom_k_hash_functions; // number of hash filter functions

  hashdb_settings_t() :
        data_store_version(0),
        sector_size(0),
        block_size(0) {
  }

  hashdb_settings_t(uint32_t p_data_store_version,
                    uint32_t p_sector_size,
                    uint32_t p_block_size) :
        data_store_version(p_data_store_version),
        sector_size(p_sector_size),
        block_size(p_block_size) {
  }

  void report_settings(std::ostream& os) const {
    os << "hashdb settings:\n";
    os << "data store version: " << data_store_version << "\n";
    os << "sector size: " << sector_size << "\n";
    os << "hash block size: " << block_size << "\n";
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const struct hashdb_settings_t& settings) {
  settings.report_settings(os);
  return os;
}

#endif

