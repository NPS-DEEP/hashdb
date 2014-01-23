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
 * Provides the main entry for the hashdb_manager tool.
 */

#ifndef HASHDB_RUNTIME_OPTIONS_HPP
#define HASHDB_RUNTIME_OPTIONS_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>
#include <boost/lexical_cast.hpp>
#include <getopt.h>

struct hashdb_runtime_options_t {
  std::string repository_name;
  size_t exclude_duplicates_count;
  std::string server_service_type;
  std::string server_path;
  std::string server_socket;

  hashdb_runtime_options_t() :
         repository_name(""),
         exclude_duplicates_count(0),
         server_service_type("path"), // not "socket"
         server_path(""),
         server_socket("tcp://*:14500") {
  }
};

#endif

