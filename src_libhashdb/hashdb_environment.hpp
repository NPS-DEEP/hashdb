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
 * Create a new hashdb.
 */

#ifndef HASHDB_ENVIRONMENT_HPP
#define HASHDB_ENVIRONMENT_HPP

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

#include <unistd.h>
#include <iostream>
#include <string>

namespace hashdb {
  /**
   * Print environment information to the stream.
   */
  void print_environment(const std::string& command_line, std::ostream& os) {
    // version
    os << "# libhashdb version " << PACKAGE_VERSION;
#ifdef GIT_COMMIT
    os << ", GIT commit " << GIT_COMMIT;
#endif
    os << "\n";

    // command
    os << "# command " << command_line << "\n";

    // username
#ifdef HAVE_GETPWUID
    os << "# username " << getwpuid(getuid())->pw_name << "\n";
#endif

    // date
#define TM_FORMAT "%Y-%m-%dT%H:%M:%SZ"
    char buf[256];
    time_t t = time(0);
    strftime(buf,sizeof(buf),TM_FORMAT,gmtime(&t));
    os << "# start time " << buf << "\n";
  }

} // namespace hashdb

#endif

