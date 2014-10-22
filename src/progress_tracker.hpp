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
 * Writes progress to cout and optionally to logger_t.
 * Use total=0 if total is not known.
 */

#ifndef PROGRESS_TRACKER_HPP
#define PROGRESS_TRACKER_HPP
#include "logger.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <iostream>

extern bool quiet_mode;

class progress_tracker_t {
  private:
  const uint64_t total;
  uint64_t index;
  logger_t* logger;
  bool use_logger;
  bool is_done;

  // do not allow copy or assignment
  progress_tracker_t(const progress_tracker_t&);
  progress_tracker_t& operator=(const progress_tracker_t&);

  public:
  progress_tracker_t(uint64_t p_total) :
                         total(p_total), index(0), logger(0),
                         use_logger(false), is_done(false) {
  }
  progress_tracker_t(uint64_t p_total, logger_t* p_logger) :
                         total(p_total), index(0), logger(p_logger),
                         use_logger(true), is_done(false) {
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
      if (!quiet_mode) {
        std::cout << ss.str() << "..." << std::endl;
      }
      if (use_logger) {
        logger->add_timestamp(ss.str());
        logger->add_memory_usage(ss.str());
      }
    }
    ++index;
  }

  void done() {
    std::stringstream ss;
    if (total > 0) {
      // total is known
      ss << "Processing index " << index << " of " << total << " completed";
    } else {
      // total is not known
      ss << "Processing index " << index << " of " << index << " completed";
    }
    if (!quiet_mode) {
      std::cout << ss.str() << std::endl;
    }
    if (use_logger) {
      logger->add_timestamp(ss.str());
      logger->add_memory_usage(ss.str());
    }
    is_done = true;
  }

  ~progress_tracker_t() {
    if (!is_done) {
      std::cerr << "program error: progress_tracker.track_done not called.\n";
    }
  }
};

#endif

