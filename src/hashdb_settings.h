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
 * This file defines the data structures for managing hashdb settings.
 */

// data heirarchy:
//   hashdb_settings_t
//     hash_store_settings_t
//     hash_duplicates_store_settings_t
//     source_lookup_settings_t
//     bloom_settings_t
//

#ifndef    hashdb_settings_h
#define    hashdb_settings_h

#include "hashdb_types.h"
#include "source_lookup_record.h"
#include "dfxml/src/dfxml_writer.h"
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hash store
struct hash_store_settings_t {
    map_type_t map_type;
    uint32_t shard_count;

    hash_store_settings_t(const hash_store_settings_t& settings) :
        map_type(settings.map_type),
        shard_count(settings.shard_count) {
    }

    hash_store_settings_t() :
//        map_type(MAP_RED_BLACK_TREE),
        map_type(MAP_BTREE),
        shard_count(1) {
    }

    void report_settings(std::ostream& os) const {
      os << "hash store settings: ";
      os << "map type=" << map_type_to_string(map_type);
      os << ", shard count=" << shard_count << "\n";
    }

    void report_settings(dfxml_writer& x) const {
      x.push("hash_store_settings");
      x.xmlout("map_type", map_type_to_string(map_type));
      x.xmlout("shard_count", shard_count);
      x.pop();
    }

    hash_store_settings_t& operator=(const hash_store_settings_t& other) {
      map_type = other.map_type;
      shard_count = other.shard_count;
      return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hash_store_settings_t& settings) {
  os << "(map_type=" << settings.map_type
     << ",shard_count=" << settings.shard_count << ")";
  return os;
}

// hash duplicates store
struct hash_duplicates_store_settings_t {
    multimap_type_t multimap_type;
    uint32_t shard_count;

    hash_duplicates_store_settings_t(const hash_duplicates_store_settings_t& settings) :
        multimap_type(settings.multimap_type),
        shard_count(settings.shard_count) {
    }

    hash_duplicates_store_settings_t() :
//        multimap_type(MULTIMAP_RED_BLACK_TREE),
        multimap_type(MULTIMAP_BTREE),
        shard_count(1) {
    }

    void report_settings(std::ostream& os) const {
      os << "hash duplicates store settings: ";
      os << "duplicates map type=" << multimap_type_to_string(multimap_type);
      os << ", shard count=" << shard_count << "\n";
    }

    void report_settings(dfxml_writer& x) const {
      x.push("hash_duplicates_store_settings");
      x.xmlout("duplicates_map_type", multimap_type_to_string(multimap_type));
      x.xmlout("shard_count", shard_count);
      x.pop();
    }

    hash_duplicates_store_settings_t& operator=(const hash_duplicates_store_settings_t& other) {
      multimap_type = other.multimap_type;
      shard_count = other.shard_count;
      return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hash_duplicates_store_settings_t& settings) {
  os << "(multimap_type=" << settings.multimap_type
     << ")";
  return os;
}

// source lookup
struct source_lookup_settings_t {
    number_of_index_bits_type_t number_of_index_bits_type;
    multi_index_container_type_t multi_index_container_type;

    source_lookup_settings_t(const source_lookup_settings_t& settings) :
        number_of_index_bits_type(settings.number_of_index_bits_type),
        multi_index_container_type(settings.multi_index_container_type) {
    }

    source_lookup_settings_t() :
        number_of_index_bits_type(NUMBER_OF_INDEX_BITS32),
        multi_index_container_type(BIDIRECTIONAL_BTREE) {
    }

    void report_settings(std::ostream& os) const {
      os << "source lookup settings: ";
      os << "number of index bits type=" << number_of_index_bits_type_to_string(number_of_index_bits_type);
      os << ", multi index container type=" << multi_index_container_type_to_string(multi_index_container_type);
    }

    void report_settings(dfxml_writer& x) const {
      x.push("source_lookup_settings");
      x.xmlout("number_of_index_bits_type", number_of_index_bits_type_to_string(number_of_index_bits_type));
      x.xmlout("multi_index_container_type", multi_index_container_type_to_string(multi_index_container_type));
      x.pop();
    }

    source_lookup_settings_t& operator=(const source_lookup_settings_t& other) {
      number_of_index_bits_type = other.number_of_index_bits_type;
      multi_index_container_type = other.multi_index_container_type;
      return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class source_lookup_settings_t& settings) {
  os << "(number_of_index_bits_type=" << settings.number_of_index_bits_type
     << ",multi_index_container_type=" << settings.multi_index_container_type
  ;
  return os;
}

// bloom
inline std::string bloom_state_to_string(bool state) {
  return (state) ? "enabled" : "disabled";
}

inline bool string_to_bloom_state(std::string state_string, bool& state) {
  if (state_string == "enabled")  { state = true;  return true; }
  if (state_string == "disabled") { state = false; return true; }
  state = false;
  return false;
}

struct bloom_settings_t {
    bool is_used;
    uint32_t M_hash_size;      // size of the bloom filter hash, in bits
    uint32_t k_hash_functions; // number of hash filter functions

    bloom_settings_t(bool _is_used, uint32_t _k_hash_functions, uint32_t _M_hash_size) :
        is_used(_is_used),
        M_hash_size(_M_hash_size),
        k_hash_functions(_k_hash_functions) {
    }

    bloom_settings_t(const bloom_settings_t& settings) :
        is_used(settings.is_used),
        M_hash_size(settings.M_hash_size),
        k_hash_functions(settings.k_hash_functions) {
    }

    void report_settings(std::ostream& os, size_t index) const {
      os << "bloom filter " << index << " settings: ";
      os << "status=" << bloom_state_to_string(is_used);
      os << ", k hash functions=" << k_hash_functions;
      os << ", M hash size=" << M_hash_size << "\n";
    }

    void report_settings(dfxml_writer& x, size_t index) const {
      std::stringstream s;
      s << "index='" << index << "'";
      x.push("bloom_filter_settings", s.str());
      x.xmlout("status", bloom_state_to_string(is_used));
      x.xmlout("k_hash_functions", (uint64_t)k_hash_functions);
      x.xmlout("M_hash_size", (uint64_t)M_hash_size);
      x.pop();
    }

    bloom_settings_t& operator=(const bloom_settings_t& other) {
      is_used = other.is_used;
      M_hash_size = other.M_hash_size;
      k_hash_functions = other.k_hash_functions;
      return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class bloom_settings_t& settings) {
  os << "(is_used=" << settings.is_used
     << ",M_hash_size=" << settings.M_hash_size
     << ",k_hash_functions=" << settings.k_hash_functions << ")";
  return os;
}

// hashdb tuning options
struct hashdb_settings_t {

    uint32_t hashdb_version;
    uint32_t hash_block_size;
    hashdigest_type_t hashdigest_type;
    uint32_t maximum_hash_duplicates;
    hash_store_settings_t hash_store_settings;
    hash_duplicates_store_settings_t hash_duplicates_store_settings;
    source_lookup_settings_t source_lookup_settings;
    bloom_settings_t bloom1_settings;
    bloom_settings_t bloom2_settings;

    hashdb_settings_t(const hashdb_settings_t& settings) : 
        hashdb_version(settings.hashdb_version),
        hash_block_size(settings.hash_block_size),
        hashdigest_type(settings.hashdigest_type),
        maximum_hash_duplicates(settings.maximum_hash_duplicates),
        hash_store_settings(settings.hash_store_settings),
        hash_duplicates_store_settings(settings.hash_duplicates_store_settings),
        source_lookup_settings(settings.source_lookup_settings),
        bloom1_settings(settings.bloom1_settings),
        bloom2_settings(settings.bloom2_settings) {
    }

    hashdb_settings_t() : 
        hashdb_version(1),
        hash_block_size(4096),
        hashdigest_type(HASHDIGEST_MD5),
        maximum_hash_duplicates(0),
        hash_store_settings(),
        hash_duplicates_store_settings(),
        source_lookup_settings(),
        bloom1_settings(true, 3, 28),
        bloom2_settings(false, 3, 28) {
    }

    void report_settings(std::ostream& os) const {
      os << "hashdb settings: ";
      os << "hashdb version=" << hashdb_version << ", ";
      os << "hash block size=" << hash_block_size << ", ";
      os << "hashdigest type=" << hashdigest_type_to_string(hashdigest_type) << ", ";
      os << "maximum hash duplicates=" << maximum_hash_duplicates << "\n";
      hash_store_settings.report_settings(os);
      hash_duplicates_store_settings.report_settings(os);
      source_lookup_settings.report_settings(os);
      bloom1_settings.report_settings(os, 1);
      bloom2_settings.report_settings(os, 2);
    }

    hashdb_settings_t& operator=(const hashdb_settings_t& other) {
      hashdb_version = other.hashdb_version;
      hash_block_size = other.hash_block_size;
      hashdigest_type = other.hashdigest_type;
      maximum_hash_duplicates = other.maximum_hash_duplicates;
      hash_store_settings = other.hash_store_settings;
      hash_duplicates_store_settings = other.hash_duplicates_store_settings;
      source_lookup_settings = other.source_lookup_settings;
      bloom1_settings = other.bloom1_settings;
      bloom2_settings = other.bloom2_settings;
      return *this;
    }

    void report_settings(dfxml_writer& x) const {
      x.xmlout("hashdb_version", hashdb_version);
      x.xmlout("hashdigest_type", hashdigest_type_to_string(hashdigest_type));
      x.xmlout("hash_block_size", hash_block_size);
      x.xmlout("maximum_hash_duplicates", (uint64_t)maximum_hash_duplicates);
      hash_store_settings.report_settings(x);
      hash_duplicates_store_settings.report_settings(x);
      source_lookup_settings.report_settings(x);
      bloom1_settings.report_settings(x, 1);
      bloom2_settings.report_settings(x, 2);
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hashdb_settings_t& settings) {
  os << "("
     << "hashdb_version=" << settings.hashdb_version
     << ",hash_block_size=" << settings.hash_block_size
     << ",hashdigest_type=" << hashdigest_type_to_string(settings.hashdigest_type)
     << ",maximum hash duplicates=" << settings.maximum_hash_duplicates
     << settings.hash_store_settings
     << settings.hash_duplicates_store_settings
     << settings.source_lookup_settings
     << settings.bloom1_settings
     << settings.bloom2_settings << ")";
  return os;
}

#endif

