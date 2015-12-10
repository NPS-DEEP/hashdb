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
 *
 * Potential future changes:
 *   A non-entropy data classifier flag may be added to hash_data_t.
 *   Bloom filter support may be discontinued if internal data store
 *     is optimized to support lists.
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
  // global settings
  // ************************************************************
  static const uint32_t hashdb_settings_version = 3;
  static const uint32_t default_sector_size = 512;
  static const uint32_t default_block_size = 512;
  static const bool default_bloom_is_used = true;
  static const uint32_t default_bloom_M_hash_size = 28;
  static const uint32_t default_bloom_k_hash_functions = 3;
 
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
  // misc support
  // ************************************************************
  /**
   * Return true and "" if hashdb is valid, false and reason if not.
   */
  std::pair<bool, std::string> is_valid_hashdb(
                                      const std::string& hashdb_dir);

  /**
   * Create a new hashdb.
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to create.  The path must not
   *     exist yet.
   *   log_string - String to put into the new hashdb log.
   *   Other parameters - Other parameters control hashdb settings.
   *
   * Returns tuple:
   *   True and "" if creation was successful, false and reason if not.
   */
  // create with default settings
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const std::string& log_string);

  // create with settings copied from another hashdb
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const std::string& from_hashdb_dir,
                     const std::string& log_string);

  // create with specified and default settings
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     const std::string& log_string);

  // create with specified settings
  std::pair<bool, std::string> create_hashdb(const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     bool bloom_is_used,
                     uint32_t bloom_M_hash_size,
                     uint32_t bloom_k_hash_functioins,
                     const std::string& log_string);
  /**
   * Rebuild the Bloom filter.
   * Return true and "" if bloom filter rebuilds, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  std::pair<bool, std::string> rebuild_bloom(
                     const std::string& hashdb_dir,
                     const bool bloom_is_used,
                     const uint32_t bloom_M_hash_size,
                     const uint32_t bloom_k_hash_functions,
                     const std::string& log_string);

  // ************************************************************
  // import
  // ************************************************************
  /**
   * Manage all LMDB updates.  All interfaces are locked and threadsafe.
   * A logger is opened for logging the command (TBD) and for logging
   * timestamps and changes applied during the session.  Upon closure,
   * changes are written to the logger and the logger is closed.
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

    /**
     * Open hashdb for importing.
     *
     * Parameters:
     *   hashdb_dir - Path to the database to append to.
     *   whitelist_hashdb_dir - Path to a whitelist database for skipping
     *     whitelist hashes.  To supppress, use "".
     *   skip_low_entropy - True skips flagged hashes, False imports them.
     *   log_string - This string will be logged in the log file that is
     *      opened for this session.
     */
    import_manager_t(const std::string& hashdb_dir,
                     const std::string& whitelist_hashdb_dir,
                     const bool skip_low_entropy,
                     const std::string& log_string);

    /**
     * The destructor closes the log file and data store resources.
     */
    ~import_manager_t();

    /**
     * Initialize the environment to accept this file hash and import
     * the repository name and filename if this source is not present yet.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   repository_name - The repository name to attribute the source to.
     *   filename - The filename to attribute the source to.
     *
     * Returns:
     *   True if block hashes need to be imported for this source, false
     *   if block hashes have already been imported for this source.
     */
    bool import_source_name(const std::string& file_binary_hash,
                            const std::string& repository_name,
                            const std::string& filename);

    /**
     * Import source hashes from hash_data_list.  Low entropy and whitelist
     * hashes may be skipped, depending on initialization settings.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   filesize - The size of the source, in bytes.
     *   hash_data_list - The list of hashes to import, see hash_data_list_t.
     *
     * Returns:
     *   Nothing, but fails if import_source_name has not been called yet
     *     for this source, so be sure to call import_source_name first.
     */
    void import_source_hashes(const std::string& file_binary_hash,
                              const uint64_t filesize,
                              const hash_data_list_t& hash_data_list);

    // Returns sizes of LMDB databases in the data store.
    std::string size() const;
  };

}

#endif

