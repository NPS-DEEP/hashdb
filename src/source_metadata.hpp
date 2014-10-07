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
 * Provides simple lookup and add interfaces for a two-index btree store.
 */

#ifndef SOURCE_METADATA_HPP
#define SOURCE_METADATA_HPP

#include "hash_t_selector.h" // to define hash_t
#include <string>
#include "boost/btree/index_helpers.hpp"
#include "boost/btree/btree_index_set.hpp"

// the source_metadata_t UDT
struct source_metadata_t {
  uint64_t source_lookup_index;
  uint64_t file_size;
  hash_t file_hash;

  source_metadata_t() : source_lookup_index(0), file_size(0), file_hash() {
    // zero out the file hash digest
    for (uint32_t i=0; i<hash_t::size(); i++) {
      file_hash.digest[i] = 0;
    }
  }

  source_metadata_t(uint64_t p_source_lookup_index,
                    uint64_t p_file_size,
                    hash_t p_hash) :
          source_lookup_index(p_source_lookup_index),
          file_size(p_file_size),
          file_hash(p_hash) {
  }
};


#endif
