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
 * Class file for the hashdb library.
 */

#include <config.h>
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
#include "hashdb.hpp"
#include "hashdb_tools.hpp"
#include "hashdb_import_manager_private.hpp"
#include <string>
#include <vector>
#include <stdint.h>
#include <climits>
#ifndef HAVE_CXX11
#include <cassert>
#endif

namespace hashdb {

// ************************************************************
// version of the hashdb library
// ************************************************************
/**
 * version of the hashdb library
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

// ************************************************************
// import
// ************************************************************
import_manager_t::import_manager_t(
                   const std::string& p_hashdb_dir,
                   const std::string& p_whitelist_hashdb_dir,
                   const bool p_import_low_entropy,
                   const std::string& p_log_string) :
            hashdb_import_manager_private(new hashdb_import_manager_private_t(
                                          p_hashdb_dir,
                                          p_whitelist_hashdb_dir,
                                          p_import_low_entropy,
                                          p_log_string)) {
}

import_manager_t::~import_manager_t() {
  delete hashdb_import_manager_private;
}

bool import_manager_t::import_source_name(
                            const std::string& file_binary_hash,
                            const std::string& repository_name,
                            const std::string& filename) {
  return hashdb_import_manager_private->import_source_name(
                               file_binary_hash, repository_name, filename);
}

void import_manager_t::import_source_hashes(
                            const std::string& file_binary_hash,
                            const uint64_t filesize,
                            const hashdb::hash_data_list_t& hash_data_list) {
  return hashdb_import_manager_private->import_source_hashes(
                               file_binary_hash, filesize, hash_data_list);
}

std::string import_manager_t::size() const {
  return hashdb_import_manager_private->size();
}

// ************************************************************
// export
// ************************************************************
// zz

} // end namespace hashdb
