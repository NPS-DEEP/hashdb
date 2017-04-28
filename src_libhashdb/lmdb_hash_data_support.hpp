// Author:  Bruce Allen
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
 * Provides low-level support for moving the cursor and for reading and
 * writing Type1, Type2, and Type3 records in lmdb_hash_data_store.
 * See lmdb_hash_data_manager.
 */

#ifndef LMDB_HASH_DATA_SUPPORT_HPP
#define LMDB_HASH_DATA_SUPPORT_HPP

#include <unistd.h>
#include <string>

namespace hashdb {

  // MAX
  const int max_block_label_size;

  // move cursor to first entry of current key
  void cursor_to_first_current(hashdb::lmdb_context_t& context);

  // move cursor forward from Type 2 to correct Type 3 else false and rewind
  bool cursor_to_type3(hashdb::lmdb_context_t& context,
                       const uint64_t source_id);

  // parse Type 1 context.data into these parameters
  void decode_type1(hashdb::lmdb_context_t& context,
                    uint64_t& k_entropy,
                    std::string& block_label,
                    uint64_t& source_id,
                    uint64_t& sub_count);

  // parse Type 2 context.data into these parameters
  void decode_type2(hashdb::lmdb_context_t& context,
                    uint64_t& k_entropy,
                    std::string& block_label,
                    uint64_t& count);

  // parse Type 3 context.data into these parameters
  void decode_type3(hashdb::lmdb_context_t& context,
                    uint64_t& source_id,
                    uint64_t& sub_count);

  // write new Type 1 record, key must be valid
  void new_type1(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 uint64_t k_entropy,
                 const std::string& block_label,
                 uint64_t source_id,
                 uint64_t sub_count);

  // write new Type 3 record, key must be valid
  void new_type3(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 uint64_t& source_id,
                 uint64_t& sub_count);

  // replace Type 1 record at cursor
  void replace_type1(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t k_entropy,
                     const std::string& block_label,
                     uint64_t source_id,
                     uint64_t sub_count);

  // replace Type 2 record at cursor
  void replace_type2(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t k_entropy,
                     const std::string& block_label,
                     uint64_t count);

  // replace Type 3 record at cursor
  void replace_type3(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t& source_id,
                     uint64_t& sub_count);

} // end namespace hashdb

#endif

