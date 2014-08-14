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

#include "command_line.hpp"
#include "hashdb_changes.hpp"
#include "hashdb_settings.hpp"
#include "hash_t_selector.h"
#include "history_manager.hpp"
#include <iostream>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

inline void add_memory_usage_algorithm(dfxml_writer* writer, std::string name);

/**
 * The logger logs commands performed that relate to the database.
 * Upon closure, the log is additionally appended to the history log.
 */
class logger_t {
  private:
  const std::string hashdb_dir;
  dfxml_writer x;
  bool closed;

  // don't allow copy
  logger_t(const logger_t& changes);

  public:

  logger_t(std::string p_hashdb_dir, std::string name) :
                    hashdb_dir(p_hashdb_dir),
                    x(hashdb_dir+"/log.xml", false),
                    closed(false) {

    // log the preamble
    x.push("log");

    std::stringstream ss;
    ss << "name='" << name << "'";
    x.push("command", ss.str());
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "", command_line_t::command_line_string);
  }

/*
???????????zzzzzzzz
  // create a "closed" logger
  logger_t() : x(), closed(true) {
  }
*/

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

    // mark this logger as closed
    x.flush();
    closed = true;

    // append log to history
    history_manager_t::append_log_to_history(hashdb_dir);
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

  /**
   * Emit a named memory usage report.
   */
  void add_memory_usage(const std::string& name) {
    if (closed) {
      // logger closed
      std::cout << "logger.add_memory_usage warning: logger closed\n";
      return;
    }

    add_memory_usage_algorithm(&x, name);
  }

  template<typename T>
  void add_hashdb_configuration() {
    if (closed) {
      // logger closed
      std::cout << "logger.add_hashdb_settings warning: logger closed\n";
      return;
    }

    // see file configure.ac for configuration settings
    x.xmlout("hashdigest_type", digest_name<T>());
    x.xmlout("byte_alignment", BYTE_ALIGNMENT);

  }

  void add_hashdb_settings(const hashdb_settings_t& settings) {
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

inline void add_memory_usage_algorithm(dfxml_writer* logger, std::string name) {
#ifdef HAVE_MALLINFO
  // NOTE: this data may not be useful, we need a better way
  struct mallinfo mi;
  std::stringstream ss;
  mi = mallinfo();
  ss << "name='" << name
//     << "' allocated='" << mi.arena
     << "' occupied='" << mi.uordblks
//     << "' arena='" << mi.arena
//     << "' ordblks='" << mi.ordblks
//     << "' smblks='" << mi.smblks
//     << "' hblks='" << mi.hblks
//     << "' hblkhd='" << mi.hblkhd
//     << "' usmblks='" << mi.usmblks
//     << "' fsmblks='" << mi.fsmblks
//     << "' uordblks='" << mi.uordblks
//     << "' fordblks='" << mi.fordblks
//     << "' keepcost='" << mi.keepcost
     << "'";

  // add named memory usage
  logger->xmlout("memory_usage", "",ss.str(), true);
#endif
}


#endif

