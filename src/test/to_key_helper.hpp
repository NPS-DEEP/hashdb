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
 * Test the source lookup encoding module.
 */
#ifndef TO_KEY_HELPER_HPP
#define TO_KEY_HELPER_HPP

#include<sstream>
#include "../dfxml/src/hash_t.h"

template<typename T>
void to_key(uint64_t i, T& key) {
  std::ostringstream ss;
  ss << std::setw(T::size()*2) << std::setfill('0') << std::hex << i;
  key = T::fromhex(ss.str());
}

#endif

