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
 * This file defines basic types required for working with a hashdb.
 */

#ifndef    MULTIMAP_TYPES_H
#define    MULTIMAP_TYPES_H

#include <string>
#include <iostream>

// multimap types
enum multimap_type_t {
          MULTIMAP_BTREE,
          MULTIMAP_FLAT_SORTED_VECTOR,
          MULTIMAP_RED_BLACK_TREE,
          MULTIMAP_UNORDERED_HASH
};

inline std::string multimap_type_to_string(multimap_type_t type) {
  switch(type) {
    case MULTIMAP_BTREE:              return "btree";
    case MULTIMAP_FLAT_SORTED_VECTOR: return "flat-sorted-vector";
    case MULTIMAP_RED_BLACK_TREE:     return "red-black-tree";
    case MULTIMAP_UNORDERED_HASH:     return "unordered-hash";
    default: assert(0); std::exit(1);
  }
}

inline bool string_to_multimap_type(const std::string& name, multimap_type_t& type) {
  if (name == "btree")               { type = MULTIMAP_BTREE; return true; }
  if (name == "flat-sorted-vector")  { type = MULTIMAP_FLAT_SORTED_VECTOR; return true; }
  if (name == "red-black-tree")      { type = MULTIMAP_RED_BLACK_TREE; return true; }
  if (name == "unordered-hash")      { type = MULTIMAP_UNORDERED_HASH; return true; }
  type = MULTIMAP_BTREE;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const multimap_type_t& t) {
  os << multimap_type_to_string(t);
  return os;
}

#endif

