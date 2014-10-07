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
 * Manage creating, erasing, validating presence of hashdb_dir.
 */

#ifndef DIRECTORY_HELPER_HPP
#define DIRECTORY_HELPER_HPP

// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
#include <io.h>
// including winsock2.h now keeps an included header somewhere from
// including windows.h first, resulting in a warning.
#include <winsock2.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>

void make_dir_if_not_there(const std::string& temp_dir) {
  // if temp_dir does not exist, create it
  if (access(temp_dir.c_str(), F_OK) != 0) {
#ifdef WIN32
    int status = mkdir(temp_dir.c_str());
#else
    int status = mkdir(temp_dir.c_str(),0777);
#endif
    if (status != 0) {
      std::cout << "unable to create dir " << temp_dir << ": "<< strerror(status) << "\n";
    }
  }
}

void rm_hashdb_dir(const std::string& hashdb_dir) {
  remove((hashdb_dir + "/bloom_filter_1").c_str());
  remove((hashdb_dir + "/bloom_filter_2").c_str());
  remove((hashdb_dir + "/hash_store").c_str());
  remove((hashdb_dir + "/history.xml").c_str());
  remove((hashdb_dir + "/_old_history.xml").c_str());
  remove((hashdb_dir + "/log.xml").c_str());
  remove((hashdb_dir + "/settings.xml").c_str());
  remove((hashdb_dir + "/_old_settings.xml").c_str());
  remove((hashdb_dir + "/source_filename_store.dat").c_str());
  remove((hashdb_dir + "/source_filename_store.idx1").c_str());
  remove((hashdb_dir + "/source_filename_store.idx2").c_str());
  remove((hashdb_dir + "/source_lookup_store.dat").c_str());
  remove((hashdb_dir + "/source_lookup_store.idx1").c_str());
  remove((hashdb_dir + "/source_lookup_store.idx2").c_str());
  remove((hashdb_dir + "/source_repository_name_store.dat").c_str());
  remove((hashdb_dir + "/source_repository_name_store.idx1").c_str());
  remove((hashdb_dir + "/source_repository_name_store.idx2").c_str());
  remove((hashdb_dir + "/source_metadata_store").c_str());
  remove((hashdb_dir + "/temp_dfxml_output").c_str());

  if (access(hashdb_dir.c_str(), F_OK) == 0) {
    // dir exists so remove it
    int status = rmdir(hashdb_dir.c_str());
    if (status != 0) {
      std::cout << "unable to remove hashdb_dir " << hashdb_dir << ": "<< strerror(status) << "\n";
    }
  }
}

#endif

