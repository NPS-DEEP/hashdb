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
 * Manage reading and writing of hashdb_settings_t.
 *
 * File settings.xml is the first to read or write, so strong file checks
 * are present.
 */

#ifndef HASHDB_SETTINGS_MANAGER_HPP
#define HASHDB_SETTINGS_MANAGER_HPP

// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif
#include <sys/stat.h>
#include "hashdb_settings.hpp"
#include "hashdb_settings_reader.hpp"
#include "command_line.hpp"
#include <string>

#include "dfxml/src/dfxml_writer.h" // for writing dfxml

class hashdb_settings_manager_t {
  private:
  hashdb_settings_manager_t();

  public:

  // write settings using dfxml_writer
  static void write_settings(const std::string& hashdb_dir,
                             const hashdb_settings_t& settings) {

    // if hashdb_dir does not exist, create it
    if (access(hashdb_dir.c_str(), F_OK) != 0) {

#ifdef WIN32
      if(mkdir(hashdb_dir.c_str())){
        std::cerr << "Error: Could not make new hashdb directory '"
                  << hashdb_dir << "'.\nCannot continue.\n";
        exit(1);
      }
#else
      if(mkdir(hashdb_dir.c_str(),0777)){
        std::cerr << "Error: Could not make new hashdb directory '"
                  << hashdb_dir << "'.\nCannot continue.\n";
        exit(1);
      }
#endif

    }

    // the settings filename
    std::string filename = hashdb_dir + "/settings.xml";

    // if settings.xml exists, then fail
    if (access(filename.c_str(), F_OK) == 0) {
      std::cerr << "Error: hashdb already exists at '" << hashdb_dir
                << "'.\nCannot continue.\n";
      exit(1);
    }

    // good, write settings to new settings file
    dfxml_writer x(filename, false);
    x.push("settings");
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION,
                        "svn not tracked",
                        command_line_t::command_line_string);
    settings.report_settings(x);
    x.pop();
  }

  // replace existing settings
  static void replace_settings(const std::string& hashdb_dir,
                               const hashdb_settings_t& settings) {

    // the settings file
    std::string filename = hashdb_dir + "/settings.xml";

    // settings.xml must exist in order to replace it
    if (access(filename.c_str(), F_OK) != 0) {
      std::cerr << "Error: invalid hashdb directory at '" << hashdb_dir
                << "'.\nDoes settings.xml.backup exist?\n"
                << "Cannot continue.\n";
      exit(1);
    }

    // rename the old existing settings file to settings.xml.backup
    std::string filename_backup = filename + ".backup";
    std::rename(filename.c_str(), filename_backup.c_str());

    // write the new settings
    write_settings(hashdb_dir, settings);
  }

  // read hashdb settings using hashdb_settings_reader
  static hashdb_settings_t read_settings(const std::string& hashdb_dir) {

    // the settings file
    std::string filename = hashdb_dir + "/settings.xml";

    // settings.xml must exist in order to read it
    if (access(filename.c_str(), F_OK) != 0) {
      std::cerr << "Error: invalid hashdb directory at '" << hashdb_dir
                << "'.\nThe settings.xml file does not exist.\n"
                << "Cannot continue.\n";
      exit(1);
    }

    // create a settings object, read into it, and return it
    hashdb_settings_t settings;
    hashdb_settings_reader_t::read_settings(filename, settings);
    return settings;
  }
};

#endif

