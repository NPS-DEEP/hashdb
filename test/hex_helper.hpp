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
 * Help with hexadecimal.
 */

#ifndef HEX_HELPER_HPP
#define HEX_HELPER_HPP

/**
 * Return empty if hexdigest length is not even or has any invalid digits.
 *
 * Copied from lmdb_helper.h.
 */
static std::string hex_to_binary_hash(const std::string& hex_string) {

  size_t size = hex_string.size();
  // size must be even
  if (size%2 != 0) {
    std::cout << "hex input not aligned on even boundary in '"
              << hex_string << "'\n";
    return "";
  }

  size_t i = 0;
  size_t j = 0;
  uint8_t bin[size];
  for (; i<size; i+=2) {
    uint8_t c0 = hex_string[i];
    uint8_t c1 = hex_string[i+1];
    uint8_t d0;
    uint8_t d1;

    if(c0>='0' && c0<='9') d0 = c0-'0';
    else if(c0>='a' && c0<='f') d0 = c0-'a'+10;
    else if(c0>='A' && c0<='F') d0 = c0-'A'+10;
    else {
      std::cout << "unexpected hex character in '"
              << hex_string << "'\n";
      return "";
    }
 
    if(c1>='0' && c1<='9') d1 = c1-'0';
    else if(c1>='a' && c1<='f') d1 = c1-'a'+10;
    else if(c1>='A' && c1<='F') d1 = c1-'A'+10;
    else {
      std::cout << "unexpected hex character in '"
              << hex_string << "'\n";
      return "";
    }

    bin[j++] = d0 << 4 | d1;
  }
  return std::string(reinterpret_cast<char*>(bin), j);
}


#endif

