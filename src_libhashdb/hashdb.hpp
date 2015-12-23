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
class hashdb_scan_manager_private_t;
struct timeval;

namespace hashdb {

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* version();

  // ************************************************************
  // types used in interfaces
  // ************************************************************
  // hash_data_t of tuple(binary_hash, file_offset, entropy_label)
  struct hash_data_t {
    std::string binary_hash;
    uint64_t file_offset;
    std::string entropy_label;
    hash_data_t(const std::string& p_binary_hash,
                const uint64_t p_file_offset,
                const std::string& p_entropy_label) :
            binary_hash(p_binary_hash),
            file_offset(p_file_offset),
            entropy_label(p_entropy_label) {
    }
  };

  // hash_data_list_t
  typedef std::vector<hash_data_t> hash_data_list_t;

  // id_offset_pairs_t of vector of pair(source_id, file_offset)
  typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
  typedef std::vector<id_offset_pair_t> id_offset_pairs_t;

  // source_metadata_t of tuple(source_id, filesize, positive_count)
  struct source_metadata_t {
    uint64_t source_id;
    uint64_t filesize;
    uint64_t positive_count;
    source_metadata_t(const uint64_t p_source_id,
                      const uint64_t p_filesize,
                      const uint64_t p_positive_count) :
            source_id(p_source_id),
            filesize(p_filesize),
            positive_count(p_positive_count) {
    }
    source_metadata_t() : source_id(0), filesize(0), positive_count(0) {
    }
  };

  // source_names_t pair<repository_name, filename>
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::vector<source_name_t> source_names_t;

  // ************************************************************
  // misc support interfaces
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
   *   command_string - String to put into the new hashdb log.
   *   Other parameters - Other parameters control hashdb settings.
   *
   * Returns tuple:
   *   True and "" if creation was successful, false and reason if not.
   */
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const uint32_t sector_size,
                     const uint32_t block_size,
                     bool bloom_is_used,
                     uint32_t bloom_M_hash_size,
                     uint32_t bloom_k_hash_functioins,
                     const std::string& command_string);

  /**
   * If the hashdb is new, create it using settings in the other hashdb.
   * Return true and "" if the hashdb already exists or is created
   * successfully.  False if the new hashdb cannot be created.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to create.  The path must not
   *     exist yet.
   *   other_hashdb_dir - Path to the database containing the settings
   *     to copy.
   *   command_string - String to put into the new hashdb log.
   *
   * Returns tuple:
   *   True and "" if not created or created successfully, false and
   *   reason if not.
   */
  std::pair<bool, std::string> create_if_new_hashdb(
                     const std::string& hashdb_dir,
                     const std::string& other_hashdb_dir,
                     const std::string& command_string);

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
                     const std::string& command_string);

  /**
   * Print environment information to the stream.  Specifically, print
   * lines starting with the pound character followed by version information,
   * the command line, the username, if available, and the date.
   */
  void print_environment(const std::string& command_line, std::ostream& os);
 
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
     *   command_string - This string will be logged in the log file that is
     *      opened for this session.
     */
    import_manager_t(const std::string& hashdb_dir,
                     const std::string& whitelist_hashdb_dir,
                     const bool skip_low_entropy,
                     const std::string& command_string);

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
     * Import source metadata and hash data from hash_data_list.  Low
     * entropy and whitelist hashes may be skipped, depending on
     * initialization settings.
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
    void import_source_data(const std::string& file_binary_hash,
                            const uint64_t filesize,
                            const hash_data_list_t& hash_data_list);

    // Returns sizes of LMDB databases in the data store.
    std::string size() const;
  };

  // ************************************************************
  // scan
  // ************************************************************
  /**
   * Manage LMDB scans.  Interfaces should be threadsafe by LMDB design.
   */
  class scan_manager_t {

    private:
    hashdb_scan_manager_private_t* hashdb_scan_manager_private;

    public:
    // do not allow copy or assignment
#ifdef HAVE_CXX11
    scan_manager_t(const scan_manager_t&) = delete;
    scan_manager_t& operator=(const scan_manager_t&) = delete;
#else
    scan_manager_t(const scan_manager_t&) __attribute__ ((noreturn));
    scan_manager_t& operator=(const scan_manager_t&)
                                          __attribute__ ((noreturn));
#endif

    /**
     * Open hashdb for scanning.
     *
     * Parameters:
     *   hashdb_dir - Path to the database to scan against.
     */
    scan_manager_t(const std::string& hashdb_dir);

    /**
     * The destructor closes read-only data store resources.
     */
    ~scan_manager_t();

    /**
     * Find offset pairs associated with this hash.
     * An empty list means no match.
     */
    void find_id_offset_pairs(const std::string& binary_hash,
                              id_offset_pairs_t& id_offset_pairs) const;

    /**
     * Find source names associated with this source file's hash.
     * An empty list means no match.
     */
    void find_source_names(const std::string& file_binary_hash,
                           source_names_t& source_names) const;

    /**
     * Find source file binary hash from source ID.
     * Fail if the requested source ID is not found.
     */
    std::string find_file_binary_hash(const uint64_t source_id) const;

    /**
     * Return first hash and its matches.
     * Hash is "" and id_offset_pairs is empty when DB is empty.
     * Please use the heap for id_offset_pairs since it can get large.
     */
    std::string hash_begin(id_offset_pairs_t& id_offset_pairs) const;

    /**
     * Return next hash and its matches or "" and no pairs if at end.
     * Fail if called and already at end.
     * Please use the heap for id_offset_pairs since it can get large.
     */
    std::string hash_next(const std::string& last_binary_hash,
                          id_offset_pairs_t& id_offset_pairs) const;

    /**
     * Return first file_binary_hash and its metadata.
     */
    std::pair<std::string, source_metadata_t> source_begin() const;

    /**
     * Return next file_binary_hash and its metadata or "" and zeros if at end.
     * Fail if called and already at end.
     */
    std::pair<std::string, source_metadata_t> source_next(
                           const std::string& last_file_binary_hash) const;

    /**
     * Return sizes of LMDB databases.
     */
    std::string size() const;
  };

  // ************************************************************
  // timestamp
  // ************************************************************
  /**
   * Provide a timestamp service.
   */
  class timestamp_t {

    private:
    struct timeval* t0;
    struct timeval* t_last_timestamp;

    public:

    /**
     * Create a timestamp service.
     */
    timestamp_t();

    /**
     * Release timestamp resources.
     */
    ~timestamp_t();

    // do not allow copy or assignment
#ifdef HAVE_CXX11
    timestamp_t(const timestamp_t&) = delete;
    timestamp_t& operator=(const timestamp_t&) = delete;
#else
    timestamp_t(const timestamp_t&) __attribute__ ((noreturn));
    timestamp_t& operator=(const timestamp_t&) __attribute__ ((noreturn));
#endif

    /**
     * Create a named timestamp and return a JSON string in format
     * {"name":"name", "delta":delta, "total":total}.
     */
    std::string stamp(const std::string &name);
  };
}

#endif

