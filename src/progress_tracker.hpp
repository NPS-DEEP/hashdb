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
 * Track progress to show that long iterative actions are not hung.
 * Call track before operation, call done when done.
 * Writes progress to cout and to <dir>/progress.xml.
 * Use total=0 if total is not known.
 */

#ifndef PROGRESS_TRACKER_HPP
#define PROGRESS_TRACKER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include "json_helper.hpp"

class progress_tracker_t {
  private:
  const std::string dir;
  const uint64_t total;
  const bool is_quiet;
  uint64_t index;
  std::ofstream os;
  struct timeval t0;
  struct timeval t_last_timestamp;
 
  // do not allow copy or assignment
  progress_tracker_t(const progress_tracker_t&);
  progress_tracker_t& operator=(const progress_tracker_t&);

  // helper
  void add_timestamp(const std::string &name) {
    // adapted from dfxml_writer.cpp
    struct timeval t1;
    gettimeofday(&t1,0);
    struct timeval t;

    // timestamp delta against t_last_timestamp
    t.tv_sec = t1.tv_sec - t_last_timestamp.tv_sec;
    if(t1.tv_usec > t_last_timestamp.tv_usec){
        t.tv_usec = t1.tv_usec - t_last_timestamp.tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t_last_timestamp.tv_usec;
    }
    char delta[16];
    snprintf(delta, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // reset t_last_timestamp for the next invocation
    gettimeofday(&t_last_timestamp,0);

    // timestamp total
    t.tv_sec = t1.tv_sec - t0.tv_sec;
    if(t1.tv_usec > t0.tv_usec){
        t.tv_usec = t1.tv_usec - t0.tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t0.tv_usec;
    }
    char total_time[16];
    snprintf(total_time, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // write out the named timestamp
    std::stringstream ss;
    os << "{\"name\":\"" << name << "\""
       << ", \"delta\":" << delta
       << ", \"total\":" << total_time << "}"
       << "\n";
  }

  public:
  progress_tracker_t(const std::string& p_dir, const uint64_t p_total,
                     const bool p_is_quiet, const std::string& cmd) :
                         dir(p_dir),
                         total(p_total),
                         is_quiet(p_is_quiet),
                         index(0),
                         os(),
                         t0(),
                         t_last_timestamp() {
    std::string filename(dir+"/progress.xml");

    // fatal if unable to open
    if (!os.is_open()) {
      std::cout << "Cannot open progress tracker file " << filename
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // get t0
    gettimeofday(&t0,0);
    gettimeofday(&t_last_timestamp,0);

    // log environment information
    os << "# {\"program\":\"" << PACKAGE_NAME << "\""
       << ", \"version\":\"" << PACKAGE_VERSION << "\""
       << ", \"command_line\":\"" << "\""
       << json_helper::escape_json(cmd) << "\"}";
  }

  void track() {
    if (index%100000 == 0 && index > 0) {
      std::stringstream ss;
      if (total > 0) {
        // total is known
        ss << "Processing index " << index << " of " << total;
      } else {
        // total is not known
        ss << "Processing index " << index << " of ?";
      }
      if (!is_quiet) {
        std::cout << ss.str() << "..." << std::endl;
      }
      add_timestamp(ss.str());
    }
    ++index;
  }

  ~progress_tracker_t() {
    std::stringstream ss;
    if (total > 0) {
      // total is known
      ss << "Processing index " << index << " of " << total << " completed";
    } else {
      // total is not known
      ss << "Processing index " << index << " of " << index << " completed";
    }
    if (!is_quiet) {
      std::cout << ss.str() << std::endl;
    }
    add_timestamp(ss.str());

    os.close();
  }
};

#endif

