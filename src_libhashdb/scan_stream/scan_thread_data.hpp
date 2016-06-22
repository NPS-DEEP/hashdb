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
 * The data structure used by all scan threads.
 */

#ifndef SCAN_THREAD_DATA_HPP
#define SCAN_THREAD_DATA_HPP

#include <stdint.h>
#include "hashdb.hpp"
#include "scan_queue.hpp"

namespace scan_stream {

// common information used by scanner threads
class scan_thread_data_t {

  public:
  hashdb::scan_manager_t* const scan_manager;
  const size_t hash_size;
  const ::hashdb::scan_mode_t scan_mode;
  scan_queue_t scan_queue;
  volatile bool done;

  // do not allow copy or assignment
  scan_thread_data_t(const scan_thread_data_t&);
  scan_thread_data_t& operator=(const scan_thread_data_t&);

  public:
  scan_thread_data_t(hashdb::scan_manager_t* const p_scan_manager,
                     const size_t p_hash_size,
                     const hashdb::scan_mode_t p_scan_mode) :
            scan_manager(p_scan_manager),
            hash_size(p_hash_size),
            scan_mode(p_scan_mode),
            scan_queue(),
            done(false) {
  }
};

} // end namespace scan_stream

#endif
