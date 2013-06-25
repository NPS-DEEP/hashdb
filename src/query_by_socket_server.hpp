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
 */
class query_by_socket_server_t {
//  static const char* socket_endpoint = "inproc://zmq_query_service";
//  const std::string socket_endpoint;	// such as "tcp://*:14500"

  private:
  const hashdb_db_manager_t* hashdb_db_manager;
  void* context;
  void* socket;
  zmq_generic_request_t zmq_generic_request;
  zmq_hashes_response_md5_t zmq_hashes_response;

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
           socket(0),
           zmq_generic_request(),
           zmq_hashes_response() {
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
   */
  void process_request() {
    // receive request into query record
    int count = zmq_recv(socket, &zmq_generic_request, sizeof(zmq_generic_request_t), 0);
    if (count == -1) {
      // bad
      std::cerr << "query_by_socket_server_t process_request failure: unable to receive request: " << count << "\n";
      std::cerr << strerror(errno) << "\n";
      return;
    }

    // act based on type of request
    switch(zmq_generic_request.query_type) {
      case QUERY_INFO: process_get_info(); break;
      case QUERY_HASHES_MD5: process_query_hashes_md5(); break;
      default: {
        // program error or zmq data failure
        std::cerr << "query_by_socket_server_t process_request failure: an invalid query type was received.  Request ignored.\n";
        break;
      }
    }
  }

  private:
  void process_get_info() {
    send_response(hashdb_db_manager->hashdb_settings,
                  sizeof(hashdb_db_manager->hashdb_settings));
  }

  void process_query_hashes_md5() {

    // make sure the hashdb is using MD5
    if (hashdb_db_manager->hashdb_settings.hashdigest_type != HASHDIGEST_MD5) {
      std::cerr << "The hashdigest type requested by the client is invalid.\n";
      return;
    }

    // typecast pointer from generic request to query hashes request
    zmq_hashes_request_md5_t const* zmq_request =
            reinterpret_cast<zmq_hashes_request_md5_t const*>(&zmq_generic_request);

    // validate input count
    if (zmq_request->count() > MAX_HASHES) {
      std::cerr << "query_server invalid hash count "
                << zmq_request->count() << "\n";
      return;
    }

    // perform lookups for each hash
    source_lookup_record_t source_lookup_record;
    zmq_hashes_response.clear();
    for (size_t i=0; i!=zmq_request->count(); i++) {
      // get digest into md5
      const uint8_t* digest = zmq_request->hash_request[i].digest;
      md5_t md5;
      memcpy(md5.digest, digest, 16);

      // perform lookup
      bool has_record = hashdb_db_manager->has_source_lookup_record(
                                    md5,
                                    source_lookup_record);

      if (has_record) {
        // get values associated with this hash
        uint64_t id = zmq_request->hash_request[i].id;
        uint32_t count = source_lookup_record.get_count();
        uint64_t source_lookup_index = (count == 1) ?
                     source_lookup_record.source_lookup_index(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;
        uint64_t chunk_offset_value = (count == 1) ?
                     source_lookup_record.chunk_offset_value(hashdb_db_manager->hashdb_settings.source_lookup_settings.number_of_index_bits_type) : 0L;

        // construct hash response
        hashdb::hash_response_md5_t hash_response(id, digest, count, source_lookup_index, chunk_offset_value);
        zmq_hashes_response.add(hash_response);
      }
    }

    send_response(zmq_hashes_response, zmq_hashes_response.byte_size());
  }

  // send the class provided as the response
  template<typename T_response>
  void send_response(const T_response& response, size_t response_size) {

    // send response, validating
    int count = zmq_send(socket, &response, response_size, 0);

    // validate transaction
    if (count == -1) {
      std::cerr << "zmq response failure.  zmq error: " << strerror(errno) << "\n";
      return;
    }

    if ((size_t)count != response_size) {
      std::cerr << "zmq response failure.\n";
      std::cerr << "count: " << count << ", expected " << response_size << "\n";
      return;
    }
  }
};

#endif

