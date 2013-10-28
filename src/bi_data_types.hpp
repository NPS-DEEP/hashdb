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

#ifndef BI_DATA_TYPES_HPP
#define BI_DATA_TYPES_HPP

#include <boost/btree/index_helpers.hpp>
#include <boost/btree/support/string_view.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;

/**
 * Structure with key=uint64_t and value=boost::string_view.
 */
struct bi_data_64_sv_t {
  typedef uint64_t key_type;
//  typedef std::pair<uint64_t, uint64_t> value_type;
  typedef boost::string_view value_type;

  key_type key;
  value_type value;

  bi_data_64_sv_t() : key(0), value("") {}
  bi_data_64_sv_t(key_type p_key, value_type p_value) : key(p_key), value(p_value) {}

  // ordering for value
  struct value_ordering {
    bool operator()(const bi_data_64_sv_t& x, const bi_data_64_sv_t& y) const {
      return (x.value < y.value);
    }

    bool operator()(const bi_data_64_sv_t& x, const bi_data_64_sv_t::value_type& y) const {
      return (x.value < y);
    }

    bool operator()(const bi_data_64_sv_t::value_type& x, const bi_data_64_sv_t& y) const {
      return (x < y.value);
    }
  };
};

// ordering for key
inline bool operator<(const bi_data_64_sv_t& lhs, const bi_data_64_sv_t& rhs) {return lhs.key < rhs.key;}
inline bool operator<(const bi_data_64_sv_t& lhs, uint64_t rhs) {return lhs.key < rhs;}
inline bool operator<(uint64_t lhs, const bi_data_64_sv_t& rhs) {return lhs < rhs.key;}

// stream inserter
inline std::ostream& operator<<(std::ostream& os, const bi_data_64_sv_t& x) {
  os << "(" << x.key << ", \"" << x.value << "\")";
  return os;
}

// specializations to support btree indexes
namespace boost {
namespace btree {
  template <>
  struct index_reference<bi_data_64_sv_t> {
    typedef const bi_data_64_sv_t type;
  };

  template <>
  inline void index_serialize<bi_data_64_sv_t>(const bi_data_64_sv_t& x, flat_file_type& file)
  {
    index_serialize(x.key, file);
    index_serialize(x.value, file);
  }

  template <>
  inline index_reference<bi_data_64_sv_t>::type index_deserialize<bi_data_64_sv_t>(const char** flat)
  {
    bi_data_64_sv_t x;
    x.key = index_deserialize<uint64_t>(flat);
    x.value = index_deserialize<boost::string_view>(flat);
    return x;
  }
}} // namespaces

/**
 * Structure with key=uint64_t and value=std::pair<uint64_t, uint64_t>
 */
struct bi_data_64_pair_t {
  typedef uint64_t key_type;
  typedef std::pair<uint64_t, uint64_t> value_type;

  key_type key;
  value_type value;

  bi_data_64_pair_t() : key(0), value(value_type(0,0)) {}
  bi_data_64_pair_t(key_type p_key, value_type p_value) : key(p_key), value(p_value) {}

  // ordering for value
  struct value_ordering {
    bool operator()(const bi_data_64_pair_t& x, const bi_data_64_pair_t& y) const {
      return (x.value.first < y.value.first
          || (x.value.first == y.value.first && x.value.second < y.value.second));
    }

    bool operator()(const bi_data_64_pair_t& x, const bi_data_64_pair_t::value_type& y) const {
      return (x.value.first < y.first
          || (x.value.first == y.first && x.value.second < y.second));
    }

    bool operator()(const bi_data_64_pair_t::value_type& x, const bi_data_64_pair_t& y) const {
      return (x.first < y.value.first
          || (x.first == y.value.first && x.second < y.value.second));
    }
  };
};

// ordering for key
inline bool operator<(const bi_data_64_pair_t& lhs, const bi_data_64_pair_t& rhs) {return lhs.key < rhs.key;}
inline bool operator<(const bi_data_64_pair_t& lhs, uint64_t rhs) {return lhs.key < rhs;}
inline bool operator<(uint64_t lhs, const bi_data_64_pair_t& rhs) {return lhs < rhs.key;}

// stream inserter
inline std::ostream& operator<<(std::ostream& os, const bi_data_64_pair_t& x) {
  os << "(" << x.key << ", (" << x.value.first << ", " << x.value.second << "))";
  return os;
}

// specializations to support btree indexes
namespace boost {
namespace btree {
  template <>
  struct index_reference<bi_data_64_pair_t> {
    typedef const bi_data_64_pair_t type;
  };

  template <>
  inline void index_serialize<bi_data_64_pair_t>(const bi_data_64_pair_t& x, flat_file_type& file)
  {
    index_serialize(x.key, file);
    index_serialize(x.value.first, file);
    index_serialize(x.value.second, file);
  }

  template <>
  inline index_reference<bi_data_64_pair_t>::type index_deserialize<bi_data_64_pair_t>(const char** flat)
  {
    bi_data_64_pair_t x;
    x.key = index_deserialize<uint64_t>(flat);
    x.value.first = index_deserialize<bi_data_64_pair_t::value_type::first_type>(flat);
    x.value.second = index_deserialize<bi_data_64_pair_t::value_type::second_type>(flat);
    return x;
  }
}} // namespaces

#endif

