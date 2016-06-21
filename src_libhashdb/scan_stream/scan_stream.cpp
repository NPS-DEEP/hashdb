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
 * Creates a pool of scanner threads that consume from input pipe
 * and write matches to output pipe until input pipe closes.
 */
#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <sched.h>  // sched_yield
#include <pthread.h>
#include "scan_thread_data.hpp"
#include "num_cpus.hpp"

static void* run(void* const arg) {
  // get pointer to scan thread data job
  scan_stream::scan_thread_data_t* const job =
                    static_cast<scan_stream::scan_thread_data_t* const>(arg);

  // make space for the hash and label
  char* char_hash = new char[job->hash_size]();
  char* char_label = new char[job->label_size]();

  // size of unscanned record
  const size_t unscanned_record_size = job->hash_size + job->label_size;

  // get and process input arrays until signaled to close
  // print warnings to stderr
  while (!job->should_close || job->scan_queue.busy()) {

    // read unscanned from scan_queue
    const std::string unscanned_array = job->scan_queue.get_unscanned();

    // empty so pause and retry
    if (unscanned_array.size() == 0) {
      sched_yield();
      continue;
    }

    // set up input stream from unscanned_array
    std::istringstream unscanned_stream(unscanned_array);

    // set up empty scanned output stream
    std::ostringstream scanned_stream;

    // read and process the scan input elements
    while (unscanned_stream.peek() != EOF) {

      // char_hash
      unscanned_stream.read(char_hash, job->hash_size);

      // char_label
      unscanned_stream.read(char_label, job->label_size);

      // check for underflow
      if (unscanned_stream.rdstate() & std::ios::eofbit) {
        std::stringstream ss;
        ss << "Data error in scan_stream thread: unscanned input data was truncated.\n"
           << "A record size of " << unscanned_record_size
           << " bytes was requested but only "
           << unscanned_array.size() % unscanned_record_size
           << " were available.\n";

        hashdb::tprint(ss.str());
        continue;
      }

      // scan
      const std::string json_response = job->scan_manager->find_hash_json(
                     job->scan_mode, std::string(char_hash, job->hash_size));

      if (json_response.size() > 0) {

        // calculate response payload size
        const uint64_t scanned_record_size =
                               unscanned_record_size + json_response.size();

        // response size
        scanned_stream.write(
                        reinterpret_cast<const char*>(&scanned_record_size),
                        sizeof(uint64_t));

        // binary hash
        scanned_stream.write(char_hash, job->hash_size);

        // label
        scanned_stream.write(char_label, job->label_size);

        // JSON
        scanned_stream << json_response;
      }
    }

    // push result back, even if empty
    job->scan_queue.put_scanned(scanned_stream.str());
  }

  // clean up
  delete[] char_hash;
  delete[] char_label;
  return 0;
}

namespace hashdb {

  scan_stream_t::scan_stream_t(
              hashdb::scan_manager_t* const scan_manager,
              const size_t hash_size,
              const size_t label_size,
              const hashdb::scan_mode_t scan_mode) :
         num_threads(hashdb::numCPU()),
         threads(new ::pthread_t[num_threads]),
         scan_thread_data(new scan_stream::scan_thread_data_t(
                          scan_manager, hash_size, label_size, scan_mode)),
         finished(false) {

    // open one scan thread per CPU
    for (int i=0; i<num_threads; i++) {
      int rc = ::pthread_create(&threads[i], NULL, run,
                                (void*)scan_thread_data);
      if (rc != 0) {
        std::cerr << "Unable to start scan_stream thread: "
                  << strerror(rc) << ".\n";
        assert(0);
      }
    }
  }

  // put in data to scan
  void scan_stream_t::put(const std::string& unscanned_data) {
    if (finished) {
      std::cerr << "Usage error in scan_stream: put is not allowed after finish.\n";
      return;
    }
    scan_thread_data->scan_queue.put_unscanned(unscanned_data);
  }

  // get scanned data or "" if none available
  std::string scan_stream_t::get() {
    return scan_thread_data->scan_queue.get_scanned();
  }

  // done with input so wrap up and close
  void scan_stream_t::finish() {
    // set the flag that closes threads when scan_queue is no longer busy
    scan_thread_data->should_close = true;

    // join each thread
    for (int i=0; i<num_threads; i++) {
      int status = pthread_join(threads[i], NULL);
      if (status != 0) {
        std::cerr << "Error in threadpool join: " << strerror(status) << ".\n";
      }
    }
    finished = true;
  }

  scan_stream_t::~scan_stream_t() {
    // warn if threads are open
    if (!finished) {
      std::cerr << "Usage error in scan_stream: please call finish before closing to ensure\nthat processing has completed.\n";
      finish();
    }

    // warn if data is left behind
    if (scan_thread_data->scan_queue.busy()) {
      std::cerr << "Usage error in scan_stream: stream closed but more data is available.\n";
    }

    delete[] threads;
    delete scan_thread_data;
  }

} // end namespace hashdb

