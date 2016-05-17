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
  mutable pthread_mutex_t M;
  
  // do not allow copy or assignment
  scan_tracker_t(const scan_tracker_t&);
  scan_tracker_t& operator=(const scan_tracker_t&);

  public:
  scan_tracker_t() : zero_count(0), M() {
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }

  ~scan_tracker_t() {
    pthread_mutex_destroy(&M);
  }

  void track(const uint64_t p_zero_count) {
    // lock
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }

    // update zero_count
    zero_count += p_zero_count;

    // unlock
    pthread_mutex_unlock(&M);
  }
};

} // end namespace hasher

#endif
