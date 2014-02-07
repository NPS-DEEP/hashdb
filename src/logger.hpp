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
 * The logger manages logging to the hashdb log.xml file.
 * The log is created when the logger object opens,
 * and is closed by close or when the logger is destroyed,
 * e.g., by losing scope.
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "bloom_filter.hpp"
#include "command_line.hpp"
#include "hashdb_changes.hpp"
#include "settings.hpp"
#include <iostream>

/**
 * The hashdb change logger holds all possible change values,
 * and is used for reporting changes to the database.
 */
class logger_t {
  private:
  dfxml_writer x;
  bool closed;

  // don't allow copy
  logger_t(const logger_t& changes);

  public:

  logger_t(std::string hashdb_dir) :
                    x(hashdb_dir+"/log.xml", false),
                    closed(false) {

    // log the preamble
    x.push("log");

    x.push("command");
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line_t::command_line_string);
  }

  // create a "closed" logger
  logger_t() : x(), closed(true) {
  }

  ~logger_t() {
    if (!closed) {
      close();
    }
  }

  /**
   * You can close the logger and use the log before the logger is disposed
   * by calling close.
   * Do not use the logger after closing it; this will corrupt the log file.
   */
  void close() {
    if (closed) {
      // logger closed
      std::cout << "hashdb_change_logger.close warning: logger closed\n";
      return;
    }

    // log closure for x
    x.add_rusage();
    x.pop(); // command
    x.pop(); // log
  }

  /**
   * Emit a named timestamp.
   */
  void add_timestamp(const std::string& name) {
    if (closed) {
      // logger closed
      std::cout << "logger.add_timestamp warning: logger closed\n";
      return;
    }

    x.add_timestamp(name);
  }

  void add_hashdb_settings(const settings_t& settings) {
    if (closed) {
      // logger closed
      std::cout << "logger.add_hashdb_settings warning: logger closed\n";
      return;
    }

    settings.report_settings(x);
  }

  void add_hashdb_changes(const hashdb_changes_t& changes) {
    if (closed) {
      // logger closed
      std::cout << "logger.add_hashdb_changes warning: logger closed\n";
      return;
    }

    changes.report_changes(x);
  }

  /**
   * Add hashdb_db_manager state to the log.
   */
/*
  void add_hashdb_db_manager_state(const hashdb_db_manager_t manager) {
    manager.report_status(x);

    // zz maybe also to stdout
  }
*/
  void add_hashdb_db_manager_state() {
    if (closed) {
      // logger closed
      std::cout << "hashdb_change_logger.add_hashdb_db_manager_state warning: logger closed\n";
      return;
    }

    x.xmlout("state", "TBD");
  }

  /**
   * Add a tag, value pair for any type supported by xmlout.
   */
  template<typename T>
  void add(const std::string& tag, const T& value) {
    if (closed) {
      // logger closed
      std::cout << "hashdb_change_logger.add warning: logger closed\n";
      return;
    }

    x.xmlout(tag, value);
  }
};

#endif

