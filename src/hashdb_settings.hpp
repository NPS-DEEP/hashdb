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

#include <dfxml_writer.h>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include "globals.hpp"

inline bool string_to_bloom_state(std::string state_string, bool& state) {
  if (state_string == "enabled")  { state = true;  return true; }
  if (state_string == "disabled") { state = false; return true; }
  state = false;
  return false;
}

inline std::string bloom_state_to_string(bool state) {
  return ((state==true) ? "enabled" : "disabled");
}

// hashdb tuning options
struct hashdb_settings_t {

  uint32_t settings_version;
  uint32_t byte_alignment;
  uint32_t hash_truncation;
  uint32_t hash_block_size;
  uint32_t maximum_hash_duplicates;
  // bloom
  bool     bloom_is_used;
  uint32_t bloom_M_hash_size;      // size of the bloom filter hash, in bits
  uint32_t bloom_k_hash_functions; // number of hash filter functions

  // note: POD, so permit default copy and equals

  hashdb_settings_t() :
        settings_version(globals_t::hashdb_settings_version),
        byte_alignment(globals_t::default_byte_alignment),
        hash_truncation(globals_t::default_hash_truncation),
        hash_block_size(globals_t::default_hash_block_size),
        maximum_hash_duplicates(globals_t::default_maximum_hash_duplicates),
        bloom_is_used(globals_t::default_bloom_is_used),
        bloom_M_hash_size(globals_t::default_bloom_M_hash_size),
        bloom_k_hash_functions(globals_t::default_bloom_k_hash_functions) {
  }

  void report_settings(std::ostream& os) const {
    os << "hashdb settings:\n";
    os << "settings version: " << settings_version << "\n";
    os << "byte alignment: " << byte_alignment << "\n";
    os << "hash truncation: " << hash_truncation << "\n";
    os << "hash block size: " << hash_block_size << "\n";
    os << "maximum hash duplicates: " << maximum_hash_duplicates << "\n";
    os << "bloom used: " << bloom_state_to_string(bloom_is_used) << "\n";
    os << "bloom k hash functions: " << bloom_k_hash_functions << "\n";
    os << "bloom M hash size: " << bloom_M_hash_size << "\n";
  }

  void report_settings(dfxml_writer& x) const {
    x.xmlout("settings_version", settings_version);
    x.xmlout("byte_alignment", byte_alignment);
    x.xmlout("hash_truncation", hash_truncation);
    x.xmlout("hash_block_size", hash_block_size);
    x.xmlout("maximum_hash_duplicates", (uint64_t)maximum_hash_duplicates);

    x.xmlout("bloom_used", bloom_state_to_string(bloom_is_used));
    x.xmlout("bloom_k_hash_functions", (uint64_t)bloom_k_hash_functions);
    x.xmlout("bloom_M_hash_size", (uint64_t)bloom_M_hash_size);
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const struct hashdb_settings_t& settings) {
  settings.report_settings(os);
  return os;
}

#endif

