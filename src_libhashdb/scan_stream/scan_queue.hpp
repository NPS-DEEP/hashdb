// Author:  Bruce Allen
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
 * A simple non-blocking threadsafe scan queue that knows when the
 * queue is busy.
 *
 * To work right, every non-empty put_unscanned must be matched with
 * a put_scanned.
 */

#ifndef SCAN_QUEUE_HPP
#define SCAN_QUEUE_HPP

#include <string>
#include <queue>
#include <pthread.h>
#include "tprint.hpp"

// diagnostic
//#define TEST_SCAN_QUEUE_HPP

namespace scan_stream {

class scan_queue_t {

  private:
  std::queue<std::string> unscanned;
  std::queue<std::string> scanned;
  size_t unscanned_submitted;
  size_t scanned_submitted;
  mutable pthread_mutex_t M;   // mutex

  // do not allow copy or assignment
  scan_queue_t(const scan_queue_t&);
  scan_queue_t& operator=(const scan_queue_t&);

  void lock() {
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&M);
  }

  public:
  scan_queue_t() : unscanned(), scanned(),
                   unscanned_submitted(0), scanned_submitted(0),
                   M() {
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }

  ~scan_queue_t() {
    if (busy()) {
      // warn
      hashdb::tprint("Processing error: scan queue was closed before processing completed.\n");
    }
    pthread_mutex_destroy(&M);
  }

  std::string get_unscanned() {
    lock();
    if (unscanned.empty()) {
      unlock();
      return "";
    }
    std::string unscanned_data = unscanned.front();
#ifdef TEST_SCAN_QUEUE_HPP
    std::cerr << "get_unscanned: " << unscanned_data << "\n";
#endif
    unscanned.pop();
    unlock();
    return unscanned_data;
  }

  void put_unscanned(const std::string unscanned_data) {
    if (unscanned_data.size() == 0) {
      // drop the request
      return;
    }
    lock();
#ifdef TEST_SCAN_QUEUE_HPP
    std::cerr << "put_unscanned: " << unscanned_data << "\n";
#endif
    ++unscanned_submitted;
    unscanned.push(unscanned_data);
    unlock();
  }

  std::string get_scanned() {
    lock();
    if (scanned.empty()) {
      unlock();
      return "";
    }
    std::string scanned_data = scanned.front();
#ifdef TEST_SCAN_QUEUE_HPP
    std::cerr << "get_scanned: " << scanned_data << "\n";
#endif
    scanned.pop();
    unlock();
    return scanned_data;
  }

  void put_scanned(const std::string scanned_data) {
    lock();
    ++scanned_submitted;
#ifdef TEST_SCAN_QUEUE_HPP
    std::cerr << "put_scanned: " << scanned_data << "\n";
#endif
    if (scanned_data.size() == 0) {
      // drop the result
      unlock();
      return;
    }
    scanned.push(scanned_data);
    unlock();
  }

  bool busy() {
    lock();
    // Busy when unscanned input is available or processing is active.
    // Unpulled scanned output does not count as busy.
    const bool is_busy = unscanned.size() != 0 ||
                         unscanned_submitted != scanned_submitted;
    unlock();
    return is_busy;
  }

  bool empty() {
    lock();
    // Empty when both queues are empty and processing is not active.
    const bool is_empty = unscanned.size() == 0 &&
                          scanned.size() == 0 &&
                          unscanned_submitted == scanned_submitted;
    unlock();
    return is_empty;
  }
};

} // end namespace scan_stream

#endif
