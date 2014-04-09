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
 */

#ifndef TCP_CLIENT_MANAGER_HPP
#define TCP_CLIENT_MANAGER_HPP

#include <config.h>
#include "hashdb.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>

// types of queries that are available
const uint32_t QUERY_MD5 = 1;
const uint32_t QUERY_SHA1 = 2;
const uint32_t QUERY_SHA256 = 3;

class tcp_client_manager_t {
  private:
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::resolver::iterator resolver_iterator;

  // take socket_string in form "tcp://<host>:<port>"
  static boost::asio::ip::tcp::resolver::iterator get_resolver_iterator(
                                boost::asio::io_service* io_service,
                                const std::string& socket_string) {
std::cout << "tcp_client_manager.a\n";
    // validate input
    if (socket_string.find("tcp://") != 0) {
      return useless_resolver_iterator(io_service, socket_string);
    }

    // parse host and port
    std::string host_port_string = socket_string.substr(6, std::string::npos-1);
    size_t colon_position = host_port_string.find(':');
    if (colon_position == std::string::npos) {
      return useless_resolver_iterator(io_service, socket_string);
    }
    std::string host_string = host_port_string.substr(0, colon_position);
    std::string port_string = host_port_string.substr(colon_position+1);

std::cout << "tcp_client_manager.i host_string '" << host_string << "', port_string '" << port_string << "'\n";

    // return valid resolver iterator
    boost::asio::ip::tcp::resolver resolver(*io_service);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host_string, port_string);
std::cout << "tcp_client_manager.j\n";
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
std::cout << "tcp_client_manager.k\n";
    return iterator;
  }

  static boost::asio::ip::tcp::resolver::iterator useless_resolver_iterator(boost::asio::io_service* p_io_service, const std::string& socket_string) {
    std::cerr << "invalid socket address: '" << socket_string << "'\n";
    boost::asio::ip::tcp::resolver useless_resolver(*p_io_service);
    boost::asio::ip::tcp::resolver::query useless_query(boost::asio::ip::tcp::v4(), "", "");
      return useless_resolver.resolve(useless_query);
  }

  public:
  tcp_client_manager_t(const std::string& socket_string) :
        io_service(),
        resolver_iterator(get_resolver_iterator(&io_service, socket_string)) {
  }

  // scan
  template<typename T, int RT>
  int scan(const T& request, hashdb_t::scan_output_t& response) {

    // clear any exsting response
    response.clear();

    // already done if there are no request items
    if (request.size() == 0) {
      return 0;
    }

    // establish a socket connection

    // get the socket
    boost::asio::ip::tcp::socket socket(io_service);
    boost::asio::connect(socket, resolver_iterator);

    // write the request_type
    uint32_t request_type = RT;
    boost::system::error_code ec;
    boost::asio::write(
         socket,
         boost::asio::buffer(&request_type, sizeof(request_type)),
         ec);
    if (ec != 0) {
      return -1;
    }

    // write the request_count
    uint32_t request_count = request.size();
    boost::asio::write(
         socket,
         boost::asio::buffer(&request_type, sizeof(request_count)),
         ec);
    if (ec != 0) {
      return -1;
    }

    // write the request array
    boost::asio::write(
         socket, boost::asio::buffer(request), ec);
    if (ec != 0) {
      return -1;
    }

    // read the response
    boost::asio::read(
         socket, boost::asio::buffer(response), ec);
    if (ec != 0) {
      return -1;
    }

    // good, return 0
    return 0;
  }
};

#endif

