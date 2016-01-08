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
  remove((hashdb_dir + "/bloom_filter").c_str());

  remove((hashdb_dir + "/lmdb_hash_data_store/data.mdb").c_str());
  remove((hashdb_dir + "/lmdb_hash_data_store/lock.mdb").c_str());
  rmdir((hashdb_dir + "/lmdb_hash_data_store").c_str());

  remove((hashdb_dir + "/lmdb_hash_store/data.mdb").c_str());
  remove((hashdb_dir + "/lmdb_hash_store/lock.mdb").c_str());
  rmdir((hashdb_dir + "/lmdb_hash_store").c_str());

  remove((hashdb_dir + "/lmdb_source_data_store/data.mdb").c_str());
  remove((hashdb_dir + "/lmdb_source_data_store/lock.mdb").c_str());
  rmdir((hashdb_dir + "/lmdb_source_data_store").c_str());

  remove((hashdb_dir + "/lmdb_source_id_store/data.mdb").c_str());
  remove((hashdb_dir + "/lmdb_source_id_store/lock.mdb").c_str());
  rmdir((hashdb_dir + "/lmdb_source_id_store").c_str());

  remove((hashdb_dir + "/lmdb_source_name_store/data.mdb").c_str());
  remove((hashdb_dir + "/lmdb_source_name_store/lock.mdb").c_str());
  rmdir((hashdb_dir + "/lmdb_source_name_store").c_str());

  remove((hashdb_dir + "/log.txt").c_str());
  remove((hashdb_dir + "/settings.json").c_str());
  remove((hashdb_dir + "/_old_settings.json").c_str());

  if (access(hashdb_dir.c_str(), F_OK) == 0) {
    // dir exists so remove it
    int status = rmdir(hashdb_dir.c_str());
    if (status != 0) {
      std::cout << "unable to remove hashdb_dir " << hashdb_dir << ": "<< strerror(status) << "\n";
      exit(1);
    }
  }
}

static void require_no_dir(const std::string& dirname) __attribute__((unused));
static void require_no_dir(const std::string& dirname) {
  if (access(dirname.c_str(), F_OK) == 0) {
    std::cerr << "Error: Path '" << dirname << "' already exists.  Cannot continue.\n";
    exit(1);
  }
}

static void create_new_dir(const std::string& new_dir) __attribute__((unused));
static void create_new_dir(const std::string& new_dir) {

  // new_dir must not exist yet
  require_no_dir(new_dir);

  // create new_dir
  int status;
#ifdef WIN32
  status = mkdir(new_dir.c_str());
#else
  status = mkdir(new_dir.c_str(),0777);
#endif
  if (status != 0) {
    std::cerr << "Error: Could not create new directory '"
              << new_dir << "'.\nCannot continue.\n";
    exit(1);
  }
}

#endif

