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
 * This data structure is used by threads in the threadpool for ingesting
 * or scanning its referenced buffer of data.
 */

#ifndef HASHER_BUFFER_HPP
#define HASHER_BUFFER_HPP

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

class hasher_buffer_t {

  public:
  const std::string source_hash;
  const std::string name;
  const uint64_t offset;
  const uint8_t* const buffer;
  const size_t buffer_size;
  const size_t end_byte;
  const bool should_delete_buffer;
  const size_t recursion_count;

  hasher_buffer_t(const std::string& p_source_hash,
                  const std::string& p_name,
                  const uint64_t p_offset,
                  const uint8_t* const p_buffer,
                  const size_t p_buffer_size,
                  const size_t p_end_byte,
                  const bool p_should_delete_buffer,
                  const size_t p_recursion_count) :
       source_hash(p_source_hash),
       name(p_name),
       offset(p_offset),
       buffer(p_buffer),
       buffer_size(p_buffer_size),
       end_byte(p_end_byte),
       should_delete_buffer(p_should_delete_buffer),
       recursion_count(p_recursion_count) {
  }

  // do not allow copy or assignment
  hasher_buffer_t(const hasher_buffer_t&);
  hasher_buffer_t& operator=(const hasher_buffer_t&);

  // append nest offset and type to name, for example "a.out-1000-ZIP"
  static std::string nested_name(const std::string& name,
                                 const uint64_t nest_offset,
                                 const std::string& nest_type) {
    std::stringstream ss;
    ss << "-" << nest_offset << "-" << nest_type;
    return ss.str();
  }
};

} // end namespace hasher

#endif
