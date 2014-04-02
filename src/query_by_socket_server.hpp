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
#include "query_by_socket.hpp"
#include "hashdb.hpp"
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
//  static const char* socket_string = "inproc://zmq_query_service";
//  const std::string socket_string;	// such as "tcp://*:14500"

  private:
  const hashdb_t hashdb;
  void* context;
  void* socket;

  // do not allow these
  query_by_socket_server_t(const query_by_socket_server_t&);
  query_by_socket_server_t& operator=(const query_by_socket_server_t&);

  public:
  /**
   * The server query service.
   */
  query_by_socket_server_t(const std::string& hashdb_dir,
                           const std::string& socket_string) :
           hashdb(hashdb_dir),
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
    status = zmq_bind(socket, socket_string.c_str());
    if (status != 0) {
      std::cerr << "error: hashdb server failed to connect to socket endpoint '" << socket_string << "'.\n";
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

    // make sure there is more to read
    if (!is_more) {
      std::cerr << "Invalid request: query_by_socket_server, more required\n";
      return -1;
    }

    // process the request to scan
    if (request_type == QUERY_MD5) {
      status = scan<std::pair<uint64_t, md5_t> >(request_type);
    } else if (request_type == QUERY_SHA1) {
      status = scan<std::pair<uint64_t, sha1_t> >(request_type);
    } else if (request_type == QUERY_SHA256) {
      status = scan<std::pair<uint64_t, sha256_t> >(request_type);
    } else {
      std::cerr << "Invalid request type: " << status << "\n";
      return -1;
    }
    return status;
  }

  private:
  template<typename T>
  int scan(uint32_t request_type) {
    int status = 0;
    size_t request_count;
    bool is_more;

    // get the request
    zmq_msg_t zmq_request;
    status = zmq_helper_t::open_and_receive_part(zmq_request,
                                   socket,
                                   sizeof(T),
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

    // create space on the heap for creating the request vector
    std::vector<T>* request = new std::vector<T>();

    // get address of request
    T* request_array = reinterpret_cast<T*>(zmq_msg_data(&zmq_request));

    // copy request array to request vector
    for (size_t i=0; i<request_count; i++) {
      request->push_back(request_array[i]);
    }

    // create space on the heap for creating the response vector
    hashdb_t::scan_output_t* response = new hashdb_t::scan_output_t();

    // perform the scan using hashdb based on request type
    // NOTE: template generalization loses the hash type,
    //       so we unfortunately must deduce hash type.
//    if (request_type == QUERY_MD5) 
    status = hashdb.scan(*request, *response);
    if (status != 0) {
      std::cerr << "query_by_socket_server: Invalid scan, status: " << status << "\n";
      return -1;
    }

    // send the response
    status = zmq_helper_t::send_part(&((*response)[0]),
                       sizeof(T) * response->size(),
                       socket,
                       false);
    if (status != 0) {
      exit(-1);
    }

    delete request;
    delete response;

    return 0;
  }
};

#endif

