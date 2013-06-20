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
 * Header file for the hashdb query interface.
 */

#ifndef HASHDB_HPP
#define HASHDB_HPP

#include <string>
#include <vector>
#include <stdint.h>

class query_by_path_t;
class query_by_socket_t;

/**
 * Version of the hashdb client library.
 */
extern "C"
const char* hashdb_version();

namespace hashdb {

/**
 * Lookup types that are available.
 */
enum lookup_type_t {QUERY_NOT_SELECTED,
                    QUERY_USE_PATH,
                    QUERY_USE_SOCKET};

std::string lookup_type_to_string(lookup_type_t type);
bool string_to_lookup_type(const std::string& name, lookup_type_t& type);

// ************************************************************ 
// data structures supporting lookup_hashes_md5
// ************************************************************ 
/**
 * data associated with one hash in a request
 */
struct hash_request_md5_t {
    uint32_t id;
    uint8_t digest[16];

    hash_request_md5_t();
    hash_request_md5_t(uint32_t id, const uint8_t* p_digest);
};

/**
 * Hash lookup request sent to the query engine
 */
struct hashes_request_md5_t {
    std::vector<hash_request_md5_t> hash_requests;

    hashes_request_md5_t();
};

/**
 * data associated with one hash in a response
 */
struct hash_response_md5_t {
    uint32_t id;
    uint8_t digest[16];
    uint32_t duplicates_count;
    uint64_t source_lookup_index;
    uint64_t chunk_offset_value;

    hash_response_md5_t();
    hash_response_md5_t(uint32_t p_id,
                        const uint8_t* p_digest,
                        uint32_t p_duplicates_count,
                        uint64_t p_source_lookup_index,
                        uint64_t p_chunk_offset_value);
};

/**
 * Hash lookup response returned from the query engine
 */
struct hashes_response_md5_t {
    uint32_t chunk_size;
    std::vector<hash_response_md5_t> hash_responses;

    hashes_response_md5_t();
    void clear();
};

// ************************************************************ 
// data structures supporting lookup_sources_md5
// ************************************************************ 
/**
 * data for source requests
 */
struct source_request_md5_t {
    uint32_t id;
    uint8_t digest[16];

    source_request_md5_t();
    source_request_md5_t(uint32_t id, const uint8_t* p_digest);
};

typedef std::vector<source_request_md5_t> source_requests_md5_t;

/**
 * data for source responses
 */
/*
struct source_record_t {
    uint64_t file_offset;
    std::string repository_name;
    std::string filename;
};

typedef std::vector<source_record_t> source_records_t;

struct source_response_md5_t {
    uint32_t id;
    uint8_t digest[16];
    source_records_t source_records;

    source_resposne_md5_t();
    source_resposne_md5_t(uint32_t id, const uint8_t* p_digest);
};

typedef std::vector<source_response_md5_t> source_responses_t;
*/

class query_t {
    public:
    query_t(lookup_type_t, const std::string&);
    ~query_t();

    bool lookup_hashes_md5(const hashes_request_md5_t& hashes_request,
                           hashes_response_md5_t& hashes_response);

/*
    bool lookup_sources_md5(const sources_request_md5_t& sources_request,
                           sources_response_md5_t& sources_response);
*/

    private:
    lookup_type_t lookup_type;
    query_by_path_t* query_by_path;
    query_by_socket_t* query_by_socket;

    // do not allow these
    query_t(const query_t&);
    query_t& operator=(const query_t&);
};

} // namespace

#endif

