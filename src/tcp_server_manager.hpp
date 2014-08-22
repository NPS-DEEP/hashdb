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
 * Provides the server hashdb scan service using Boost.Asio.
 * Modeled after the Boost blocking_tcp_echo_server.cpp example.
 */

#ifndef TCP_SERVER_MANAGER_HPP
#define TCP_SERVER_MANAGER_HPP

#include <config.h>
#include "hashdb.hpp"
#include "hashdb_manager.hpp"
#include "file_modes.h"
#include "hash_t_selector.h"
#include <boost/bind.hpp>
#define BOOST_THREAD_USE_LIB // required because of compiler rules for Windows
#include <boost/thread.hpp>
#include <boost/asio.hpp>

class tcp_server_manager_t {

  private:
  hashdb_manager_t hashdb_manager;
  boost::asio::io_service io_service;
  boost::mutex scan_mutex;

  typedef boost::asio::ip::tcp::socket* socket_ptr_t;

  // run a complete connection session.
  // The session is run on a thread by a server dispatcher.
  void run_session(socket_ptr_t socket_ptr) {
    try {
      // improve speed by disabling nagle's algorithm
      socket_ptr->set_option(boost::asio::ip::tcp::no_delay(true));

      // loop servicing request/response cycles
      bool valid_session = true;
      while(valid_session) {
        valid_session = do_scan(socket_ptr);
      }
    } catch (std::exception& e) {
      std::cerr << "Exception in request, request dropped: " << e.what() << "\n";
    }
    delete socket_ptr;
  }

  // perform one request/response scan iteration
  // true = more, false=EOF
  bool do_scan(socket_ptr_t socket_ptr) {
    
    // read the request size
    uint32_t request_size;
    boost::system::error_code error;
    boost::asio::read(*socket_ptr,
                      boost::asio::buffer(&request_size, sizeof(request_size)),
                      error);

    // the protocol allows us to leave the scan loop here
    if (error == boost::asio::error::eof) {
      // done
      return false;
    } else if (error) {
      throw boost::system::system_error(error); // Some other error.
    }

    // allocate request and response vectors on heap
    std::vector<hash_t>* request_ptr = new std::vector<hash_t>(request_size);
    typename hashdb_t__<hash_t>::scan_output_t* response_ptr = new typename hashdb_t__<hash_t>::scan_output_t();

    // read the request
    boost::asio::read(*socket_ptr, boost::asio::buffer(*request_ptr));

    // lock this until we are confident that reading is threadsafe
    // use boost mutex since this will be running on a boost-launched thread
    scan_mutex.lock();

    // scan each input in turn
    for (uint32_t i=0; i<request_size; ++i) {
      uint32_t count = hashdb_manager.find_count((*request_ptr)[i]);
      if (count > 0) {
        response_ptr->push_back(std::pair<uint32_t, uint32_t>(i, count));
      }
    }

    // unlock
    scan_mutex.unlock();

    // send the response count
    uint32_t response_size = response_ptr->size();
    boost::asio::write(*socket_ptr, boost::asio::buffer(
                                    &response_size, sizeof(response_size)));

    // send the response vector
    boost::asio::write(*socket_ptr, boost::asio::buffer(*response_ptr));

    delete request_ptr;
    delete response_ptr;

    return true;
  }

  public:
  /**
   * This works as follows:
   *   initialize state,
   *   while true:
   *     wait to accept a socket connection,
   *     dispatch the connection to service request, scan, response queries.
   *
   * Note that the service thread intentionally runs on the
   * tcp_server_manager_t singleton in order to have visibility to
   * its resources, and that the singleton must access threadsafe members.
   */
  tcp_server_manager_t(const std::string& hashdb_dir, uint16_t port_number) :
                       hashdb_manager(hashdb_dir, READ_ONLY),
                       io_service(),
                       scan_mutex() {

    try {
      boost::asio::ip::tcp::acceptor acceptor(io_service,
                  boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                 port_number));
      while (true) {
        socket_ptr_t socket_ptr(new boost::asio::ip::tcp::socket(io_service));
        acceptor.accept(*socket_ptr);

        // now run this.run_session on a thread.
        // this should be safe since the resources it accesses are threadsafe.
        boost::thread t(boost::bind(&tcp_server_manager_t::run_session,
                                    this,
                                    socket_ptr));
      }
    } catch (std::exception& e) {
      std::cerr << "Exception: unable to connect to socket.  Aborting.  " << e.what() << "\n";
    }
  }
};

#endif

