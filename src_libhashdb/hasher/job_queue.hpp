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
 * Provides a threadsafe job queue with a maximum size:
 *
 * On push: yield the thread until the *job can be added.
 * On pop: pop the *job.  Returns NULL when empty.
 *
 * When done, set is_done to true so threads can know to exit.
 *
 * The idea is to have a few more buffers than threads so threads always
 * have a buffer to consume and we don't fill up RAM with waiting buffers.
 */


#ifndef JOB_QUEUE_HPP
#define JOB_QUEUE_HPP

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <string.h> // strlen
#include <vector>
#include <set>
#include <cassert>
#include <libewf.h>

#include <pthread.h>
#include <sched.h>  // sched_yield
#include "job.hpp"

namespace hasher {

class job_queue_t {

  private:
  size_t max_queue_size;
  std::queue<const hasher::job_t*> job_queue;

  public:
  bool is_done;

  mutable pthread_mutex_t M;                  // mutext

  // do not allow copy or assignment
  job_queue_t(const job_queue_t&);
  job_queue_t& operator=(const job_queue_t&);

  void lock() {
    if(pthread_mutex_lock(&M)) {
      assert(0);
    }
  }

  void unlock() {
    pthread_mutex_unlock(&M);
  }


 errx(1,"pthread_mutex_lock failed");


  public:
  job_queue_t(const p_max_queue_size) : max_queue_size(p_max_queue_size),
                                        job_queue(), is_done(false) {
    if(pthread_mutex_init(&M,NULL)) {
      std::cerr << "Error obtaining mutex.\n";
      assert(0);
    }
  }
    
  ~job_queue_t() {
    if (job_queue.size() > 0) {
      // program error if queue is not empty
      std::cerr << "Processing error: job ended but job queue is not empty.\n";
    }
    pthread_mutex_destroy(&M);
  }

  void push(const hasher::job_t* const job) {
    while(true) {
      // get job queue size
      lock();
      size_t size = job_queue.size();
      unlock();
      if (size < max_queue_size) {

        // add job to queue now
        lock();
        size_t size = job_queue.push(job);
        unlock();
        break;

      } else {
        // try again later
        sched_yield();
      }
    }
  }

  hasher::job_t* pop() {
    const job_t* job = NULL;
    lock();
    if (job_queue.size() > 0) {
      job = job_queue.pop();
    }
    unlock();
    return job;
  }
};

} // end namespace hasher

#endif

