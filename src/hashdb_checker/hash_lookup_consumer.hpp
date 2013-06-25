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
 * Provides the hash lookup service for testing hashdb query interfaces.
 */

#ifndef HASH_LOOKUP_CONSUMER_HPP
#define HASH_LOOKUP_CONSUMER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
//#include <vector>
#include <map>
#include "hashdb.hpp"
#include "dfxml/src/hash_t.h"

// helper class
class hash_lookup_consumer_t {
  private:
  uint32_t query_id;
  hashdb::hashes_request_md5_t* request;
  std::map<uint32_t, std::string>* source_map;

  public:
  hash_lookup_consumer_t(hashdb::hashes_request_md5_t* p_request,
                         std::map<uint32_t, std::string>* p_source_map) :
          query_id(0), request(p_request), source_map(p_source_map) {
  }

  // consume hashdb element by adding it to the request list
  void consume(const hashdb_element_t& hashdb_element) {

    md5_t md5 = hashdb_element.first;
    hash_source_record_t hash_source_record = hashdb_element.second;

    // prepare the raw digest
    uint8_t digest[16];
    memcpy(digest, md5.digest, 16);

    // prepare the source text
    std::stringstream ss;
    ss << hash_source_record.filename << ":" << hash_source_record.file_offset;
    std::string source = ss.str();

    // add hash to request
    request->push_back(hashdb::hash_request_md5_t(query_id, digest));

    // add source to source map
    (*source_map)[query_id] = source;

    // increment the query ID
    query_id++;
  }
};

#endif

