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
#include <stdint.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/**
 * Version of the hashdb library.
 */
extern "C"
const char* hashdb_version();

// required inside hashdb_t__
class hashdb_manager_t;
class hashdb_changes_t;
class logger_t;
class tcp_client_manager_t;

/**
 * The hashdb library.
 *
 * Note: libhashdb must be compiled to support the same hash type
 * as the hash type provided in the template.
 */
template<typename T>
class hashdb_t__ {
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
  tcp_client_manager_t *tcp_client_manager;
  const uint32_t block_size;
  const uint32_t max_duplicates;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;  // mutext protecting database access
#else
  mutable int M;              // placeholder
#endif

  public:
  // data structure for one import element
  struct import_element_t {
    T hash;
    std::string repository_name;
    std::string filename;
    uint64_t file_offset;
    import_element_t(const T p_hash,
                     const std::string p_repository_name,
                     const std::string p_filename,
                     uint64_t p_file_offset) :
                            hash(p_hash),
                            repository_name(p_repository_name),
                            filename(p_filename),
                            file_offset(p_file_offset) {
    }
    import_element_t() :
                            hash(),
                            repository_name(),
                            filename(),
                            file_offset(0) {
    }
  };

  /**
   * The import input is an array of import_element_t objects
   * to be imported into the hash database.
   */
  typedef std::vector<import_element_t> import_input_t;

  /**
   * The scan input is an array of hash values to be scanned for.
   */
  typedef std::vector<T> scan_input_t;

  /**
   * The scan output is an array of pairs of uint32_t index values that
   * index into the input vector, and uint32_t count values, where count
   * indicates the number of source entries that contain this hash value.
   * The scan output does not contain scan responses for hashes
   * that are not found (count=0).
   */
  typedef std::vector<std::pair<uint32_t, uint32_t> > scan_output_t;

  /**
   * Constructor for importing.
   */
  hashdb_t__(const std::string& hashdb_dir,
             uint32_t p_block_size,
             uint32_t p_max_duplicates);

  /**
   * Import.
   */
  int import(const import_input_t& import_input);

  /**
   * Import specific source metadata.
   */
  int import_metadata(const std::string& repository_name,
                      const std::string& filename,
                      uint64_t file_size,
                      T file_hash);

  /**
   * Constructor for scanning.
   */
  hashdb_t__(const std::string& path_or_socket);

  /**
   * Scan.
   */
  int scan(const scan_input_t& scan_input,
           scan_output_t& scan_output) const;

#ifdef HAVE_CXX11
  hashdb_t__(const hashdb_t__& other) = delete;
#else
  // don't use this.
  hashdb_t__(const hashdb_t__& other) __attribute__ ((noreturn));
#endif

#ifdef HAVE_CXX11
  hashdb_t__& operator=(const hashdb_t__& other) = delete;
#else
  // don't use this.
  hashdb_t__& operator=(const hashdb_t__& other) __attribute__ ((noreturn));
#endif

  ~hashdb_t__();
};

#endif

