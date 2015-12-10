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
 * Provides typedefs and simple classes for DB managers.
 */

#ifndef LMDB_TYPEDEFS_H
#define LMDB_TYPEDEFS_H

#include <string>
#include <vector>
#include "hashdb.hpp" // for hash_data_t

// id_offset_pairs_t vector<pair<source_id, file_offset>>
typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
typedef std::vector<id_offset_pair_t> id_offset_pairs_t;

// hashdb_scan_it_data_t
typedef std::pair<std::string, id_offset_pairs_t> hashdb_scan_it_data_t;

// source_names_t pair<repository_name, filename>
typedef std::pair<std::string, std::string> source_name_t;
typedef std::vector<source_name_t> source_names_t;

// source_metadata_t of tuple(source_id, filesize, positive_count)
class source_metadata_t {
  public:
  std::string file_binary_hash;
  uint64_t source_id;
  uint64_t filesize;
  uint64_t positive_count;
  source_metadata_t(const std::string p_file_binary_hash,
                    uint64_t p_source_id, uint64_t p_filesize,
                    uint64_t p_positive_count) :
          file_binary_hash(p_file_binary_hash),
          source_id(p_source_id),
          filesize(p_filesize),
          positive_count(p_positive_count) {
  }
};

typedef hashdb::hash_data_t hash_data_t;
typedef hashdb::hash_data_list_t hash_data_list_t;

// below is in hashdb.hpp

//// hash_data_t of tuple(binary_hash, file_offset, entropy_label)
//class hash_data_t {
//  public:
//  std::string binary_hash;
//  uint64_t file_offset;
//  std::string entropy_label;
//  hash_data_t(const std::string& p_binary_hash,
//              uint64_t p_file_offset,
//              const std::string& p_entropy_label) :
//          binary_hash(p_binary_hash),
//          file_offset(p_file_offset),
//          entropy_label(p_entropy_label) {
//  }
//};
//
//// hash_data_list_t
//typedef std::vector<hash_data_t> hash_data_list_t;
//
//// sql_source_it_data_t of tuple(file_binary_hash, source_metadata,
////                               source_names)
//class sql_source_it_data_t {
//  public:
//  std::string file_binary_hash;
//  source_metadata_t source_metadata;
//  source_names_t source_names;
//  sql_source_it_data_t(const std::string& p_file_binary_hash,
//                    const source_metadata_t& p_source_metadata,
//                    const source_names_t& p_source_names) :
//          file_binary_hash(p_file_binary_hash),
//          source_metadata(p_source_metadata),
//          source_names(p_source_names) {
//  }
//};

#endif

