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

#ifndef RANDOM_BINARY_HASH_HPP
#define RANDOM_BINARY_HASH_HPP
#include <time.h> // for random number generator

/**
 * Provides a randomly generated hash.
 */

std::string random_binary_hash() {

  // random hash buffer
  union hash_buffer_t {
    char hash[16];
    uint32_t words[(16+3)/4];
    hash_buffer_t() {
      for (size_t i=0; i<(16+3)/4; i++) {
        words[i]=rand();
      }
    }
  };
  return std::string(chars, 16);
}

#endif

