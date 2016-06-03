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
 * Provide a new buffer containing uncompressed data such as zip data.
 */

#ifndef UNCOMPRESS_HPP
#define UNCOMPRESS_HPP

#include "stdint.h"

namespace hasher {

  // zip
  inline bool zip_signature(const uint8_t* const b, const size_t b_size,
                     const size_t offset) {
    // do not let this overflow.
    if (offset + 30 < b_size) {
      return false;
    }

    return (b[0] == 0x50 && b[1]==0x4B && b[2]==0x03 && b[3]==0x04);
  }

  // return a new buffer which must be deleted, successful or not.
  std::string new_from_zip(const uint8_t* const in_buf,
                           const size_t in_size,
                           const size_t in_offset,
                           uint8_t** out_buf,
                           size_t* out_size);

} // end namespace hasher

#endif

