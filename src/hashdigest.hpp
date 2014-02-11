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
 * A hashdigest type identifies a hash value and its type as strings,
 * decoupled from hash type bindings.
 */

#ifndef HASHDIGEST_HPP
#define HASHDIGEST_HPP

#include <string>
#include <iostream>
#include "dfxml/src/hash_t.h"

struct hashdigest_t {
  std::string hashdigest;
  std::string hashdigest_type;

  hashdigest_t(const std::string& p_hashdigest, std::string p_hashdigest_type) :
                   hashdigest(p_hashdigest),
                   hashdigest_type(p_hashdigest_type) {
  }

  hashdigest_t(const md5_t& key) :
                   hashdigest(key.hexdigest()),
                   hashdigest_type(std::string("MD5")) {
  }
  hashdigest_t(const sha1_t& key) :
                   hashdigest(key.hexdigest()),
                   hashdigest_type(std::string("SHA1")) {
  }
  hashdigest_t(const sha256_t& key) :
                   hashdigest(key.hexdigest()),
                   hashdigest_type(std::string("SHA256")) {
  }

  hashdigest_t() :
                   hashdigest(""),
                   hashdigest_type("") {
  }
};
    
#endif

