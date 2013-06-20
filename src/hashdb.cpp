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
 * Class file for the hashdb query interface.
 */

#include <config.h>
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif
#include "hashdb.hpp"
#include "hashdb_types.h"                  // for hashdigest_type_t
#include "query_by_path.hpp"
#include "query_by_socket.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>

// ************************************************************ 
// version of the hashdb query library
// ************************************************************ 
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

// ************************************************************ 
// the lookup type for performing the query
// ************************************************************ 
std::string hashdb::lookup_type_to_string(lookup_type_t type) {
  switch(type) {
    case QUERY_USE_PATH: return "use_path";
    case QUERY_USE_SOCKET: return "use_socket";
    default: return "not_selected";
  }
}

bool hashdb::string_to_lookup_type(
                            const std::string& name, lookup_type_t& type) {
  if (name == "use_path")       { type = QUERY_USE_PATH;        return true; }
  if (name == "use_socket")     { type = QUERY_USE_SOCKET;      return true; }
  if (name == "not_selected")   { type = QUERY_NOT_SELECTED;    return true; }
  type = QUERY_NOT_SELECTED;
  return false;
}

// ************************************************************ 
// data structures required by the query interfaces
// ************************************************************ 
/**
 * data associated with one hash in a request
 */
// request
hashdb::hash_request_md5_t::hash_request_md5_t() : id(0), digest() {
}
// response serves correctly sized digest
hashdb::hash_request_md5_t::hash_request_md5_t(uint32_t p_id, const uint8_t* p_digest) :
         id(p_id), digest() {
  memcpy(this->digest,p_digest,16);
}

/**
 * Hash request sent to the query engine
 */
hashdb::hashes_request_md5_t::hashes_request_md5_t() :
             hash_requests() {
}

/**
 * data associated with one hash in a response
 */
// response
hashdb::hash_response_md5_t::hash_response_md5_t() :
             id(0), digest(), duplicates_count(0),
             source_lookup_index(0), chunk_offset_value(0) {
}

// response serves correctly sized digest
hashdb::hash_response_md5_t::hash_response_md5_t(
                    uint32_t p_id,
                    const uint8_t* p_digest,
                    uint32_t p_duplicates_count,
                    uint64_t p_source_lookup_index,
                    uint64_t p_chunk_offset_value) :
             id(p_id),
             duplicates_count(p_duplicates_count),
             source_lookup_index(p_source_lookup_index),
             chunk_offset_value(p_chunk_offset_value) {
  memcpy(this->digest,p_digest,16);
}

/**
 * Hash lookup response returned from the query engine
 */
hashdb::hashes_response_md5_t::hashes_response_md5_t() :
             chunk_size(0), hash_responses() {
}

void hashdb::hashes_response_md5_t::clear() {
  chunk_size = 0;
  hash_responses.clear();
}

// ************************************************************ 
// the query interfaces
// ************************************************************ 

hashdb::query_t::query_t(
                              hashdb::lookup_type_t p_lookup_type,
                              const std::string& lookup_source_string) :
                                        lookup_type(p_lookup_type),
                                        query_by_path(0),
                                        query_by_socket(0) {

    switch(lookup_type) {
      case QUERY_USE_PATH:
        query_by_path = new query_by_path_t(lookup_source_string);
        break;
      case QUERY_USE_SOCKET:
        query_by_socket = new query_by_socket_t(lookup_source_string);
        break;
      default:
        // non-valid source type was provided
        std::cerr << "Error on init: A valid lookup service type is required.\n";
  }
}

hashdb::query_t::~query_t() {
  switch(lookup_type) {
    case QUERY_USE_PATH:
      delete query_by_path;
      break;
    case QUERY_USE_SOCKET:
      delete query_by_socket;
      break;
    default:
      // non-valid source type was provided
      std::cerr << "Error on delete: A valid lookup service type is required.\n";
  }
}

bool hashdb::query_t::lookup_hashes_md5(
                const hashdb::hashes_request_md5_t& request,
                hashdb::hashes_response_md5_t& response) {
    switch(lookup_type) {
      case QUERY_USE_PATH:
        return query_by_path->lookup_hashes_md5(request, response);
      case QUERY_USE_SOCKET:
        return query_by_socket->lookup_hashes_md5(request, response);
      default:
        // non-valid source type was provided
        std::cerr << "Error on lookup: A valid lookup service type is required.\n";
        return false;
    }
}
 
