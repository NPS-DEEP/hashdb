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

#ifndef RANDOM_KEY_HPP
#define RANDOM_KEY_HPP
#include <time.h> // for random number generator
#include "hash_t_selector.h"

/**
 * Provides a randomly generated key.
 */

hash_t random_key() {
//  const size_t word_count = (sizeof(hash_t)+3)/4;

  // random key buffer
  union key_buffer_t {
    uint8_t key[sizeof(hash_t)];
    uint32_t words[(sizeof(hash_t)+3)/4];
    key_buffer_t() {
      for (size_t i=0; i<(sizeof(hash_t)+3)/4; i++) {
        words[i]=rand();
      }
    }
  };
  return hash_t(key_buffer_t().key);
}

#endif

