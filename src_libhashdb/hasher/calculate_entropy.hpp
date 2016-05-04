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
 * Calculate entropy from data.
 */

#ifndef CALCULATE_ENTROPY_HPP
#define CALCULATE_ENTROPY_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>

namespace hasher {

  static size_t calculate_entropy_private(const uint8_t* const buffer,
                                          const size_t count) {
    // zzzzzzzzz
    return 86;
  }

  size_t calculate_entropy(uint8_t* const buffer,
                           const size_t buffer_size,
                           const size_t offset,
                           const size_t count) {

    if (offset + count <= buffer_size) {
      // calculate when not a buffer overrun
      return calculate_entropy_private(buffer + offset, count);
    } else {
      // make new buffer from old but zero-extended
      b = ::calloc(buffer_size, 1);
      size_t entropy = calculate_entropy_private(b, count);
      free(b);
      return entropy;
    }
  }
} // end namespace hasher

#endif
