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
 * Provides client hashdb query service interfaces for using a socket ZMQ
 * server service.
 *
 * This interface is threadsafe because it manages zmq in a threadsafe way.
 */

#ifndef QUERY_BY_SOCKET_HPP
#define QUERY_BY_SOCKET_HPP

#include <config.h>
#include "hashdb_types.h"
#include "hashdb_settings.h"
#include "hashdb_db_manager.hpp"
#include "hashdb.hpp"
#include <map>
#include <zmq.h>


#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_PTHREAD
#define MUTEX_LOCK(M)   pthread_mutex_lock(M)
#define MUTEX_UNLOCK(M) pthread_mutex_unlock(M)
#else
#define MUTEX_LOCK(M)   {}
#define MUTEX_UNLOCK(M) {}
#endif

#ifdef HAVE_PTHREAD_H
  #include <pthread.h> // zmq.h includes this too
  #ifdef WIN32
    // help ptw32_handle_t because in the WIN32 configuration std::map
    // needs operator<
    inline bool operator< (const ptw32_handle_t& lh, const ptw32_handle_t& rh) {
      return lh.p < rh.p;
    }
  #endif
#endif

// ************************************************************
// types of fixed size for ZMQ
// ************************************************************
enum query_type_t {NOT_DEFINED,
                   QUERY_INFO,
                   QUERY_HASHES_MD5,
                   QUERY_DUPLICATE_HASHES_MD5,
                   QUERY_SOURCE_MD5};

static const size_t MAX_HASHES = 10000;

class zmq_hashes_request_md5_t {
  private:
  const query_type_t query_type;
  uint32_t hash_count;

  public:
  hashdb::hash_request_md5_t hash_request[MAX_HASHES];

  zmq_hashes_request_md5_t() : query_type(QUERY_HASHES_MD5), hash_count(0) {
  }

  uint32_t byte_size() const {
    return sizeof(query_type_t) + sizeof(uint32_t) + hash_count * sizeof(hashdb::hash_request_md5_t);
  }

  void add(const hashdb::hash_request_md5_t& p_hash_request) {
    if (hash_count >= MAX_HASHES) {
      // the caller is responsible for checking size
      assert(0);
    }
    hash_request[hash_count++] = p_hash_request;
  }

  size_t count() const {
    return hash_count;
  }

  void clear() {
    hash_count = 0;
  }

  bool is_full() const {
    return (hash_count >= MAX_HASHES);
  }
};

class zmq_hashes_response_md5_t {
  public:
  const size_t chunk_size;

  private:
  uint32_t hash_count;

  public:
  hashdb::hash_response_md5_t hash_response[MAX_HASHES];

  zmq_hashes_response_md5_t(size_t p_chunk_size) :
                              chunk_size(p_chunk_size), hash_count(0) {
  }
  zmq_hashes_response_md5_t() : chunk_size(0), hash_count(0) {
  }

  size_t byte_size() const {
    return sizeof(size_t) + sizeof(uint32_t) + hash_count * sizeof(hashdb::hash_response_md5_t);
  }

  void add(const hashdb::hash_response_md5_t& p_hash_response) {
    if (hash_count >= MAX_HASHES) {
      // the caller is responsible for checking size
      assert(0);
    }
    hash_response[hash_count++] = p_hash_response;
  }

  uint32_t count() const {
    return hash_count;
  }

  void clear() {
    hash_count = 0;
  }

  bool is_full() const {
    return (hash_count >= MAX_HASHES);
  }
};

// generic structure
struct zmq_generic_request_t {
  query_type_t query_type;
  // bytes of largest request structure
  uint8_t unused[sizeof(zmq_hashes_request_md5_t) - sizeof(query_type_t)];
  zmq_generic_request_t() : query_type(NOT_DEFINED) {
  }
};

// ************************************************************
// query by socket class
// ************************************************************
/**
 * Provide client hashdb query services using zmq3.
 */
class query_by_socket_t {

  private:

#ifdef HAVE_PTHREAD
    pthread_mutex_t M;                  // mutext protecting out
#else
    int M;                              // placeholder
#endif

  // zmq state
  void* context;
  std::string socket_endpoint;
  std::map<pthread_t, void*> sockets;

  // do not allow these
  query_by_socket_t(const query_by_socket_t&);
  query_by_socket_t& operator=(const query_by_socket_t&);

  // ************************************************************
  // private zmq helpers
  // ************************************************************
  void* get_socket() {
    pthread_t this_pthread = pthread_self();

    MUTEX_LOCK(&M);

    // find and return the socket assocated with this pthread
    if (sockets.find(this_pthread) != sockets.end()) {
      MUTEX_UNLOCK(&M);
      return sockets[this_pthread];
    }

    // create the socket for this pthread
    void* socket = zmq_socket(context, ZMQ_REQ);
    if (socket == 0) {
      std::cerr << "error: query_by_socket_t failed to create hashdb query service client, zmq_socket.\n";
      std::cerr << strerror(errno) << "\n";

      // socket allocation failed
      MUTEX_UNLOCK(&M);
      return 0;
    }

    // connect the socket
    int status;
    status = zmq_connect(socket, socket_endpoint.c_str());
    if (status != 0) {
      std::cerr << "Error: " << strerror(errno) << "\n"
                << "Socket endpoint '" << socket_endpoint
                << "' failed to connect.\n"
                << "Please check that the socket endpoint is valid.\n"
                << "An example socket endpoint is 'tcp://localhost:14500'.\n";

      // if connect fails then close the socket
      status = zmq_close(socket);
      if (status != 0) {
        std::cerr << "query_by_socket_t failed zmq_close after failing to connect\n";
      }

      // socket connection failed
      MUTEX_UNLOCK(&M);
      return 0;
    }

    // add the allocated and connected socket for this pthread
    sockets[this_pthread] = socket;

    // return the freshly created socket
    MUTEX_UNLOCK(&M);
    return socket;
  }

  // perform zmq transaction.
  // Returns true if the transaction works.
  // Does not validate that the response is complete.
  template<typename T_request, typename T_response>
  bool transact(const T_request& request,
                size_t request_size,
                T_response& response,
                int& response_size) {
//std::cout << "transact request byte size " << request.byte_size() << "\n\n";
//std::cout << "transact request size " << request_size << "\n";

    // get the socket
    void* socket = get_socket();
    if (socket == 0) {
      return false;
    }

    // send request
    int sent_size = zmq_send(socket, &request, request_size, 0);
//std::cout << "transact.b\n";

    // validate request
    if (sent_size == -1) {
      std::cerr << "zmq send failure.  zmq error: " << strerror(errno) << "\n";
      return false;
    }
//std::cout << "transact.c\n";
    if ((size_t)sent_size != request_size) {
      std::cerr << "zmq send failure.  "
                << request_size << " bytes expected but "
                << sent_size << " bytes were sent.\n";
    }
//std::cout << "transact.d\n";

    // receive response
    response_size = zmq_recv(socket, &response, sizeof(T_response), 0);
//std::cout << "transact response " << response.byte_size() << "\n\n";
//std::cout << "transact response size " << response_size << "\n";
    return true;
  }

  bool validate_response_size(size_t expected_size, int response_size) {
    // validate response code
    if (response_size == -1) {
      std::cerr << "zmq receive failure.  zmq error: " << strerror(errno) << "\n";
      return false;
    }

    // validate response size
    if (expected_size == (size_t)response_size) {
      return true;
    } else {
      std::cerr << "zmq response failure.  "
                << expected_size << " bytes expected but "
                << response_size << " bytes were received.\n";
      return false;
    }
  }

  public:

  /**
   * Create the client hashdb query service using zmq3 and socket endpoint
   * such as "tcp://localhost:14500".
   */
  query_by_socket_t(const std::string& lookup_source_string) :
                                M(),
                                context(),
                                socket_endpoint(lookup_source_string),
                                sockets() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif
 
    // create the context which is thread-safe
    context = zmq_ctx_new();
    if (context == 0) {
      std::cerr << "error: query_by_socket_t failed to create query service client, zmq_ctx_new.\n";
      std::cerr << strerror(errno) << "\n";
      exit(1);
    }
  }

  ~query_by_socket_t() {

    // close socket resources
    for (std::map<pthread_t,
         void*>::const_iterator it = sockets.begin();
         it != sockets.end(); ++it) {
      int status;
      void* socket = it->second;
      status = zmq_close(socket);
      if (status != 0) {
        std::cerr << "query_by_socket_t failed zmq_close\n";
      }
    }

    // destroy the context;
    int status = zmq_ctx_destroy(context);
    if (status != 0) {
      std::cerr << "query_by_socket_t failed zmq_ctx_destroy\n";
    }
  }
 
  /**
   * Look up hashes.
   */
  bool lookup_hashes_md5(const hashdb::hashes_request_md5_t& request,
                         hashdb::hashes_response_md5_t& response) {

    // allocate big space on heap for zmq request and response
    zmq_hashes_request_md5_t* zmq_request = new zmq_hashes_request_md5_t;
    zmq_hashes_response_md5_t* zmq_response = new zmq_hashes_response_md5_t;

    bool success = true;

    // clear existing response
    response.chunk_size = 0;
    response.clear();

    // copy request to zmq request, flushing when array is full
    for (std::vector<hashdb::hash_request_md5_t>::const_iterator it = request.hash_requests.begin(); it != request.hash_requests.end(); ++it) {
      zmq_request->add(*it);
      if (zmq_request->is_full()) {
        success = zmq_lookup_hashes_md5(zmq_request, zmq_response);
        if (success == true) {
          copy_zmq_response(zmq_response, response);
        } else {
          response.clear();
          break;
        }
        zmq_request->clear();
      }
    }
    if (success == true) {
      success = zmq_lookup_hashes_md5(zmq_request, zmq_response);
    }
    if (success == true) {
      copy_zmq_response(zmq_response, response);
    } else {
      response.clear();
    }

    // deallocate big space on heap for zmq request and response
    delete zmq_request;
    delete zmq_response;

    return success;
  }

  /**
   * Request information about the hashdb.
   */
  bool get_hashdb_info(std::string& info) {
    info = "info currently not available";
    return true;
  }

private:
  bool zmq_lookup_hashes_md5(const zmq_hashes_request_md5_t* zmq_request,
                             zmq_hashes_response_md5_t* zmq_response) {

    // perform the lookup
    int response_size;
    bool success = transact(*zmq_request,
                            zmq_request->byte_size(),
                            *zmq_response,
                            response_size);

    // transaction status and response size must both be okay
    return (success && validate_response_size(zmq_response->byte_size(), response_size));
  }

  void copy_zmq_response(const zmq_hashes_response_md5_t* zmq_response,
                         hashdb::hashes_response_md5_t& response) {

    // record chunk_size from response
    response.chunk_size = zmq_response->chunk_size;

    // record each feature in the response
    for (size_t i=0; i<zmq_response->count(); i++) {
      response.hash_responses.push_back(zmq_response->hash_response[i]);
    }
  }
};

#endif

