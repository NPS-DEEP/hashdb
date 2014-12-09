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
 * Defines data structures for use with bidirectional btrees.
 * Adapted from btree/example/udt.hpp
 */

#ifndef INDEXED_HASH_STORE_TYPE_HPP
#define INDEXED_HASH_STORE_TYPE_HPP

#include "hash_t_selector.h"
#include <boost/btree/index_helpers.hpp>
#include <boost/btree/support/string_view.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;


/**
 * Structure with key=struct{hash_t} and value=uint64_t source_lookup_encoding
 */
struct indexed_hash_store_t {
  typedef hash_t key_type;
  typedef uint64_t value_type; // source_lookup_encoding

  key_type key;
  value_type value;

  indexed_hash_store_t() : key(), value(0) {
  }
  indexed_hash_store_t(key_type p_key, value_type p_value) :
            key(p_key), value(p_value) {
  }

  // ordering for value is source_lookup_index, see source_lookup_encoding
  struct value_ordering {
    bool operator()(const indexed_hash_store_t& x, const indexed_hash_store_t& y) const {
      return ((x.value>>34) < (y.value>>34));
    }

    bool operator()(const indexed_hash_store_t& x, const indexed_hash_store_t::value_type& y) const {
      return (x.value>>34 < y>>34);
    }

    bool operator()(const indexed_hash_store_t::value_type& x, const indexed_hash_store_t& y) const {
      return (x>>34 < y.value>>34);
    }
  };
};

// ordering for key
inline bool operator<(const indexed_hash_store_t& lhs, const indexed_hash_store_t& rhs) {
  return lhs.key < rhs.key;
}
inline bool operator<(const indexed_hash_store_t& lhs, indexed_hash_store_t::key_type rhs) {
  return lhs.key < rhs;
}
inline bool operator<(indexed_hash_store_t::key_type lhs, const indexed_hash_store_t& rhs) {
return lhs < rhs.key;
}

// stream inserter
inline std::ostream& operator<<(std::ostream& os, const indexed_hash_store_t& x) {
  os << "(" << x.key.hexdigest()
     << ", " << (x.value>>34)
     << ", " << (((uint64_t)1<<34) - 1) * HASHDB_BYTE_ALIGNMENT
     << ")";
  return os;
}

// specializations to support btree indexes
namespace boost {
namespace btree {
  template <>
  struct index_reference<indexed_hash_store_t> {
    typedef const indexed_hash_store_t type;
  };

  template <>
  inline void index_serialize<indexed_hash_store_t>(const indexed_hash_store_t& x, flat_file_type& file)
  {
    index_serialize(x.key, file);
    index_serialize(x.value, file);
  }

  template <>
  inline index_reference<indexed_hash_store_t>::type index_deserialize<indexed_hash_store_t>(const char** flat)
  {
    indexed_hash_store_t x;
    x.key = index_deserialize<indexed_hash_store_t::key_type>(flat);
    x.value = index_deserialize<indexed_hash_store_t::value_type>(flat);
    return x;
  }
}} // namespaces

#endif

