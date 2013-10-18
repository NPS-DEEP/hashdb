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
 * Provides indexable key, value data type for use with btree.
 */

// file adapted from btree/example/udt.hpp

#ifndef INDEXED_STRING_T_HPP
#define INDEXED_STRING_T_HPP

#include <boost/btree/index_helpers.hpp>
#include <boost/btree/support/string_view.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;
//using boost::string_view;

template<typename KEY_T, typename PAY_T>
struct two_index_set_t {
  typedef uint64_t key_type;

  KEY_T key;
  PAY_T payload;
  uint64_t index;
  string_view value;

  two_index_set_t() {}
  two_index_set_t(uint64_t p_index, const std::string p_value) :
                             index(p_index), value(p_value) {
  }

  bool operator < (const two_index_set_t& rhs) const {
    return value < rhs.index;
  }
};

//  stream inserter  ---------------------------------------------------------//

inline std::ostream& operator<<(std::ostream& os,
                                const two_index_set_t& two_index_set)
{
  os << two_index_set.index
     << " \""
     << two_index_set.value
     << "\"";
  return os;
}

//  function objects for different orderings  --------------------------------//

struct value_ordering
{
  bool operator()(const two_index_set_t& x, const two_index_set_t& y) const
    {return x.value < y.value;}
};

//  specializations to support btree indexes  --------------------------------//

namespace boost {
namespace btree {
  template <> struct index_reference<two_index_set_t> {
    typedef const two_index_set_t type;
  };

  template <>
  inline void index_serialize<two_index_set_t>(const two_index_set_t& two_index_set, flat_file_type& file) {
    index_serialize(two_index_set.index, file);
    index_serialize(two_index_set.value, file);
  }

  template <>
  inline index_reference<two_index_set_t>::type index_deserialize<two_index_set_t>(const char** flat) {
    two_index_set_t two_index_set;
    two_index_set.index = index_deserialize<two_index_set_t::key_type>(flat);
    two_index_set.value = index_deserialize<boost::string_view>(flat);
    return two_index_set;
  }
}} // namespaces

#endif

