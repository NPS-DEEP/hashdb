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
 * Provides simple lookup and add interfaces for a two-index btree store.
 */

#ifndef SOURCE_METADATA_HPP
#define SOURCE_METADATA_HPP

#include "hash_t_selector.h" // to define hash_t
#include <string>
#include "boost/btree/index_helpers.hpp"
#include "boost/btree/btree_index_set.hpp"

// the source_metadata_t UDT
struct source_metadata_t {
  uint64_t source_lookup_index;
  hash_t hash;
  uint64_t file_size;

  source_metadata_t() : source_lookup_index(0), hash(), file_size(0) {
    // zero out the hash digest
    for (uint32_t i=0; i<hash_t::size(); i++) {
      hash.digest[i] = 0;
    }
  }

  source_metadata_t(uint64_t p_source_lookup_index,
                    hash_t p_hash,
                    uint64_t p_file_size) :
          source_lookup_index(p_source_lookup_index),
          hash(p_hash),
          file_size(p_file_size) {
  }

/*
  // order by source_lookup_index
  bool operator < (const source_metadata_t& rhs) const {
    return source_lookup_index < rhs.source_lookup_index;
  }
*/
};

// zzzzzzzzzzzzzz
// ordering by source_lookup_index
inline bool operator<(const source_metadata_t& lhs, const source_metadata_t& rhs) {return lhs.source_lookup_index < rhs.source_lookup_index;}
inline bool operator<(const source_metadata_t& lhs, uint64_t rhs) {return lhs.source_lookup_index < rhs;}
inline bool operator<(uint64_t lhs, const source_metadata_t& rhs) {return lhs < rhs.source_lookup_index;}
// zzzzzzzzzzzzzz

// function object for hash ordering
struct hash_ordering {
  bool operator()(const source_metadata_t& x, const source_metadata_t& y) const {
    return x.hash < y.hash;
  }
// zzzzzzzzzzzzzz
  bool operator()(const source_metadata_t& x, const hash_t& y) const {
    return x.hash < y;
  }
  bool operator()(const hash_t& x, const source_metadata_t& y) const {
    return x < y.hash;
  }
// zzzzzzzzzzzzzz
};

// function object for file size ordering
struct file_size_ordering {
  bool operator()(const source_metadata_t& x, const source_metadata_t& y) const {
    return x.file_size < y.file_size;
  }
  bool operator()(const source_metadata_t& x, uint64_t y) const {
    return x.file_size < y;
  }
  bool operator()(uint64_t x, const source_metadata_t& y) const {
    return x < y.file_size;
  }
};

// specializations to support btree indexes
namespace boost {
namespace btree {
template <>
struct index_reference<source_metadata_t> {
  typedef const source_metadata_t type;
};

template <>
inline void index_serialize<source_metadata_t>(
                  const source_metadata_t& udt, flat_file_type& file) {
  index_serialize(udt.source_lookup_index, file);
  index_serialize(udt.hash, file);
  index_serialize(udt.file_size, file);
}

template <>
inline index_reference<source_metadata_t>::type
                     index_deserialize<source_metadata_t>(const char** flat) {
  source_metadata_t udt;
  udt.source_lookup_index = index_deserialize<uint64_t>(flat);
  udt.hash = index_deserialize<hash_t>(flat);
  udt.file_size = index_deserialize<uint64_t>(flat);
  return udt;
}
}} // namespaces

#endif
