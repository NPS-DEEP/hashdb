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
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type

class source_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  source_lookup_index_manager_t source_lookup_index_manager;

  // convenience
  uint8_t source_lookup_index_bits;
  uint32_t block_size;

  public:
  hashdb_element_generator_t(std::string p_hashdb_dir,
                             file_mode_type_t p_file_mode) :
                 hashdb_dir(p_hashdb_dir),
                 file_mode(p_file_mode),
                 source_lookup_index_manager(hashdb_dir, file_mode) {
  }
/*
                 uint8_t p_source_lookup_index_bits,
                 uint32_t p_block_size) :
           source_lookup_index_manager(p_source_lookup_index_manager),
           source_lookup_index_bits(p_source_lookup_index_bits),
           block_size(p_block_size) {
  }
*/

  // I allow these without protecting against null pointer, which is very bad,
  // I should manage this better.
  hashdb_element_generator_t(const hashdb_element_generator_t& theirs) :
               source_lookup_index_manager(theirs.source_lookup_index_manager),
               source_lookup_index_bits(theirs.source_lookup_index_bits),
               block_size(theirs.block_size) {
  }

  hashdb_element_generator_t& operator=(const hashdb_element_generator_t& theirs) {
    this.source_lookup_index_manager = theirs.source_lookup_index_manager;
    this.source_lookup_index_bits = theirs.source_lookup_index_bits;
    this.block_size = theirs.block_size;
  }

/*
  // I provide this function object as a way to minimize exposure to internals
  // while providing this to hashdb_manager and hashdb_iterator
  hashdb_element_t operator()(const std::pair<hashdigest_t, uint64_t>&
                                  map_multimap_element) const {}
*/

  source_element_t get_source_element(

    // get source strings from source_lookup_encoding
    uint64_t source_lookup_encoding = map_multimap_element.second;
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
                         map_multimap_element.first.hashdigest,
                         map_multimap_element.first.hashdigest_type,
                         block_size,
                         source.first,
                         source.second,
                         file_offset);

    return source_element;
  }
};

#endif

