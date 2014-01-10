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

#ifndef    hashdb_settings_hpp
#define    hashdb_settings_hpp

#include "hashdb_types.h"
#include "dfxml/src/dfxml_writer.h"
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hashdb tuning options
struct hashdb_settings_t {

  uint32_t hashdb_version;
  uint32_t hash_block_size;
  hashdigest_type_t hashdigest_type;
  uint32_t maximum_hash_duplicates;
  uint8_t number_of_index_bits;
  map_type_t map_type;
  uint32_t map_shard_count;
  multimap_type_t multimap_type;
  uint32_t multimap_shard_count;
  // bloom 1 and 2
  bool     bloom1_is_used;
  uint32_t bloom1_M_hash_size;      // size of the bloom filter hash, in bits
  uint32_t bloom1_k_hash_functions; // number of hash filter functions
  bool     bloom2_is_used;
  uint32_t bloom2_M_hash_size;      // size of the bloom filter hash, in bits
  uint32_t bloom2_k_hash_functions; // number of hash filter functions

  // note: POD, so permit default copy and equals

  hashdb_settings_t() :
        hashdb_version(2),
        hash_block_size(4096),
        hashdigest_type(HASHDIGEST_MD5),
        maximum_hash_duplicates(0),
        number_of_index_bits(32),
        map_type(MAP_BTREE),
        map_shard_count(1),
        multimap_type(MULTIMAP_BTREE),
        multimap_shard_count(1),
        bloom1_is_used(true),
        bloom1_M_hash_size(3),
        bloom1_k_hash_functions(28),
        bloom2_is_used(false),
        bloom2_M_hash_size(3),
        bloom2_k_hash_functions(28) {
  }

  void report_settings(std::ostream& os) const {
    os << "hashdb settings: ";
    os << "hashdb version=" << hashdb_version << ", ";
    os << "hash block size=" << hash_block_size << ", ";
    os << "hashdigest type=" << hashdigest_type_to_string(hashdigest_type) << ", ";
    os << "maximum hash duplicates=" << maximum_hash_duplicates << ", ";

    os << "number of index bits type=" << (uint32_t)number_of_index_bits << "\n";
    os << "map type=" << map_type_to_string(map_type) << ", ";
    os << "map shard count=" << map_shard_count << "\n";
    os << "multimap type=" << multimap_type_to_string(multimap_type) << ", ";
    os << "multimap_shard count=" << multimap_shard_count << "\n";

    os << "bloom 1 used=" << bloom_state_to_string(bloom1_is_used);
    os << ", bloom 1 k hash functions=" << bloom1_k_hash_functions;
    os << ", bloom 1 M hash size=" << bloom1_M_hash_size << "\n";
 
    os << "bloom 2 used=" << bloom_state_to_string(bloom2_is_used);
    os << ", bloom 2 k hash functions=" << bloom2_k_hash_functions;
    os << ", bloom 2 M hash size=" << bloom2_M_hash_size << "\n";
  }

  void report_settings(dfxml_writer& x) const {
    x.xmlout("hashdb_version", hashdb_version);
    x.xmlout("hashdigest_type", hashdigest_type_to_string(hashdigest_type));
    x.xmlout("hash_block_size", hash_block_size);
    x.xmlout("maximum_hash_duplicates", (uint64_t)maximum_hash_duplicates);
    x.xmlout("number_of_index_bits", (uint32_t)number_of_index_bits);
    x.xmlout("map_type", map_type_to_string(map_type));
    x.xmlout("map_shard_count", map_shard_count);
    x.xmlout("multimap_type", multimap_type_to_string(multimap_type));
    x.xmlout("multimap_shard_count", multimap_shard_count);

    x.xmlout("bloom1_used", bloom_state_to_string(bloom1_is_used));
    x.xmlout("bloom1_k_hash_functions", (uint64_t)bloom1_k_hash_functions);
    x.xmlout("bloom1_M_hash_size", (uint64_t)bloom1_M_hash_size);
 
    x.xmlout("bloom2_used", bloom_state_to_string(bloom2_is_used));
    x.xmlout("bloom2_k_hash_functions", (uint64_t)bloom2_k_hash_functions);
    x.xmlout("bloom2_M_hash_size", (uint64_t)bloom2_M_hash_size);
  }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb_settings_t& settings) {
  settings.report_settings(os);
  return os;
}

#endif

