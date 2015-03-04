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
 * Header file for the hashdb library.
 */

#ifndef HASHDB_HPP
#define HASHDB_HPP

#include <string>
#include <vector>
#include <cstdint>

/**
 * Version of the hashdb library.
 */
extern "C"
const char* hashdb_version();

// required inside hashdb_t
class lmdb_rw_manager_t;
class lmdb_ro_manager_t;
class logger_t;

/**
 * The hashdb library.
 */
class hashdb_t {
  private:
  enum hashdb_modes_t {HASHDB_NONE,
                       HASHDB_IMPORT,
                       HASHDB_SCAN,
                       HASHDB_SCAN_SOCKET};
  std::string path_or_socket;
  uint32_t block_size;
  uint32_t max_duplicates;
  hashdb_modes_t mode;
  lmdb_rw_manager_t* rw_manager;
  lmdb_ro_manager_t* ro_manager;
  logger_t* logger;

  public:
  /**
   * Constructor
   */
  hashdb_t();

  /**
   * Open for importing, return true else false with error string.
   */
  std::pair<bool, std::string> open_import(const std::string& p_hashdb_dir,
                                           uint32_t p_block_size,
                                           uint32_t p_max_duplicates);

  /**
   * Import hash.
   */
  int import(const std::string& binary_hash,
             const uint64_t file_offset,
             const std::string& repository_name,
             const std::string& filename,
             const uint64_t filesize,
             const std::string& file_binary_hash,
             const std::string& block_hash_label);

  /**
   * Open for scanning.
   * Return true else false with error string.
   */
  std::pair<bool, std::string> open_scan(const std::string& p_path_or_socket);

  /**
   * Scan.
   */
  int scan(const std::string& binary_hash,
           uint32_t& count) const;

#ifdef HAVE_CXX11
  hashdb_t(const hashdb_t& other) = delete;
#else
  // don't use this.
  hashdb_t(const hashdb_t& other) __attribute__ ((noreturn));
#endif

#ifdef HAVE_CXX11
  hashdb_t& operator=(const hashdb_t& other) = delete;
#else
  // don't use this.
  hashdb_t& operator=(const hashdb_t& other) __attribute__ ((noreturn));
#endif

  ~hashdb_t();
};

#endif

