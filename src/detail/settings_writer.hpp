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
 * Provides the service of writing settings to the hashdb.
 */

#ifndef SETTINGS_WRITER_HPP
#define SETTINGS_WRITER_HPP
#include "hashdb_types.h"
#include "hashdb_settings.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "dfxml/src/dfxml_writer.h" // for writing dfxml

extern std::string command_line;

/**
 * hashdb settings.
 */
namespace hashdb_settings {
  void write_settings(const std::string& hashdb_dir,
                      const hashdb_settings_t& settings) {

    std::string filename = hashdb_filenames_t::settings_filename(hashdb_dir);

    dfxml_writer x(filename, false);
    x.push("settings");
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line);
    settings.report_settings(x);
    x.pop();
  }
};

#endif

