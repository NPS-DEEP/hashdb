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
// the query type for performing the query
// ************************************************************ 
std::string hashdb::query_type_to_string(query_type_t type) {
  switch(type) {
    case QUERY_USE_PATH: return "use_path";
    case QUERY_USE_SOCKET: return "use_socket";
    default: return "not_selected";
  }
}

bool hashdb::string_to_query_type(
                            const std::string& name, query_type_t& type) {
  if (name == "use_path")       { type = QUERY_USE_PATH;        return true; }
  if (name == "use_socket")     { type = QUERY_USE_SOCKET;      return true; }
  if (name == "not_selected")   { type = QUERY_NOT_SELECTED;    return true; }
  type = QUERY_NOT_SELECTED;
  return false;
}

// ************************************************************ 
// data structures required by the query interfaces
// ************************************************************ 
// hash request
hashdb::hash_request_md5_t::hash_request_md5_t() : id(0), digest() {
}
hashdb::hash_request_md5_t::hash_request_md5_t(uint32_t p_id, const uint8_t* p_digest) :
         id(p_id), digest() {
  memcpy(this->digest,p_digest,16);
}

// hash response, also same as source request
hashdb::hash_response_md5_t::hash_response_md5_t() :
             id(0), digest(), duplicates_count(0),
             source_query_index(0), hash_block_offset_value(0) {
}
hashdb::hash_response_md5_t::hash_response_md5_t(
                    uint32_t p_id,
                    const uint8_t* p_digest,
                    uint32_t p_duplicates_count,
                    uint64_t p_source_query_index,
                    uint64_t p_hash_block_offset_value) :
             id(p_id),
             duplicates_count(p_duplicates_count),
             source_query_index(p_source_query_index),
             hash_block_offset_value(p_hash_block_offset_value) {
  memcpy(this->digest,p_digest,16);
}

// one source reference
hashdb::source_reference_t::source_reference_t() :
             repository_name(""),
             filename(""),
             file_offset(0) {
}
hashdb::source_reference_t::source_reference_t(
                    std::string p_repository_name,
                    std::string p_filename,
                    uint64_t p_file_offset) :
             repository_name(p_repository_name),
             filename(p_filename),
             file_offset(p_file_offset) {
}

// source response for one hash
hashdb::source_response_md5_t::source_response_md5_t() :
             id(0),
             source_references() {
}
hashdb::source_response_md5_t::source_response_md5_t(
                    uint32_t p_id,
                    const uint8_t* p_digest) :
             id(p_id),
             source_references() {
  memcpy(this->digest,p_digest,16);
}

// ************************************************************ 
// the query interfaces
// ************************************************************ 

hashdb::query_t::query_t(hashdb::query_type_t p_query_type,
                         const std::string& p_query_source_string) :
             query_type(p_query_type),
             query_by_path(0),
             query_by_socket(0) {

    switch(query_type) {
      case QUERY_USE_PATH:
        query_by_path = new query_by_path_t(p_query_source_string);
        break;
      case QUERY_USE_SOCKET:
        query_by_socket = new query_by_socket_t(p_query_source_string);
        break;
      default:
        // non-valid source type was provided
        std::cerr << "Error on init: A valid query service type is required.\n";
  }
}

hashdb::query_t::~query_t() {
  switch(query_type) {
    case QUERY_USE_PATH:
      delete query_by_path;
      break;
    case QUERY_USE_SOCKET:
      delete query_by_socket;
      break;
    default:
      // non-valid source type was provided
      std::cerr << "Error on delete: A valid query service type is required.\n";
  }
}

int hashdb::query_t::query_status() const {
    switch(query_type) {
      case QUERY_USE_PATH:
        return query_by_path->query_status();
      case QUERY_USE_SOCKET:
        return query_by_socket->query_status();
      default:
        return -1;
    }
}

int hashdb::query_t::query_hashes_md5(
                const hashdb::hashes_request_md5_t& request,
                hashdb::hashes_response_md5_t& response) {
    switch(query_type) {
      case QUERY_USE_PATH:
        return query_by_path->query_hashes_md5(request, response);
      case QUERY_USE_SOCKET:
        return query_by_socket->query_hashes_md5(request, response);
      default:
        // non-valid source type was provided
        std::cerr << "Error on query hashes md5: A valid query service type is required.\n";
        return -1;
    }
}
 
int hashdb::query_t::query_sources_md5(
                const hashdb::sources_request_md5_t& request,
                hashdb::sources_response_md5_t& response) {
    switch(query_type) {
      case QUERY_USE_PATH:
        return query_by_path->query_sources_md5(request, response);
      case QUERY_USE_SOCKET:
        return query_by_socket->query_sources_md5(request, response);
      default:
        // non-valid source type was provided
        std::cerr << "Error on query sources md5: A valid query service type is required.\n";
        return -1;
    }
}
 
int hashdb::query_t::query_hashdb_info(std::string& hashdb_info_response) {
    switch(query_type) {
      case QUERY_USE_PATH:
        return query_by_path->query_hashdb_info(hashdb_info_response);
      case QUERY_USE_SOCKET:
        return query_by_socket->query_hashdb_info(hashdb_info_response);
      default:
        // non-valid source type was provided
        std::cerr << "Error on query hashdb info: A valid query service type is required.\n";
        return -1;
    }
}
 
