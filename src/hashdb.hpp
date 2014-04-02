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
#include <dfxml/src/hash_t.h>
#include <vector>
#include <stdint.h>

/**
 * Version of the hashdb library.
 */
extern "C"
const char* hashdb_version();

// required inside hashdb_t
class hashdb_manager_t;
class hashdb_changes_t;
class logger_t;
class query_by_socket_t;

/**
 * The hashdb library.
 */
class hashdb_t {
  private:
  enum hashdb_modes_t {HASHDB_NONE,
                       HASHDB_IMPORT,
                       HASHDB_SCAN,
                       HASHDB_SCAN_SOCKET};
  const std::string hashdb_dir;
  const hashdb_modes_t mode;
  hashdb_manager_t *hashdb_manager;
  hashdb_changes_t *hashdb_changes;
  logger_t *logger;
  query_by_socket_t *query_by_socket;
  const uint32_t block_size;
  const uint32_t max_duplicates;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;  // mutext protecting database access
#else
  mutable int M;              // placeholder
#endif

  public:
  // data structure for one import element
  template<typename T>
  struct import_element_t {
    T hash;
    std::string repository_name;
    std::string filename;
    uint64_t file_offset;
    import_element_t(T p_hash,
                   std::string p_repository_name,
                   std::string p_filename,
                   uint64_t p_file_offset) :
                          hash(p_hash),
                          repository_name(p_repository_name),
                          filename(p_filename),
                          file_offset(p_file_offset) {
    }
  };

  /**
   * The import input is an array of import_element_t objects
   * to be imported into the hash database.
   */
  typedef std::vector<import_element_t<md5_t> > import_input_md5_t;
  typedef std::vector<import_element_t<sha1_t> > import_input_sha1_t;
  typedef std::vector<import_element_t<sha256_t> > import_input_sha256_t;

  /**
   * The scan input is an array of pairs of uint64_t index values
   * and hash values to be scanned for.
   */
  typedef std::vector<std::pair<uint64_t, md5_t> > scan_input_md5_t;
  typedef std::vector<std::pair<uint64_t, sha1_t> > scan_input_sha1_t;
  typedef std::vector<std::pair<uint64_t, sha256_t> > scan_input_sha256_t;

  /**
   * The scan output is an array of pairs of uint64_t index values
   * and uint32_t count values, where count indicates the number of
   * source entries that contain this value.  The scan output does not
   * contain scan responses for hashes that are not found (count=0).
   */
  typedef std::vector<std::pair<uint64_t, uint32_t> > scan_output_t;

  private:
  // the interface is specific but the implementation is generic
  template<typename T>
  int import_private(const std::vector<import_element_t<T> >& import_input);

  // the interface is specific but the implementation is generic
  template<typename T>
  int scan_private(const std::vector<std::pair<uint64_t, T> >& scan_input,
           scan_output_t& scan_output) const;

  public:
  /**
   * Constructor for importing.
   */
  hashdb_t(const std::string& hashdb_dir,
           const std::string& hashdigest_type,
           uint32_t p_block_size,
           uint32_t p_max_duplicates);

  /**
   * Import MD5 hash.
   */
  int import(const import_input_md5_t& import_input_md5);
  /**
   * Import SHA1 hash.
   */
  int import(const import_input_sha1_t& import_input_sha1);
  /**
   * Import SHA256 hash.
   */
  int import(const import_input_sha256_t& import_input_sha256);

  /**
   * Constructor for scanning.
   */
  hashdb_t(const std::string& path_or_socket);

  /**
   * Scan for MD5 hashes.
   */
  int scan(const scan_input_md5_t& scan_input_md5,
           scan_output_t& scan_output) const;

  /**
   * Scan for SHA1 hashes.
   */
  int scan(const scan_input_sha1_t& scan_input_sha1,
           scan_output_t& scan_output) const;

  /**
   * Scan for SHA256 hashes.
   */
  int scan(const scan_input_sha256_t& scan_input_sha256,
           scan_output_t& scan_output) const;

#ifdef HAVE_CXX11
  hashdb_t(const hashdb_t& other) = delete;
#else
  // don't use this.
  hashdb_t(const hashdb_t& other);
#endif

#ifdef HAVE_CXX11
  hashdb_t& operator=(const hashdb_t& other) = delete;
#else
  // don't use this.
  hashdb_t& operator=(const hashdb_t& other);
#endif

  ~hashdb_t();
};

#endif

