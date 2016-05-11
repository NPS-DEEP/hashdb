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
 * Creates a pool of threads.
 *
 * Threads will continually pop *job from job_queue
 * and call hasher::process_job(job) until job_queue->is_done.
 *
 * Destructor waits on join for all threads.
 */

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <cstring>
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
#include "job.hpp"
#include "job_queue.hpp"
#include "process_job.hpp"

namespace hasher {

  static void* run(void* const arg) {
    // get job_queue
    hasher::job_queue_t* const job_queue =
                           static_cast<hasher::job_queue_t* const>(arg);

    while (job_queue->is_done == false) {
      const hasher::job_t* const job = job_queue->pop();
      if (job == NULL) {
        // queue is empty
        sched_yield();
      } else {
        // process the job
        hasher::process_job(*job);
      }
    }
    return 0;
  }

class threadpool_t {
  private:
  const int num_threads;
  ::pthread_t* threads;
  const hasher::job_queue_t* job_queue;

  public:
  threadpool_t(const int p_num_threads, const job_queue_t* const p_job_queue) :
           num_threads(p_num_threads),
           threads(new ::pthread_t[num_threads]),
           job_queue(p_job_queue) {

    // open one thread per CPU
    for (int i=0; i<num_threads; i++) {
      int rc = ::pthread_create(&threads[i], NULL, hasher::run,
                                (void*)job_queue);
      if (rc != 0) {
        std::cerr << "Unable to start hasher thread.\n";
        assert(0);
      }
    }
  }

  ~threadpool_t() {
    // join each thread
    for (int i=0; i<num_threads; i++) {
      int status = pthread_join(threads[i], NULL);
      if (status != 0) {
        std::cerr << "error in threadpool join " << status << "\n";
      }
    }
    delete[] threads;
  }
};

} // end namespace hasher

#endif
