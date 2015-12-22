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
 * Timestamp tool.
 */

#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

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

// helper to get valid json output, taken from
// http://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
static std::string escape_json(const std::string& input) {
  std::ostringstream ss;
  //for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
  //C++98/03:
  for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
    switch (*iter) {
      case '\\': ss << "\\\\"; break;
      case '"': ss << "\\\""; break;
      case '/': ss << "\\/"; break;
      case '\b': ss << "\\b"; break;
      case '\f': ss << "\\f"; break;
      case '\n': ss << "\\n"; break;
      case '\r': ss << "\\r"; break;
      case '\t': ss << "\\t"; break;
      default: ss << *iter; break;
    }
  }
  return ss.str();
}

namespace hashdb {

  timestamp_t::timestamp_t() :
              t0(new timeval()), t_last_timestamp(new timeval()) {
    gettimeofday(t0, 0);
    gettimeofday(t_last_timestamp, 0);
  }

  timestamp_t::~timestamp_t() {
    delete t0;
    delete t_last_timestamp;
  }

  /**
   * Take a timestamp and return a JSON string in format {"name":"name",
   * "delta":delta, "total":total}.
   */
  std::string timestamp_t::stamp(const std::string &name) {
    // adapted from dfxml_writer.cpp
    struct timeval t1;
    gettimeofday(&t1,0);
    struct timeval t;

    // timestamp delta against t_last_timestamp
    t.tv_sec = t1.tv_sec - t_last_timestamp->tv_sec;
    if(t1.tv_usec > t_last_timestamp->tv_usec){
        t.tv_usec = t1.tv_usec - t_last_timestamp->tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t_last_timestamp->tv_usec;
    }
    char delta[16];
    snprintf(delta, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // reset t_last_timestamp for the next invocation
    gettimeofday(t_last_timestamp,0);

    // timestamp total
    t.tv_sec = t1.tv_sec - t0->tv_sec;
    if(t1.tv_usec > t0->tv_usec){
        t.tv_usec = t1.tv_usec - t0->tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t0->tv_usec;
    }
    char total_time[16];
    snprintf(total_time, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // return the named timestamp
    std::stringstream ss;
    ss << "{\"name\":\"" << escape_json(name) << "\""
       << ", \"delta\":" << delta
       << ", \"total\":" << total_time << "}"
       << "\n";
    return ss.str();
  }

}

#endif

