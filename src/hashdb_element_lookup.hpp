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
 * Provides factory helper.
 */

#ifndef HASHDB_ELEMENT_HELPER_HPP
#define HASHDB_ELEMENT_HELPER_HPP

#include <cstring>
#include <stdint.h>
#include "source_lookup_index_manager.hpp"
#include "hashdigest.hpp"
#include "settings_manager.hpp"
#include "hashdb_element.hpp"
#include <iostream>

class hashdb_element_lookup_t {
  private:
  const source_lookup_index_manager_t* source_lookup_index_manager;
  const settings_t* settings;

  public:
  hashdb_element_lookup_t(
            source_lookup_index_manager_t* p_source_lookup_index_manager,
            settings_t* p_settings) :
                   source_lookup_index_manager(p_source_lookup_index_manager),
                   settings(p_settings) {
  }

/*
  hashdb_element_lookup_t() :
                   source_lookup_index_manager(0),
                   settings(0) {
  }
*/

/*
  // Beware that I am not properly managing pointers here.
  hashdb_element_lookup_t(const hashdb_element_lookup_t& other) :
               source_lookup_index_manager(other.source_lookup_index_manager),
               settings(other.settings) {
  }

  // Beware that I am not properly managing pointers here.
  hashdb_element_lookup_t& operator=(const hashdb_element_lookup_t& other) {
    this->source_lookup_index_manager = other.source_lookup_index_manager;
    this->settings = other.settings;
    return *this;
  }
*/

  bool operator==(const hashdb_element_lookup_t& other) const {
    // equal if same object
    return (source_lookup_index_manager == other.source_lookup_index_manager);
  }

/* non-fancy
  // lookup
  hashdb_element_t do_lookup(
             const std::pair<hashdigest_t, uint64_t>& hashdb_pair) const {

    // get source strings from source_lookup_encoding
    uint64_t source_lookup_encoding = hashdb_pair.second;
    uint64_t source_lookup_index =
          source_lookup_encoding::get_source_lookup_index(
                  settings->source_lookup_index_bits, source_lookup_encoding);
    std::pair<std::string, std::string> source(
                     source_lookup_index_manager->find(source_lookup_index));

    // calculate file offset
    uint64_t hash_block_offset =
          source_lookup_encoding::get_hash_block_offset(
                  settings->source_lookup_index_bits, source_lookup_encoding);
    uint64_t file_offset = hash_block_offset * settings->hash_block_size;

    // put hashdb element together
    hashdb_element_t hashdb_element(
                         hashdb_pair.first.hashdigest,
                         hashdb_pair.first.hashdigest_type,
                         settings->hash_block_size,
                         source.first,
                         source.second,
                         file_offset);

    return hashdb_element;
  }
*/

  // lookup
  hashdb_element_t operator()(
                 const std::pair<hashdigest_t, uint64_t>& hashdb_pair) const {

    // get source strings from source_lookup_encoding
    uint64_t source_lookup_encoding = hashdb_pair.second;
    uint64_t source_lookup_index =
          source_lookup_encoding::get_source_lookup_index(
                  settings->source_lookup_index_bits, source_lookup_encoding);
    std::pair<std::string, std::string> source(
                     source_lookup_index_manager->find(source_lookup_index));

    // calculate file offset
    uint64_t hash_block_offset =
          source_lookup_encoding::get_hash_block_offset(
                  settings->source_lookup_index_bits, source_lookup_encoding);
    uint64_t file_offset = hash_block_offset * settings->hash_block_size;

    // put hashdb element together
    hashdb_element_t hashdb_element(
                         hashdb_pair.first.hashdigest,
                         hashdb_pair.first.hashdigest_type,
                         settings->hash_block_size,
                         source.first,
                         source.second,
                         file_offset);

    return hashdb_element;
  }
};

#endif

