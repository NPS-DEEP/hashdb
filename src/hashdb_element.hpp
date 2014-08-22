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
 * This fully defines a hash, its type, ts block size,
 * and its source.
 */

#ifndef HASHDB_ELEMENT_HPP
#define HASHDB_ELEMENT_HPP

#include <cstring>
#include <stdint.h>
#include <iostream>
#include "hash_t_selector.h"

/**
 * A hashdb element fully describes a hash source.
 */
class hashdb_element_t {

  public:

  hash_t key;
  uint32_t hash_block_size;         // typically 4096
  std::string repository_name;
  std::string filename;
  uint64_t file_offset;        // should be a multiple of hash_block_size
  
  // fully specified
  hashdb_element_t(const hash_t& p_key,
                   uint32_t p_block_size,
                   const std::string p_repository_name,
                   const std::string p_filename,
                   uint64_t p_file_offset) :
          key(p_key),
          hash_block_size(p_block_size),
          repository_name(p_repository_name),
          filename(p_filename),
          file_offset(p_file_offset) {
  }

  // key-based with no source information
  hashdb_element_t(const hash_t& p_key) :
          key(p_key),
          hash_block_size(0),
          repository_name(""),
          filename(""),
          file_offset(0) {
  }

  // empty
  hashdb_element_t() :
          key(),
          hash_block_size(0),
          repository_name(""),
          filename(""),
          file_offset(0) {
  }
};

#endif

