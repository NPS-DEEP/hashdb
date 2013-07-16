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
 * Provides a server interface service for a hashdb.
 */

#ifndef QUERY_BY_SOCKET_SERVER
#define QUERY_BY_SOCKET_SERVER
#include "hashdb_types.h"
#include "query_by_socket.hpp"
//#ifdef WIN32
//  // including winsock2.h keeps windows.h from complaining in zmq.h
//  #include <winsock2.h>
//#endif
#include <zmq.h>
#include "dfxml/src/hash_t.h"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

/**
 * Provide server hashdb query service.
 *
 * Design philosophy for failure conditions:
 * May call exit() or return error, depending on failure type.
 * Terminal failures are because of failed system resources such as no memory.
 * Recoverable failures include message loss due to a network failure.
 * Since zmq manages multipart message groups as atomic, a missing close flag
 * is considered terminal.
 */
class query_by_socket_server_t {
//  static const char* socket_endpoint = "inproc://zmq_query_service";
//  const std::string socket_endpoint;	// such as "tcp://*:14500"

  private:
  const hashdb_db_manager_t* hashdb_db_manager;
  void* context;
  void* socket;

  // do not allow these
  query_by_socket_server_t(const query_by_socket_server_t&);
  query_by_socket_server_t& operator=(const query_by_socket_server_t&);

  public:
  /**
   * Create the server query service.
   */
  query_by_socket_server_t(const hashdb_db_manager_t* p_hashdb_db_manager,
                        const std::string& socket_endpoint) :
           hashdb_db_manager(p_hashdb_db_manager),
           context(0),
           socket(0) {
    int status;
    context = zmq_ctx_new();
    if (context == 0) {
      std::cerr << "error: hashdb socket server failed to create hashdb query service server, zmq_ctx_new.\n";
      std::cerr << strerror(errno) << "\n";
      exit(1);
    }
    socket = zmq_socket(context, ZMQ_REP);
    status = zmq_bind(socket, socket_endpoint.c_str());
    if (status != 0) {
      std::cerr << "error: hashdb server failed to connect to socket endpoint '" << socket_endpoint << "'.\n";
      std::cerr << strerror(errno) << "\n";
      exit(1);
    }
  }

  ~query_by_socket_server_t() {
    int status;
    status = zmq_close(socket);
    if (status != 0) {
      std::cerr << "hashdb_query_server failed zmq_close\n";
    }
    status = zmq_ctx_destroy(context);
    if (status != 0) {
      std::cerr << "hashdb_query_server failed zmq_ctx_destroy\n";
    }
  }
    
  /**
   * Receive request from client and provide response.
   * Currently this protocol can't handle difficulty but instead calls exit().
   * Also, buffers are copied; this can be improved for performance.
   * TBD: allow resilience.
   */
  int process_request() {
    zmq_msg_t zmq_request_type;
    int status;
    bool is_more;
    status = zmq_helper_t::open_and_receive_part(zmq_request_type,
                                                 socket,
                                                 sizeof(uint32_t),
                                                 false,
                                                 is_more);

    // read the request type
    uint32_t* request_type_pointer;
    request_type_pointer = reinterpret_cast<uint32_t*>(zmq_msg_data(&zmq_request_type));
    uint32_t request_type;
    request_type = *request_type_pointer;

    // close zmq_request_type
    status = zmq_helper_t::close_part(zmq_request_type);
    if (status != 0) {
      exit(-1);
    }

    // process the request
    if (request_type == QUERY_HASHES_MD5) {
      if (!is_more) {
        std::cerr << "Invalid request: query_by_socket_server hashes more required\n";
        return -1;
      }
      status = process_query_hashes_md5();
    } else if (request_type == QUERY_SOURCES_MD5) {
      if (!is_more) {
        std::cerr << "Invalid request: query_by_socket_server sources more required\n";
        return -1;
      }
      status = process_query_sources_md5();
    } else if (request_type == QUERY_HASHDB_INFO) {
      if (is_more) {
        std::cerr << "Invalid request: query_by_socket_server sources no more required\n";
        return -1;
      }
      status = process_query_hashdb_info();
    }
    return status;
  }

  private:
  int process_query_hashes_md5() {
    int status = 0;
    size_t request_count;
    bool is_more;

    // get the request
    zmq_msg_t zmq_request;
    status = zmq_helper_t::open_and_receive_part(zmq_request,
                                   socket,
                                   sizeof(hashdb::hash_request_md5_t),
                                   false,
                                   request_count,
                                   is_more);
    if (status != 0) {
      exit(-1);
    }
    if (is_more) {
      std::cerr << "Invalid request: query_by_socket_server no more required\n";
      return -1;
    }

    // get address of request
    hashdb::hash_request_md5_t* request_array = reinterpret_cast<hashdb::hash_request_md5_t*>(zmq_msg_data(&zmq_request));

    // create space on the heap for creating the response
    hashdb::hashes_response_md5_t* response = new hashdb::hashes_response_md5_t();

    // perform lookups for each hash in request
    source_lookup_record_t source_lookup_record;
    for (size_t i=0; i<request_count; i++) {
      // get digest into md5
      const uint8_t* digest = request_array[i].digest;
      md5_t md5;
      memcpy(md5.digest, digest, 16);

      // perform lookup
      bool has_record = hashdb_db_manager->has_source_lookup_record(
                                    md5,
                                    source_lookup_record);

      if (has_record) {
        // get values associated with this hash
        uint64_t id = request_array[i].id;
        uint32_t count = source_lookup_record.get_count();
        uint64_t source_lookup_index = (count == 1) ?
                     source_lookup_record.source_lookup_index(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;
        uint64_t chunk_offset_value = (count == 1) ?
                     source_lookup_record.chunk_offset_value(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;

        // construct hash response
        hashdb::hash_response_md5_t hash_response(id, digest, count, source_lookup_index, chunk_offset_value);
        response->push_back(hash_response);
      }
    }

    // send the response
    status = zmq_helper_t::send_part(&((*response)[0]),
                       sizeof(hashdb::hash_response_md5_t) * response->size(),
                       socket,
                       false);
    if (status != 0) {
      exit(-1);
    }

    delete response;

    return 0;
  }

  int send_source_references(const std::vector<hash_source_record_t>& hash_source_records) {

    // allocate zmq_source_references
    zmq_source_references_t* zmq_source_references = new zmq_source_references_t();

    // copy hash_source_records to zmq_source_references
    for (std::vector<hash_source_record_t>::const_iterator it = hash_source_records.begin(); it!= hash_source_records.end(); ++it) {

      zmq_source_references->push_back(zmq_source_reference_t(
                  it->repository_name,
                  it->filename,
                  it->file_offset));
    }

    // send zmq_source_references
    int status = zmq_helper_t::send_part( (&(*zmq_source_references)[0]),
                   sizeof(zmq_source_reference_t) * hash_source_records.size(),
                   socket,
                   true);
    if (status != 0) {
      exit(-1);
    }

    // deallocate zmq_source_references
    delete zmq_source_references;

    return 0;
  }

  int process_query_sources_md5() {
    int status = 0;
    size_t request_count;
    bool is_more;

    // get the request
    zmq_msg_t zmq_request;
    status = zmq_helper_t::open_and_receive_part(zmq_request,
                                   socket,
                                   sizeof(hashdb::source_request_md5_t),
                                   false,
                                   request_count,
                                   is_more);
    if (status != 0) {
      exit(-1);
    }
    if (is_more) {
      std::cerr << "Invalid request: query_by_socket_server no more required for sources\n";
      return -1;
    }
    // get address of request
    hashdb::source_request_md5_t* request_array = reinterpret_cast<hashdb::source_request_md5_t*>(zmq_msg_data(&zmq_request));

    // create space on the heap for hash source records
    std::vector<hash_source_record_t>* hash_source_records =
                                      new std::vector<hash_source_record_t>();

    // perform query for each source in request
    for (size_t i=0; i<request_count; i++) {

      // get digest into md5
      const uint8_t* digest = request_array[i].digest;
      md5_t md5;
      memcpy(md5.digest, digest, 16);

      // perform lookup
      bool has_record = hashdb_db_manager->get_hash_source_records(
                                    md5, *hash_source_records);

      // skip if hash is not there
      if (!has_record) {
        continue;
      }

      // send 1 of 2: hash_request part of response pair
      status = zmq_helper_t::send_part( (&request_array[i]),
                                        sizeof(hashdb::source_request_md5_t),
                                        socket,
                                        true);
      if (status != 0) {
        exit(-1);
      }

      // send 2 of 2: response for this source request
      status = send_source_references(*hash_source_records);
      if (status != 0) {
        exit(-1);
      }
    }

    // free hash_source_records
    delete hash_source_records;

    // send closure signal indicating last part
    status = zmq_helper_t::send_part(0, 0, socket, false);
    if (status != 0) {
      exit(-1);
    }

    return 0;
  }

  int process_query_hashdb_info() {
    int status = 0;

    // create space on the heap for the response
    std::string response;

    // get the query info
    status = hashdb_db_info_provider_t::get_hashdb_info(
                    hashdb_db_manager->hashdb_dir, response);

    if (status != 0) {
      exit(-1);
    }

    // convert to char array to ensure null termination
    char *c=new char[response.size()+1];
    c[response.size()] = '\0';
    memcpy(c, response.c_str(), response.size());

    // send the response
    status = zmq_helper_t::send_part(&c[0],
                       sizeof(char) * response.size(),
                       socket,
                       false);
    if (status != 0) {
      exit(-1);
    }

    delete c;

    return 0;
  }
};

#endif

