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

#ifndef    hashdb_types_h
#define    hashdb_types_h

#include "source_lookup_record.h"
#include <stdint.h>
#include <cassert>
#include <string.h> // for memcmp
#include <string>
#include <iostream>
#include "dfxml/src/hash_t.h"    // defines chunk hash type

// ************************************************************
// file mode types
// ************************************************************
enum file_mode_type_t {READ_ONLY,
                       RW_NEW,
                       RW_MODIFY};

inline std::string file_mode_type_to_string(file_mode_type_t type) {
  switch(type) {
    case READ_ONLY: return "read_only";
    case RW_NEW: return "rw_new";
    case RW_MODIFY: return "rw_modify";
    default: assert(0); return "";
  }
}

inline bool string_to_file_mode_type(const std::string& name, file_mode_type_t& type) {
  if (name == "read_only") { type = READ_ONLY; return true; }
  if (name == "rw_new")    { type = RW_NEW;    return true; }
  if (name == "rw_modify") { type = RW_MODIFY; return true; }
  type = READ_ONLY;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const file_mode_type_t& t) {
  os << file_mode_type_to_string(t);
  return os;
}

// ************************************************************
// map type enumerators for map, multimap, and multi_index_container
// ************************************************************
// single-value map types
enum map_type_t {MAP_RED_BLACK_TREE,
                 MAP_SORTED_VECTOR,
                 MAP_HASH,
                 MAP_BTREE};

inline std::string map_type_to_string(map_type_t type) {
  switch(type) {
    case MAP_RED_BLACK_TREE: return "red-black-tree";
    case MAP_SORTED_VECTOR: return "sorted-vector";
    case MAP_HASH: return "hash";
    case MAP_BTREE: return "btree";
    default: assert(0); return "";
  }
}

inline bool string_to_map_type(const std::string& name, map_type_t& type) {
  if (name == "red-black-tree") { type = MAP_RED_BLACK_TREE; return true; }
  if (name == "sorted-vector")  { type = MAP_SORTED_VECTOR;  return true; }
  if (name == "hash")           { type = MAP_HASH;           return true; }
  if (name == "btree")          { type = MAP_BTREE;          return true; }
  type = MAP_RED_BLACK_TREE;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const map_type_t& t) {
  os << map_type_to_string(t);
  return os;
}

// multimap map types
enum multimap_type_t {MULTIMAP_SIMPLE_STD,
                      MULTIMAP_RED_BLACK_TREE,
                      MULTIMAP_SORTED_VECTOR,
                      MULTIMAP_HASH,
                      MULTIMAP_BTREE};

inline std::string multimap_type_to_string(multimap_type_t type) {
  switch(type) {
    case MULTIMAP_RED_BLACK_TREE: return "red-black-tree";
    case MULTIMAP_SORTED_VECTOR: return "sorted-vector";
    case MULTIMAP_HASH: return "hash";
    case MULTIMAP_BTREE: return "btree";
    default: assert(0); return "";
  }
}

inline bool string_to_multimap_type(const std::string& name, multimap_type_t& type) {
  if (name == "red-black-tree") { type = MULTIMAP_RED_BLACK_TREE; return true; }
  if (name == "sorted-vector")  { type = MULTIMAP_SORTED_VECTOR;  return true; }
  if (name == "hash")           { type = MULTIMAP_HASH;           return true; }
  if (name == "btree")          { type = MULTIMAP_BTREE;          return true; }
  type = MULTIMAP_RED_BLACK_TREE;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const multimap_type_t& t) {
  os << multimap_type_to_string(t);
  return os;
}

// variable length value reversible value lookup map type
enum multi_index_container_type_t {MULTI_INDEX_CONTAINER};

inline std::string multi_index_container_type_to_string(multi_index_container_type_t type) {
  switch(type) {
    case MULTI_INDEX_CONTAINER: return "multi-index-container";
    default: assert(0); return "";
  }
}

inline bool string_to_multi_index_container_type(const std::string& name, multi_index_container_type_t& type) {
  if (name == "multi-index-container") {
    type = MULTI_INDEX_CONTAINER;
    return true;
  }
  type = MULTI_INDEX_CONTAINER;
  return false;
}

inline std::ostream& operator<<(std::ostream& os,
                         const multi_index_container_type_t& t) {
  os << multi_index_container_type_to_string(t);
  return os;
}
 
// ************************************************************
// higher level types used internally
// ************************************************************
// hash types
enum hashdigest_type_t {HASHDIGEST_UNDEFINED,
                        HASHDIGEST_MD5};

inline std::string hashdigest_type_to_string(hashdigest_type_t type) {
  switch(type) {
    case HASHDIGEST_MD5: return "MD5";
    default: return "undefined";
  }
}

inline bool string_to_hashdigest_type(const std::string& name, hashdigest_type_t& type) {
  if (name == "MD5") {
    type = HASHDIGEST_MD5;
    return true;
  }
  std::cerr << "wrong hashdigest type '" << name << "'\n";
  type = HASHDIGEST_UNDEFINED;
  return false;
}

// hash source record
struct hash_source_record_t {
    uint64_t file_offset;
    uint32_t chunk_size;
    std::string hashdigest_type_string;
    std::string repository_name;
    std::string filename;


    hash_source_record_t() : file_offset(0),
                             chunk_size(0),
                             hashdigest_type_string(""),
                             repository_name(""),
                             filename("") {
    }

    hash_source_record_t(uint64_t p_file_offset,
                         size_t p_chunk_size,
                         std::string p_hashdigest_type_string,
                         std::string p_repository_name,
                         std::string p_filename) :
                               file_offset(p_file_offset),
                               chunk_size(p_chunk_size),
                               hashdigest_type_string(p_hashdigest_type_string),
                               repository_name(p_repository_name),
                               filename(p_filename) {
    }

    bool const operator==(const hash_source_record_t& hash_source_record) const {
      return (hash_source_record.file_offset == this->file_offset
           && hash_source_record.chunk_size == this->chunk_size
           && hash_source_record.hashdigest_type_string == this->hashdigest_type_string
           && hash_source_record.repository_name == this->repository_name
           && hash_source_record.filename == this->filename);
    }

    bool const operator<(const hash_source_record_t& hash_source_record) const {
      if (file_offset < hash_source_record.file_offset) return true;
      if (chunk_size < hash_source_record.chunk_size) return true;
      if (hashdigest_type_string < hash_source_record.hashdigest_type_string) return true;
      if (repository_name < hash_source_record.repository_name) return true;
      if (filename < hash_source_record.filename) return true;
      return false;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class hash_source_record_t& hash_source_record) {
//  os << "(hash_source_record_t file_offset=" << hash_source_record.file_offset
  os << "(file_offset=" << hash_source_record.file_offset
     << ",chunk_size=" << hash_source_record.chunk_size

     << ",hashdigest_type_string=" << hash_source_record.hashdigest_type_string
     << ",repository_name=" << hash_source_record.repository_name
     << ",filename=" << hash_source_record.filename << ")";
  return os;
}
// hashdb element pair
typedef std::pair<md5_t, hash_source_record_t> hashdb_element_t;

inline std::ostream& operator<<(std::ostream& os,
                         const hashdb_element_t& hashdb_element) {
  os << "(md5=" << hashdb_element.first.hexdigest()
     << ",hash_source_record=" << hashdb_element.second << ")";
  return os;
}

#endif

