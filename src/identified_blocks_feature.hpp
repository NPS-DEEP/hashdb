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
 * Provides a record of information from an identified_blocks.txt file.
 */

#ifndef IDENTIFIED_BLOCKS_FEATURE_HPP
#define IDENTIFIED_BLOCKS_FEATURE_HPP

#include <cstring>
#include <stdint.h>
#include <iostream>
#include "hash_t_selector.h"

//struct identified_blocks_feature_t {
struct identified_blocks_feature_t {
  std::string offset_string;
  hash_t key;
  uint32_t count;

  identified_blocks_feature_t(std::string p_offset_string,
                      hash_t p_key,
                      uint32_t p_count) :
              offset_string(p_offset_string), key(p_key), count(p_count) {
  }

  identified_blocks_feature_t() : offset_string(""), key(), count(0) {
    // zero out the hash digest
    for (uint32_t i=0; i<hash_t::size(); i++) {
      key.digest[i] = 0;
    }
  }
};

#endif

