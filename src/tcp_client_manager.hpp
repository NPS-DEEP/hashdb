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
 * Provides the client hashdb query service using Boost.Asio.
 * Modeled after the Boost blocking_tcp_echo_client.cpp example.
 */

#ifndef TCP_CLIENT_MANAGER_HPP
#define TCP_CLIENT_MANAGER_HPP

#include <config.h>
#include "hashdb.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "mutex_lock.hpp" // define MUTEX_LOCK

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

// types of queries that are available
const uint32_t QUERY_MD5 = 1;
const uint32_t QUERY_SHA1 = 2;
const uint32_t QUERY_SHA256 = 3;

class tcp_client_manager_t {
  private:
//  typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr_t;
  typedef boost::asio::ip::tcp::socket* socket_ptr_t;

  boost::asio::io_service io_service;
  boost::asio::ip::tcp::resolver::iterator resolver_iterator;
  std::map<pthread_t, socket_ptr_t> sockets;
#ifdef HAVE_PTHREAD
  pthread_mutex_t M;                  // mutext
#else
  int M;                              // placeholder
#endif

  // ************************************************************
  // thread-safe socket acquisition
  // ************************************************************
  socket_ptr_t get_socket() {
    pthread_t this_pthread = pthread_self();

    MUTEX_LOCK(&M);

    // find and return the socket assocated with this pthread
    if (sockets.find(this_pthread) != sockets.end()) {
      MUTEX_UNLOCK(&M);
      return sockets[this_pthread];
    }

    // create the socket for this pthread
//    socket_ptr_t socket_ptr(new boost::asio::ip::tcp::socket(io_service));
    socket_ptr_t socket_ptr(new boost::asio::ip::tcp::socket(io_service));

    // connect this socket
    boost::asio::connect(*socket_ptr, resolver_iterator);

    // add the socket prepared for this thread to the sockets map
    sockets[this_pthread] = socket_ptr;

    // return the freshly created socket
    MUTEX_UNLOCK(&M);
    return socket_ptr;
  }

  // helper: get the resolver iterator
  static boost::asio::ip::tcp::resolver::iterator get_resolver_iterator(
                                boost::asio::io_service* io_service,
                                const std::string& socket_string) {
    // validate input
    // take socket_string in form "tcp://<host>:<port>"
    if (socket_string.find("tcp://") != 0) {
      // unusable socket_string syntax
      throw std::runtime_error("Error: invalid socket syntax, 'tcp://' expected");
    }

    // parse host and port
    std::string host_port_string = socket_string.substr(6, std::string::npos-1);
    size_t colon_position = host_port_string.find(':');
    if (colon_position == std::string::npos) {
      // unusable socket_string syntax
      throw std::runtime_error("Error: invalid socket syntax, ':' expected");
    }
    std::string host_string = host_port_string.substr(0, colon_position);
    std::string port_string = host_port_string.substr(colon_position+1);

//std::cout << "tcp_client_manager.i host_string '" << host_string << "', port_string '" << port_string << "'\n";

    // return valid resolver iterator
    boost::asio::ip::tcp::resolver resolver(*io_service);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host_string, port_string);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    return iterator;
  }

  public:
  tcp_client_manager_t(const std::string& socket_string) :
        io_service(),
        resolver_iterator(get_resolver_iterator(&io_service, socket_string)),
        sockets(),
        M() {
#ifdef HAVE_PTHREAD
    pthread_mutex_init(&M,NULL);
#endif
  }

  ~tcp_client_manager_t() {
    // close sockets
    for (std::map<pthread_t, socket_ptr_t>::const_iterator it = sockets.begin();
         it != sockets.end(); ++it) {
      socket_ptr_t socket_ptr = it->second;

      socket_ptr->close();
      delete socket_ptr;
//      delete *it;
    }
  }

  // scan
  template<typename T, int RT>
  int scan(const T& request, hashdb_t::scan_output_t& response) {

    // clear any exsting response
    response.clear();

    // nothing to do if there are no request items
    if (request.size() == 0) {
      return 0;
    }

    try {

      // get, possibly establishing, the connnected socket for this thread
      socket_ptr_t socket = get_socket();

      // write the hashdigest type to use
      uint32_t hashdigest_type = RT;
      boost::asio::write(
           *socket, boost::asio::buffer(&hashdigest_type,
                                        sizeof(hashdigest_type)));


      // write the request_count
      uint32_t request_count = request.size();
      boost::asio::write(
           *socket,
           boost::asio::buffer(&request_count, sizeof(request_count)));

      // write the request vector
      boost::asio::write(
           *socket, boost::asio::buffer(request));

      // read the response_count
      uint32_t response_count;
      boost::asio::read(
           *socket,
           boost::asio::buffer(&response_count, sizeof(request_count)));

      // allocate the response vector with the expected size
      response = hashdb_t::scan_output_t(response_count);

      // read the response vector
      boost::asio::read(*socket, boost::asio::buffer(response));

      // good, return 0
      return 0;

    } catch (std::exception& e) {
      // bad, return -1
      std::cerr << "Exception in socket scan request, request dropped: "
                << e.what() << "\n";
      return -1;
    }
  }
};

#endif

