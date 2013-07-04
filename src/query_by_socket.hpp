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
 *
 * The query request consists of two parts: the query type and then,
 * unless not needed, the query request.
 * The response consists of parts for building a response array, as needed.
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

// types of queries that are available
const uint32_t QUERY_INFO = 1;
const uint32_t QUERY_HASHES_MD5 = 2;
const uint32_t QUERY_SOURCES_MD5 = 3;

// ************************************************************
// types used for zmq transit
// ************************************************************
typedef hashdb::hash_request_md5_t zmq_source_response_header_md5_t;

struct zmq_source_reference_t {
  char repository_name_bytes[200];
  char filename_bytes[200];
  uint64_t file_offset;

  zmq_source_reference_t(hashdb::source_reference_t source_reference) :
          repository_name_bytes(), filename_bytes(),
          file_offset(source_reference.file_offset) {
    strncpy(repository_name_bytes, source_reference.repository_name.c_str(), 200);
    repository_name_bytes[200-1] = '\0';
    strncpy(filename_bytes, source_reference.filename.c_str(), 200);
    filename_bytes[200-1] = '\0';
  }
};

typedef std::vector<zmq_source_reference_t> zmq_source_references_t;
  

class zmq_helper_t {
  public:
  /**
   * helper function for cleanly sending one part of a multipart message
   */
  static int send_part(const void* data, size_t size, void* socket, bool is_more) {

    // create zmq message part
    zmq_msg_t part;
    int status = zmq_msg_init_size(&part, size);
    if (status != 0) {
      std::cerr << "send_part.init " << zmq_strerror(status) << "\n";
      zmq_msg_close(&part);
      return status;
    }

    // copy data to zmq message part
    memcpy (zmq_msg_data(&part), data, size); 

    // send part
    int send_size = zmq_msg_send(&part, socket, (is_more) ? ZMQ_SNDMORE : 0);
    if (send_size == -1) {
      std::cerr << "send_part.send " << zmq_strerror(status) << "\n";
      if (status == 0) {
        zmq_msg_close(&part);
        return -1;
      } else {
        zmq_msg_close(&part);
        return status;
      }
    }
    if ((size_t)send_size != size) {
      std::cerr << "send_part.send size " << send_size << ", " << size << "\n";
      zmq_msg_close(&part);
      return -1;
    }

    // release zmq message part
    zmq_msg_close(&part);
    return 0;
  }

  /**
   * Helper function for cleanly receiving one part of a multipart message.
   * This validates that the response is aligned to a data structure.
   * On failure, part is closed.
   */
  static int open_and_receive_part(zmq_msg_t& zmq_part,
                                   void* socket,
                                   size_t structure_size,
                                   size_t& count,
                                   bool& is_more) {

    // initialize zmq_part
    size_t status;
    status = zmq_msg_init(&zmq_part);
    if (status != 0) {
      std::cerr << "open_and_receive_part failed zmq_msg_init\n";
      return status;
    }

    // get the response
    int response_count = zmq_msg_recv(&zmq_part, socket, 0);
    if (response_count == -1) {
      std::cerr << "open_and_receive_part failed zmq_msg_recv " << zmq_strerror(errno) << "\n";
      if (errno == 0) {
        bool status2 __attribute__((unused)) = close_part(zmq_part);
        return -1;
      } else {
        bool status3 __attribute__((unused)) = close_part(zmq_part);
        return errno;
      }
    }

    // validate that the response size is aligned
    if (response_count % structure_size != 0) {
      std::cerr << "open_and_receive_part failed data structure boundary, " << response_count << " does not align with " << structure_size << "\n";
      bool status4 __attribute__((unused)) = close_part(zmq_part);
      return -1;
    }

    // set count and is_more
    count = response_count / structure_size;
    is_more = (zmq_msg_more(&zmq_part) == 1) ? true : false;
    return 0;
  }

  /**
   * Helper function for cleanly receiving one fixed size data structure.
   */
  static int open_and_receive_part(zmq_msg_t& zmq_part, void* socket, size_t structure_size, bool& is_more) {
    size_t count;
    int status = open_and_receive_part(zmq_part, socket, structure_size, count, is_more);
    if (status != 0) {
      bool status2 __attribute__((unused)) = close_part(zmq_part);
      return status;
    }
    // return size must be exactly equal to structure size
    if (count != 1) {
      std::cerr << "open_and_receive_part failed data size, " << zmq_msg_size(&zmq_part) << " is not " << structure_size << "\n";
      return -1;
    }
  }

  /**
   * Helper function to close the zmq_part that open_and_receive_part opened
   */
  static int close_part(zmq_msg_t& zmq_part) {
    // deallocate the message
    int status = zmq_msg_close(&zmq_part);
    if (status != 0) {
      std::cerr << "close_response failed zmq_msg_close " << status << ", " << zmq_strerror(status) << "\n";
      zmq_msg_close(&zmq_part);
      return status;
    }
    return 0;
  }

  private:
  // do not allow these
  zmq_helper_t();
};

/**
 * Provide client hashdb query services using zmq3.
 */
class query_by_socket_t {

  private:
  bool is_valid;

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
   * Create the client hashdb query service using zmq3 and socket endpoint
   * such as "tcp://localhost:14500".
   */
  query_by_socket_t(const std::string& query_source_string) :
                                is_valid(false),
                                M(),
                                context(),
                                socket_endpoint(query_source_string),
                                sockets() {

#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif
 
    // create the context which is thread-safe
    context = zmq_ctx_new();
    if (context == 0) {
      std::cerr << "error: query_by_socket_t failed to create query service client, zmq_ctx_new.\n";
      std::cerr << strerror(errno) << "\n";
      std::cerr << "Query by socket service not activated.\n";
    }
    is_valid = true;
  }

  ~query_by_socket_t() {
    int status;
    is_valid = false;

    // close socket resources
    for (std::map<pthread_t,
         void*>::const_iterator it = sockets.begin();
         it != sockets.end(); ++it) {
      void* socket = it->second;
      status = zmq_close(socket);
      if (status != 0) {
        std::cerr << "query_by_socket_t failed zmq_close\n";
      }
    }

    // destroy the context;
    status = zmq_ctx_destroy(context);
    if (status != 0) {
      std::cerr << "query_by_socket_t failed zmq_ctx_destroy\n";
    }
  }
 
  /**
   * Look up hashes.
   */
  int query_hashes_md5(const hashdb::hashes_request_md5_t& request,
                       hashdb::hashes_response_md5_t& response) {
std::cerr << "query_by_socket.query_hashes_md5.a\n";

    // the query service must be working
    if (!is_valid) {
      return -1;
    }

    // clear any exsting response
    response.clear();

    // already done if there are no request items
    if (request.size() == 0) {
      return 0;
    }

    // get the socket
    void* socket = get_socket();
    if (socket == 0) {
      std::cerr << "query_by_socket_t failed to procure a socket\n";
      return -1;
    }

    int status;

    // send the hash query request type
    status = zmq_helper_t::send_part(
                       &QUERY_HASHES_MD5, sizeof(uint32_t), socket, true);
    if (status != 0) {
      return status;
    }

    // send the hash query request
    status = zmq_helper_t::send_part(
                       &request[0],
                       sizeof(hashdb::hash_request_md5_t) * request.size(),
                       socket,
                       false);
    if (status != 0) {
      return status;
    }

    // get the hash query response
    zmq_msg_t zmq_response;

    size_t hash_response_count;
    bool is_more;
    status = zmq_helper_t::open_and_receive_part(zmq_response,
                                   socket,
                                   sizeof(hashdb::hash_response_md5_t),
                                   hash_response_count,
                                   is_more);
    if (status != 0) {
      return status;
    }
    if (is_more) {
      std::cerr << "query_hashes_md5 unexpected more data\n";
      bool status2 __attribute__((unused)) = zmq_helper_t::close_part(zmq_response);
      return -1;
    }
    
    hashdb::hash_response_md5_t* response_array = reinterpret_cast<hashdb::hash_response_md5_t*>(zmq_msg_data(&zmq_response));

    // copy response array to response vector
    for (size_t i = 0; i<hash_response_count; i++) {
      response.push_back(response_array[i]);
    }

    // close hash query response
    status = zmq_helper_t::close_part(zmq_response);
    if (status != 0) {
      return status;
    }

    return 0;
  }

  /**
   * Look up sources.
   */
  int query_sources_md5(const hashdb::sources_request_md5_t& request,
                        hashdb::sources_response_md5_t& response) {

    // the query service must be working
    if (!is_valid) {
      return -1;
    }

    // clear any exsting response
    response.clear();

    // get the socket
    void* socket = get_socket();
    if (socket == 0) {
      std::cerr << "query_sources_md5 failed to procure a socket\n";
      return -1;
    }

    int status;

    // send the source query request type
    status = zmq_helper_t::send_part(&QUERY_SOURCES_MD5, sizeof(uint32_t), socket, true);
    if (status != 0) {
      return status;
    }

    // send the hash query request
    status = zmq_helper_t::send_part(&request[0],
                       sizeof(hashdb::source_request_md5_t) * request.size(),
                       socket,
                       false);
    if (status != 0) {
      return status;
    }

    // receive pairs of source_request and source_references
    while (true) {
      // get the source_request response
      zmq_msg_t zmq_source_request;
      size_t count;
      bool is_more;
      status = zmq_helper_t::open_and_receive_part(zmq_source_request,
                                     socket,
                                     sizeof(hashdb::hash_request_md5_t),
                                     count,
                                     is_more);
      if (status != 0) {
        return status;
      }

      // done when count is 0 and no more parts
      if (count == 0 && !is_more) {
        status = zmq_helper_t::close_part(zmq_source_request);
        if (status != 0) {
          return status;
        }
        return 0;
      }

      if ((count != 1) || (!is_more)) {
        std::cerr << "query_sources_md5 receive hash request requires more data.\n";
        bool status2 __attribute__((unused)) = zmq_helper_t::close_part(zmq_source_request);
        return -1;
      }

      // get address of source request
      hashdb::source_request_md5_t* source_request_md5 = reinterpret_cast<hashdb::source_request_md5_t*>(zmq_msg_data(&zmq_source_request));

      // push a source response record onto the respnse vector
      response.push_back(hashdb::source_response_md5_t(source_request_md5->id,
                                               source_request_md5->digest));

      // get the source response in order to add source references to it
      hashdb::source_response_md5_t* source_response = &response.back();

      // close zmq_source_request
      status = zmq_helper_t::close_part(zmq_source_request);
      if (status != 0) {
        return status;
      }

      // get the zmq source_references
      zmq_msg_t zmq_source_references_request;
      size_t zmq_source_references_count;
      status = zmq_helper_t::open_and_receive_part(zmq_source_references_request,
                                     socket,
                                     sizeof(zmq_source_reference_t),
                                     zmq_source_references_count,
                                     is_more);
      if (status != 0) {
        return status;
      }
      if (!is_more) {
        std::cerr << "query_sources_md5 receive hash request expects more data.\n";
        return -1;
      }

      // get address of source references request
      zmq_source_reference_t* zmq_source_reference_array = reinterpret_cast<zmq_source_reference_t*>(zmq_msg_data(&zmq_source_references_request));

      // copy each source reference into source response
      for (size_t i=0; i<zmq_source_references_count; i++) {
        source_response->source_references.push_back(hashdb::source_reference_t(
                std::string(zmq_source_reference_array[i].repository_name_bytes),
                std::string(zmq_source_reference_array[i].filename_bytes),
                zmq_source_reference_array[i].file_offset));
      }

      // close zmq_source_references_request
      status = zmq_helper_t::close_part(zmq_source_references_request);

      if (status != 0) {
        return status;
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

    info = "info currently not available";
    return 0;
  }

  private:
  // ************************************************************
  // private thread-safe socket management
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


};

#endif

