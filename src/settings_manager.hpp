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
 * Manage reading and writing of settings_t.
 */

#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP
#include "settings.hpp"
#include "hashdb_settings_reader.hpp"
#include "command_line.hpp"
#include <string>

#include "dfxml/src/dfxml_writer.h" // for writing dfxml

class settings_manager_t {
  private:
  settings_manager_t();

  public:
  // read settings using hashdb_settings_reader
  static settings_t read_settings(const std::string& hashdb_dir) {
    // create a settings object, read into it, and return it
    settings_t settings;
    std::string filename = hashdb_dir + "/settings.xml";
    hashdb_settings_reader_t::read_settings(filename, settings);
    return settings;
  }

  // write settings using dfxml_writer
  static void write_settings(const std::string& hashdb_dir,
                             const settings_t& settings) {
    std::string filename = hashdb_dir + "/settings.xml";

    dfxml_writer x(filename, false);
    x.push("settings");
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION,
                        "svn not tracked",
                        command_line_t::command_line_string);
    settings.report_settings(x);
    x.pop();
  }
};

#endif

