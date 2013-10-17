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
 * Provides interfaces to the source lookup store.
 */

// file adapted from btree/example/udt.hpp

#ifndef INDEXED_STRING_T_HPP
#define INDEXED_STRING_T_HPP

#include <boost/btree/index_helpers.hpp>
#include <boost/btree/support/string_view.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;
using boost::string_view;

struct indexed_string_t {
  typedef uint64_t index_type;

  uint64_t index;
  string_view value;

  indexed_string_t() {}
  indexed_string_t(uint64_t p_index, const std::string p_value) :
                             index(p_index), value(p_value) {
  }

  bool operator < (const indexed_string_t& rhs) const {
    return value < rhs.index;
  }
};

//  stream inserter  ---------------------------------------------------------//

inline std::ostream& operator<<(std::ostream& os,
                                const indexed_string_t& indexed_string)
{
  os << indexed_string.index
     << " \""
     << indexed_string.value
     << "\"";
  return os;
}

//  function objects for different orderings  --------------------------------//

struct value_ordering
{
  bool operator()(const indexed_string_t& x, const indexed_string_t& y) const
    {return x.value < y.value;}
};

//  specializations to support btree indexes  --------------------------------//

namespace boost {
namespace btree {
  template <> struct index_reference<indexed_string_t> {
    typedef const indexed_string_t type;
  };

  template <>
  inline void index_serialize<indexed_string_t>(const indexed_string_t& indexed_string, flat_file_type& file) {
    index_serialize(indexed_string.index, file);
    index_serialize(indexed_string.value, file);
  }

  template <>
  inline index_reference<indexed_string_t>::type index_deserialize<indexed_string_t>(const char** flat) {
    indexed_string_t indexed_string;
    indexed_string.index = index_deserialize<indexed_string_t::index_type>(flat);
    indexed_string.value = index_deserialize<boost::string_view>(flat);
    return indexed_string;
  }
}} // namespaces

#endif

