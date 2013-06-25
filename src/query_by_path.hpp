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
#include "hashdb_settings.h"
#include "hashdb_db_manager.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdb.hpp"

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
  bool query_source_is_valid() const {
    return is_valid;
  }

  /**
   * Create the client hashdb query service using a file path.
   */
  query_by_path_t(const std::string& lookup_source_string) :
                                is_valid(false), hashdb_db_manager(0) {

    // perform setup by opening a hashdb

    // make sure the hashdb path is available
    bool is_present = (access(lookup_source_string.c_str(),F_OK) == 0);
    if (!is_present) {
      std::cerr << "Error: hashdb directory path '" << lookup_source_string << "' is invalid.\n"
              << "Query by path service not activated.\n";
    }

    // open the hashdb
    std::cout << "Opening hashdb '" << lookup_source_string << "' ...\n";
    hashdb_db_manager = new hashdb_db_manager_t(lookup_source_string, READ_ONLY);
    is_valid = true;
    std::cout << "hashdb opened.\n";

  }

  ~query_by_path_t() {
    // close the hashdb resource
    is_valid = false;
    std::cout << "hashdb closed.\n";
  }
 
  /**
   * Look up hashes.
   */
  bool lookup_hashes_md5(const hashdb::hashes_request_md5_t& request,
                         hashdb::hashes_response_md5_t& response) {

    // the query service must be working
    if (!is_valid) {
      return false;
    }

    // make sure the hashdb is using MD5
    if (hashdb_db_manager->hashdb_settings.hashdigest_type != HASHDIGEST_MD5) {
      std::cerr << "The hashdigest type is invalid.\n";
      return false;
    }

    // perform lookups for each hash
    source_lookup_record_t source_lookup_record;
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
        uint32_t count = source_lookup_record.get_count();
        uint64_t source_lookup_index = (count == 1) ?
                 source_lookup_record.source_lookup_index(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;
        uint64_t chunk_offset_value = (count == 1) ?
                 source_lookup_record.chunk_offset_value(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;

        // construct hash response
        hashdb::hash_response_md5_t hash_response(it->id,
                                                  it->digest,
                                                  count,
                                                  source_lookup_index,
                                                  chunk_offset_value);
        response.push_back(hash_response);
      }
    }

    return true;
  }

  /**
   * Look up sources.
   */
  bool lookup_sources_md5(const hashdb::sources_request_md5_t& sources_request,
                          hashdb::sources_response_md5_t& sources_response) {

    // note: this implemenatation can be optimized but it will require better
    //       interfaces in hashdb_db_manager_t

    // the query service must be working
    if (!is_valid) {
      return false;
    }

    // make sure the hashdb is using MD5
    if (hashdb_db_manager->hashdb_settings.hashdigest_type != HASHDIGEST_MD5) {
      std::cerr << "The hashdigest type is invalid.\n";
      return false;
    }

    std::vector<hash_source_record_t> hash_source_records;
    sources_response.clear();

    // perform lookups for each source
    for (hashdb::sources_request_md5_t::const_iterator it = sources_request.begin(); it != sources_request.end(); ++it) {
      
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
      hashdb::source_response_md5_t source_response(it->id, it->digest);
      for (std::vector<hash_source_record_t>::const_iterator hash_source_record_it = hash_source_records.begin(); hash_source_record_it != hash_source_records.end(); ++hash_source_record_it) {
        source_response.source_references.push_back(hashdb::source_reference_t(
                              hash_source_record_it->repository_name,
                              hash_source_record_it->filename,
                              hash_source_record_it->file_offset));
      }

      // push source response for this hash
      sources_response.push_back(source_response);
    }

    return true;
  }



  /**
   * Request information about the hashdb.
   */
  bool get_hashdb_info(std::string& info) {

    // the query service must be working
    if (!is_valid) {
      return false;
    }

    info = "currently not available";
    return true;
  }
};

#endif

