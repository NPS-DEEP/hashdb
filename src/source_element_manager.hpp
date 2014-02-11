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
 * Manage hash source elements.
 */

#ifndef SOURCE_ELEMENT_MANAGER_HPP
#define SOURCE_ELEMENT_MANAGER_HPP
#include "source_lookup_encoding.hpp"
#include "source_lookup_index_manager.hpp"
#include "hashdb_element.hpp"
#include "source_element.hpp"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type

class source_element_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  settings_t settings;
  source_lookup_index_manager_t source_lookup_index_manager;

//  // convenience
//  uint8_t source_lookup_index_bits;
//  uint32_t block_size;

  public:
  source_element_manager_t(std::string p_hashdb_dir,
                           file_mode_type_t p_file_mode) :
                 hashdb_dir(p_hashdb_dir),
                 file_mode(p_file_mode),
                 settings(settings_manager_t::read_settings(hashdb_dir));
                 source_lookup_index_manager(hashdb_dir, file_mode) {
  }

  source_element_t get_source_element(const hashdb_element_t& hashdb_element) {

    // get source strings from source_lookup_encoding
    uint64_t source_lookup_encoding = hashdb_element.second;
    uint64_t source_lookup_index =
                source_lookup_encoding::get_source_lookup_index(
                          source_lookup_index_bits, source_lookup_encoding);
    std::pair<std::string, std::string> source(
                     source_lookup_index_manager->find(source_lookup_index);

    // calculate file offset
    uint64_t hash_block_offset =
                source_lookup_encoding::get_hash_block_offset(
                          source_lookup_index_bits, source_lookup_encoding);
    uint64_t file_offset = hash_block_offset * block_size;

    // put source_element together
    source_element_t source_element(
                         hashdb_element.first.hashdigest,
                         hashdb_element.first.hashdigest_type,
                         block_size,
                         source.first,
                         source.second,
                         file_offset);

    return source_element;
  }
};

#endif

