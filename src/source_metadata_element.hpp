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
 * This fully defines a hash, its type, ts block size,
 * and its source.
 */

#ifndef SOURCE_METADATA_ELEMENT_HPP
#define SOURCE_METADATA_ELEMENT_HPP

#include <cstring>
#include <stdint.h>
#include <iostream>
#include "hash_t_selector.h"

/**
 * A source metadata element fully describes a source.
 */
class source_metadata_element_t {

  public:

  const std::string repository_name;
  const std::string filename;
  const uint64_t file_size;
  const hash_t file_hash;
  
  // fully specified
  source_metadata_element_t(const std::string& p_repository_name,
                            const std::string& p_filename,
                            uint64_t p_file_size,
                            const hash_t p_file_hash) :
          repository_name(p_repository_name),
          filename(p_filename),
          file_size(p_file_size),
          file_hash(p_file_hash) {
  }

  // empty
  source_metadata_element_t() :
          repository_name(""),
          filename(""),
          file_size(0),
          file_hash(0) {
  }
};

#endif

