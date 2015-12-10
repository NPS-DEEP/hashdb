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

class hashdb_import_manager_private_t;

namespace hashdb {

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* hashdb_version();

  // ************************************************************
  // types used in interfaces
  // ************************************************************
  // hash_data_t of tuple(binary_hash, file_offset, entropy_label)
  class hash_data_t {
    public:
    std::string binary_hash;
    uint64_t file_offset;
    std::string entropy_label;
    hash_data_t(const std::string& p_binary_hash,
                uint64_t p_file_offset,
                const std::string& p_entropy_label) :
            binary_hash(p_binary_hash),
            file_offset(p_file_offset),
            entropy_label(p_entropy_label) {
    }
  };

  // hash_data_list_t
  typedef std::vector<hash_data_t> hash_data_list_t;

  // ************************************************************
  // misc tools
  // ************************************************************
  /**
   * Return true and "" if hashdb is valid, false and reason if not.
   */
  std::pair<bool, std::string> is_valid_hashdb(
                                      const std::string& hashdb_dir);

  /**
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir);

  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const std::string& from_hashdb_dir);

  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size);

  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     bool bloom_is_used,
                     uint32_t bloom_M_hash_size,
                     uint32_t bloom_k_hash_functioins);

  // ************************************************************
  // import
  // ************************************************************
  /**
   * Manage all LMDB updates.  All interfaces are locked.
   * Several types of change events are noted in hashdb_changes_t.
   * A logger is opened for logging the command (TBD) and for logging
   * timestamps.  Upon closure, changes are written to the logger
   * and the logger is closed.
   */
  class import_manager_t {

    private:
    hashdb_import_manager_private_t* hashdb_import_manager_private;

    public:

    // do not allow copy or assignment
#ifdef HAVE_CXX11
    import_manager_t(const import_manager_t&) = delete;
    import_manager_t& operator=(const import_manager_t&) = delete;
#else
    import_manager_t(const import_manager_t&) __attribute__ ((noreturn));
    import_manager_t& operator=(const import_manager_t&)
                                            __attribute__ ((noreturn));
#endif

    public:
    import_manager_t(const std::string& p_hashdb_dir,
                     const std::string& p_whitelist_hashdb_dir,
                     const bool p_import_low_entropy);

    ~import_manager_t();

    /**
     * Initialize the environment for this file hash.  Import name if new.
     * Return true if block hashes need to be imported for this file.
     * Return false if block hashes have already been imported for this file.
     */
    bool import_source_name(const std::string& file_binary_hash,
                            const std::string& repository_name,
                            const std::string& filename);

    /**
     * Import hashes from hash_data_list.  Fail on error.
     *
     * If import_low_entropy is false then skip hashes with entropy flags.
     * In the future, a non-entropy data classifier flag may be added
     * to hash_data_t.
     *
     */
    void import_source_hashes(const std::string& file_binary_hash,
                              const uint64_t filesize,
                              const hash_data_list_t& hash_data_list);

    // Sizes of LMDB databases.
    std::string size() const;
  };

} // namespace hashdb

#endif

