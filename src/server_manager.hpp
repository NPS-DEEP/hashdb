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

#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <config.h>
#include "hashdb.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>

// types of queries that are available
const uint32_t QUERY_MD5 = 1;
const uint32_t QUERY_SHA1 = 2;
const uint32_t QUERY_SHA256 = 3;

// ************************************************************
// session_t
// Session sequence:
//       start reads request_type
//       read_request_count
//       read_md5, read_sha1, or read_sha256
//       write_response
// ************************************************************
class session_t {
  private:
  hashdb_t* hashdb;
  boost::asio::ip::tcp::socket session_socket;
  uint32_t request_type;
  uint16_t request_count;
  hashdb_t::scan_input_md5_t* scan_input_md5;
  hashdb_t::scan_input_sha1_t* scan_input_sha1;
  hashdb_t::scan_input_sha256_t* scan_input_sha256;
  hashdb_t::scan_output_t* scan_output;

  // do not allow copy or assignment
  session_t(const session_t&);
  session_t& operator=(const session_t&);

  public:
  session_t(hashdb_t* hashdb_p, boost::asio::io_service& io_service) :
                    hashdb(hashdb_p),
                    session_socket(io_service),
                    request_type(0),
                    request_count(0),
                    scan_input_md5(0),
                    scan_input_sha1(0),
                    scan_input_sha256(0),
                    scan_output(new hashdb_t::scan_output_t) {
  }

  // get this session's socket
  boost::asio::ip::tcp::socket& socket() {
    return session_socket;
  }

  // read the request type, one of MD5, SHA1, or SHA256
  void start() {
    boost::asio::async_read(
               session_socket,

               // get request_type
               boost::asio::buffer(&request_type, sizeof(request_type)),

               // next step will be to call read_request_count
               boost::bind(&session_t::read_request_count,
                           this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
  }

  // read the request count
  void read_request_count(const boost::system::error_code& error,
                          size_t bytes_transferred) {
    if (!error) {
      boost::asio::async_read(
               session_socket,

               // get request_count
               boost::asio::buffer(&request_count, sizeof(request_count)),

               // next step will be to call read_vector
               boost::bind(&session_t::read_vector,
                           this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
    } else {
      // failure in read
      delete this;
    }
  }

  // read the vector of input data
  void read_vector(const boost::system::error_code& error,
                   size_t bytes_transferred) {
    if (!error) {
      // the read operation depends on the request type
      if (request_type == QUERY_MD5) {
        scan_input_md5 = new hashdb_t::scan_input_md5_t(request_count);
        boost::asio::async_read(
               session_socket,

               // get request_count
               boost::asio::buffer(*scan_input_md5),

               // next step will be to call write_vector
               boost::bind(&session_t::write_vector_md5,
                           this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
      } else if (request_type == QUERY_SHA1) {
        scan_input_sha1 = new hashdb_t::scan_input_sha1_t(request_count);
        boost::asio::async_read(
               session_socket,

               // get request_count
               boost::asio::buffer(*scan_input_sha1),

               // next step will be to call write_vector
               boost::bind(&session_t::write_vector_sha1,
                           this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
      } else if (request_type == QUERY_SHA256) {
        scan_input_sha256 = new hashdb_t::scan_input_sha256_t(request_count);
        boost::asio::async_read(
               session_socket,

               // get request_count
               boost::asio::buffer(*scan_input_sha256),

               // next step will be to call write_vector
               boost::bind(&session_t::write_vector_sha256,
                           this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
      } else {
        // failure in request type read
        delete this;
      }
    } else {
      // failure in read
      delete this;
    }
  }

  // look up data
  void write_vector_md5(const boost::system::error_code& error,
                        size_t bytes_transferred) {

    if (!error && request_count * sizeof(hashdb_t::import_element_t<md5_t>) ==
                  bytes_transferred) {

      // perform the scan
      hashdb->scan(*scan_input_md5, *scan_output);
      delete scan_input_md5;

      // send the output
      boost::asio::async_write(
               session_socket,
               boost::asio::buffer(*scan_output),
               boost::bind(&session_t::completion_handler_md5,
               this,
               boost::asio::placeholders::error));
    } else {
      delete scan_input_md5;
      delete this;
    }
  }

  void write_vector_sha1(const boost::system::error_code& error,
                        size_t bytes_transferred) {

    if (!error && request_count * sizeof(hashdb_t::import_element_t<sha1_t>) ==
                  bytes_transferred) {

      // perform the scan
      hashdb->scan(*scan_input_sha1, *scan_output);
      delete scan_input_sha1;

      // send the output
      boost::asio::async_write(
               session_socket,
               boost::asio::buffer(*scan_output),
               boost::bind(&session_t::completion_handler_sha1,
               this,
               boost::asio::placeholders::error));
    } else {
      delete scan_input_sha1;
      delete this;
    }
  }

  void write_vector_sha256(const boost::system::error_code& error,
                        size_t bytes_transferred) {

    if (!error && request_count * sizeof(hashdb_t::import_element_t<sha256_t>)
                 == bytes_transferred) {

      // perform the scan
      hashdb->scan(*scan_input_sha256, *scan_output);
      delete scan_input_sha256;

      // send the output
      boost::asio::async_write(
               session_socket,
               boost::asio::buffer(*scan_output),
               boost::bind(&session_t::completion_handler_sha256,
               this,
               boost::asio::placeholders::error));
    } else {
      delete scan_input_sha256;
      delete this;
    }
  }

  void completion_handler_md5(const boost::system::error_code& error,
                        size_t bytes_transferred) {
    delete scan_output;
    delete this;
  }

  void completion_handler_sha1(const boost::system::error_code& error,
                        size_t bytes_transferred) {
    delete scan_output;
    delete this;
  }

  void completion_handler_sha256(const boost::system::error_code& error,
                        size_t bytes_transferred) {
    delete scan_output;
    delete this;
  }
};

// ************************************************************
// server_t
// ************************************************************
class server_t {
  private:
  hashdb_t* hashdb;
  boost::asio::io_service& io_service;
  boost::asio::ip::tcp::acceptor acceptor;

  // do not allow copy or assignment
  server_t(const server_t&);
  server_t& operator=(const server_t&);

  // async wait for a session, run handle_accept when available
  void start_accept() {
    session_t* session = new session_t(hashdb, io_service);
    acceptor.async_accept(
          session->socket(),
          boost::bind(&server_t::handle_accept, this, session,
                      boost::asio::placeholders::error));
  }

  // arrive here when tcp::acceptor accepts a new session
  void handle_accept(session_t* session,
                     const boost::system::error_code& error) {
    if (!error) {
      // start timeout timer for killing stale session
      //TBD

      // begin reading session data
      session->start();
    } else {
      // give up this session that just started because it had an error
      delete session;
    }

    // session is done, accept a new session
    start_accept();
  }

  public:
  server_t(hashdb_t* hashdb_p,
           uint16_t port_number,
           boost::asio::io_service& io_service_p) :
             hashdb(hashdb_p),
             io_service(io_service_p),
             acceptor(io_service, boost::asio::ip::tcp::endpoint(
                                boost::asio::ip::tcp::v4(), port_number)) {
    start_accept();
  }
};

// ************************************************************
// server_manager_t
// ************************************************************
class server_manager_t {
  // note: basic action:
  //   connect
  //   read from connection
  //   query hashdb
  //   write connection
  // so to optimize, start several server handlers to help with TCP I/O.
  // Don't start too many, the bottleneck is in lookup.
  //
  // note: smart pointers for input and output buffers would be much cleaner.

  private:
  hashdb_t hashdb;
  boost::asio::io_service io_service;
  server_t server1;
  server_t server2;

  public:
  server_manager_t(const std::string& hashdb_dir, uint16_t port_number) :
                    hashdb(hashdb_dir),
                    io_service(),
                    server1(&hashdb, port_number, io_service),
                    server2(&hashdb, port_number, io_service) {
    io_service.run();
  }
};

#endif

