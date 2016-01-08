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
 * Append change logs to the log file.
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "lmdb_changes.hpp"
#include "hashdb_settings.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cassert>
#include "hashdb.hpp" // for timestamp_t

/**
 * The logger appends database change information to the log file.
 */
class logger_t {
  private:
  const std::string hashdb_dir;
  std::ofstream os;
  hashdb::timestamp_t timestamp;

  // do not allow copy or assignment
  logger_t(const logger_t&);
  logger_t& operator=(const logger_t&);

  public:

  logger_t(std::string p_hashdb_dir, std::string command_line) :
                    hashdb_dir(p_hashdb_dir),
                    os(),
                    timestamp() {

    std::string filename(hashdb_dir+"/log.txt");

    // open append, fatal if unable to open
    os.open(filename.c_str(), std::fstream::app);
    if (!os.is_open()) {
      std::cout << "Cannot open log file " << filename
                << ": " << strerror(errno) << "\nAborting.\n";
      exit(1);
    }

    // log environment information
    hashdb::print_environment(command_line, os);

    // log start
    os << timestamp.stamp("begin");
  }

  // log
  void add_log(const std::string& name) {
    os << name;
  }

  // timestamp
  void add_timestamp(const std::string& name) {
    os << timestamp.stamp(name);
  }

  // settings
  void add_hashdb_settings(const hashdb_settings_t& settings) {
    os << settings;
  }

  void add_lmdb_changes(const lmdb_changes_t& changes) {
    os << changes;
  }

  // destructor
  ~logger_t() {

    // log end
    os << timestamp.stamp("end");
    os.close();
  }

};

#endif

