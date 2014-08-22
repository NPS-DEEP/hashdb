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
 * Manage creating the hashdb directory
 */

#ifndef HASHDB_DIRECTORY_MANAGER_HPP
#define HASHDB_DIRECTORY_MANAGER_HPP

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
#include <string>

class hashdb_directory_manager_t {
  public:

  static void create_new_hashdb_dir(const std::string& hashdb_dir) {

    // hashdb_dir must not exist yet
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      std::cerr << "Error: Path '" << hashdb_dir << "' is not empty.  Cannot continue.\n";
      exit(1);
    }

    // create hashdb_dir
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

  static bool is_hashdb_dir(const std::string& hashdb_dir) {
    std::string settings_filename = hashdb_dir + "/settings.xml";
    return (access(settings_filename.c_str(), F_OK) == 0);
  }
};

#endif

