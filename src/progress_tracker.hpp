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
 * Writes progress to cout and to <dir>/progress.json log.
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
#include "../src_libhashdb/hashdb.hpp" // for timestamp

class progress_tracker_t {
  private:
  const std::string dir;
  const uint64_t total;
  uint64_t index;
  std::ofstream os;
  hashdb::timestamp_t timestamp;
 
  // do not allow copy or assignment
  progress_tracker_t(const progress_tracker_t&);
  progress_tracker_t& operator=(const progress_tracker_t&);

  public:
  progress_tracker_t(const std::string& p_dir, const uint64_t p_total) :
                         dir(p_dir),
                         total(p_total),
                         index(0),
                         os(),
                         timestamp() {
    std::string filename(dir+"/progress.json");

    // open, fatal if unable to open
    os.open(filename.c_str());
    if (!os.is_open()) {
      std::cout << "Cannot open progress tracker file " << filename
                << ": " << strerror(errno) << "\n";
      exit(1);
    }
  }

  void track() {
    if (index%100000 == 0 && index > 0) {
      std::stringstream ss;
      if (total > 0) {
        // total is known
        ss << "# Processing index " << index << " of " << total;
      } else {
        // total is not known
        ss << "# Processing index " << index << " of ?";
      }
      std::cout << ss.str() << "..." << std::endl;
      os << timestamp.stamp(ss.str());
    }
    ++index;
  }

  ~progress_tracker_t() {
    std::stringstream ss;
    if (total > 0) {
      // total is known
      ss << "# Processing index " << index << " of " << total << " completed.";
    } else {
      // total is not known
      ss << "# Processing index " << index << " of " << index << " completed.";
    }
    std::cout << ss.str() << std::endl;
    os << timestamp.stamp(ss.str());

    os.close();
  }
};

#endif

