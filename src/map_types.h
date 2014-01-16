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

#ifndef    map_types_h
#define    map_types_h

#include <stdint.h>
#include <cassert>
#include <string.h> // for memcmp
#include <string>
#include <iostream>
#include "dfxml/src/hash_t.h" // defines the hash block's cryptographic hash type

// ************************************************************
// map type enumerators for map, multimap, and multi_index_container
// ************************************************************
// single-value map types
enum map_type_t {
          MAP_BTREE,
          MAP_FLAT_SORTED_VECTOR,
          MAP_RED_BLACK_TREE,
          MAP_UNORDERED_HASH
};

inline std::string map_type_to_string(map_type_t type) {
  switch(type) {
    case MAP_BTREE:              return "btree";
    case MAP_FLAT_SORTED_VECTOR: return "flat-sorted-vector";
    case MAP_RED_BLACK_TREE:     return "red-black-tree";
    case MAP_UNORDERED_HASH:     return "unordered-hash";
    default: assert(0); return "";
  }
}

inline bool string_to_map_type(const std::string& name, map_type_t& type) {
  if (name == "btree")               { type = MAP_BTREE; return true; }
  if (name == "flat_sorted-vector")  { type = MAP_FLAT_SORTED_VECTOR; return true; }
  if (name == "red-black-tree")      { type = MAP_RED_BLACK_TREE; return true; }
  if (name == "unordered_hash")      { type = MAP_UNORDERED_HASH; return true; }
  type = MAP_BTREE;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const map_type_t& t) {
  os << map_type_to_string(t);
  return os;
}

#endif

