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
 * Provides client interfaces for accessing a hashdb mounted from a
 * filesystem path.
 */

#ifndef QUERY_BY_PATH_HPP
#define QUERY_BY_PATH_HPP

#include "hashdb_types.h"
#include "hashdb_settings.hpp"
#include "hashdb_db_manager.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdb.hpp"
#include "query_by_socket_server.hpp"

// Standard includes
//#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>

/**
 * Provide client hashdb query services using a file path.
 */
class query_by_path_t {

  private:

  // hashdb state
  bool is_valid;
  hashdb_db_manager_t* hashdb_db_manager;

  // do not allow these
  query_by_path_t(const query_by_path_t&);
  query_by_path_t& operator=(const query_by_path_t&);

  public:

  // the query source is valid
  int query_status() const {
    if (is_valid) {
      return 0;
    } else {
      return -1;
    }
  }

  /**
   * Create the client hashdb query service using a file path.
   */
  query_by_path_t(const std::string& query_source_string) :
                                is_valid(false), hashdb_db_manager(0) {

    // make sure the hashdb path is available
    bool is_present = (access(query_source_string.c_str(),F_OK) == 0);
    if (!is_present) {
      std::cerr << "Error: hashdb directory path '" << query_source_string << "' is invalid.\n"
              << "Query by path service not activated.\n";
      return;
    }

    // open the hashdb
    is_valid = true;
    std::cout << "Opening hashdb '" << query_source_string << "' for query by path ...\n";
    hashdb_db_manager = new hashdb_db_manager_t(query_source_string, READ_ONLY);
    std::cout << "hashdb opened.\n";
  }

  ~query_by_path_t() {
    if (is_valid) {
      // close the hashdb resource
      delete hashdb_db_manager;
      is_valid = false;
      std::cout << "hashdb closed.\n";
    }
  }
 
  /**
   * Look up hashes.
   */
  int query_hashes_md5(const hashdb::hashes_request_md5_t& request,
                       hashdb::hashes_response_md5_t& response) {

    // the query service must be working
    if (!is_valid) {
      return -1;
    }

    // make sure the hashdb is using MD5
    if (hashdb_db_manager->hashdb_settings.hashdigest_type != HASHDIGEST_MD5) {
      std::cerr << "The hashdigest type is invalid.\n";
      return -2;
    }

    // perform query for each hash
    uint64_t source_lookup_record;
    response.clear();
    for (std::vector<hashdb::hash_request_md5_t>::const_iterator it = request.begin(); it != request.end(); ++it) {
      
//      md5_t md5 = it->digest;
      const uint8_t* digest = it->digest;
      md5_t md5;
      memcpy(md5.digest, digest, 16);

      bool has_record = hashdb_db_manager->has_source_lookup_record(
                                                   md5, source_lookup_record);

      if (has_record) {
        // get values associated with this hash
        uint32_t count = source_lookup_encoding::get_count(source_lookup_record);
        uint64_t source_lookup_index = (count == 1) ?
                 source_lookup_encoding::get_source_lookup_index(hashdb_db_manager->hashdb_settings.number_of_index_bits, source_lookup_record) : 0L;
        uint64_t hash_block_offset_value = (count == 1) ?
                 source_lookup_encoding::get_hash_block_offset(hashdb_db_manager->hashdb_settings.number_of_index_bits, source_lookup_record) : 0L;

        // construct hash response
        hashdb::hash_response_md5_t hash_response(it->id,
                                                  it->digest,
                                                  count,
                                                  source_lookup_index,
                                                  hash_block_offset_value);
        response.push_back(hash_response);
      }
    }

    return 0;
  }

  /**
   * Look up sources.
   */
  int query_sources_md5(const hashdb::sources_request_md5_t& request,
                        hashdb::sources_response_md5_t& response) {

    // note: this implemenatation can be optimized but it will require better
    //       interfaces in hashdb_db_manager_t

    // the query service must be working
    if (!is_valid) {
      return -1;
    }

    // make sure the hashdb is using MD5
    if (hashdb_db_manager->hashdb_settings.hashdigest_type != HASHDIGEST_MD5) {
      std::cerr << "The hashdigest type is invalid.\n";
      return -2;
    }

    std::vector<hash_source_record_t> hash_source_records;
    response.clear();

    // perform query for each source
    for (hashdb::sources_request_md5_t::const_iterator it = request.begin(); it != request.end(); ++it) {
      
//      md5_t md5 = it->digest;
      const uint8_t* digest = it->digest;
      md5_t md5;
      memcpy(md5.digest, digest, 16);

      bool has_records = hashdb_db_manager->get_hash_source_records(
                                                   md5, hash_source_records);

      // skip if hash is not there
      if (!has_records) {
        continue;
      }

      // build source response for this hash
      response.push_back(hashdb::source_response_md5_t(it->id, it->digest));

      // get address of source response
      hashdb::source_response_md5_t* source_response = &response.back();

      // fill in source reference array of source response
      for (std::vector<hash_source_record_t>::const_iterator hash_source_record_it = hash_source_records.begin(); hash_source_record_it != hash_source_records.end(); ++hash_source_record_it) {
        source_response->source_references.push_back(hashdb::source_reference_t(
                              hash_source_record_it->repository_name,
                              hash_source_record_it->filename,
                              hash_source_record_it->file_offset));
      }
    }

    return 0;
  }

  /**
   * Request information about the hashdb.
   */
  int query_hashdb_info(std::string& info) {

    // the query service must be working
    if (!is_valid) {
      return -1;
    }

    int status = hashdb_db_info_provider_t::get_hashdb_info(
                     hashdb_db_manager->hashdb_dir, info);
    return status;
  }
};

#endif

