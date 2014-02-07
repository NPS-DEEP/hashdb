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
 * This file defines basic types required for working with a hashdb.
 */

#ifndef HASHDIGEST_TYPES_H
#define HASHDIGEST_TYPES_H

#include <stdint.h>
#include <cassert>
#include <string.h> // for memcmp
#include <string>
#include <iostream>
#include "dfxml/src/hash_t.h" // defines the hash block's cryptographic hash type


// ************************************************************
// higher level types used internally
// ************************************************************
// hash types
enum hashdigest_type_t {HASHDIGEST_UNDEFINED,
                        HASHDIGEST_MD5};

inline std::string hashdigest_type_to_string(hashdigest_type_t type) {
  switch(type) {
    case HASHDIGEST_MD5: return "MD5";
    default: return "undefined";
  }
}

inline bool string_to_hashdigest_type(const std::string& name, hashdigest_type_t& type) {
  if (name == "MD5") {
    type = HASHDIGEST_MD5;
    return true;
  }
  std::cerr << "wrong hashdigest type '" << name << "'\n";
  type = HASHDIGEST_UNDEFINED;
  return false;
}

#endif

