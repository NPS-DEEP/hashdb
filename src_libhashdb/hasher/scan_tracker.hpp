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
 * Tracks zero_count during threaded ingest to know how many zero blocks
 *   are skipped.  Read zero_count after all threads have closed.
 */

#ifndef SCAN_TRACKER_HPP
#define SCAN_TRACKER_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include "tprint.hpp"

namespace hasher {

class scan_tracker_t {

  public:
  /**
   * Read zero_count after threads have closed.
   */
  size_t zero_count;

  private:
  const uint64_t bytes_total;
  uint64_t bytes_done;
  uint64_t bytes_reported_done;
  mutable pthread_mutex_t M;
  
  // do not allow copy or assignment
  scan_tracker_t(const scan_tracker_t&);
  scan_tracker_t& operator=(const scan_tracker_t&);

  void lock() {
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&M);
  }

  public:
  scan_tracker_t(const uint64_t p_bytes_total) :
                     zero_count(0), bytes_total(p_bytes_total),
                     bytes_done(0), bytes_reported_done(0), M() {
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }

  ~scan_tracker_t() {
    pthread_mutex_destroy(&M);
  }

  void track_zero_count(const uint64_t p_zero_count) {
    lock();
    zero_count += p_zero_count;
    unlock();
  }

  void track_bytes(const uint64_t count) {
    static const size_t INCREMENT = 134217728; // = 2^27 = 100 MiB
    lock();
    bytes_done += count;
    if (bytes_done == bytes_total ||
        bytes_done > bytes_reported_done + INCREMENT) {

      // print %done
      std::stringstream ss;
      ss << "# " << bytes_done
         << " of " << bytes_total
         << " bytes completed (" << bytes_done * 100 / bytes_total
         << "%)\n";
      hashdb::tprint(ss.str());

      // next milestone
      bytes_reported_done += INCREMENT;
    }
    unlock();
  }
};

} // end namespace hasher

#endif
