// Author:  Bruce Allen
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
 * NOTE: This file includes SWIG preprocessor directives used for
 * building Python bindings.  Specifically:
 *   SWIG is not defined when building C++.
 *   SWIG is defined when building Python bindings.
 */

#ifndef HASHDB_HPP
#define HASHDB_HPP

#include <string>
#include <set>
#include <stdint.h>
#include <sys/time.h>   // timeval* for timestamp_t
#include <pthread.h>    // pthread_t* for scan_stream_t

// ************************************************************
// version of the hashdb library
// ************************************************************
/**
 * Version of the hashdb library, same as hashdb::version.
 */
extern "C"
const char* hashdb_version();

namespace scan_stream {
  class scan_thread_data_t;
}
namespace hashdb {
  class lmdb_hash_data_manager_t;
  class lmdb_hash_manager_t;
  class lmdb_source_data_manager_t;
  class lmdb_source_id_manager_t;
  class lmdb_source_name_manager_t;
  class lmdb_changes_t;
  class logger_t;
  class locked_member_t;

  // ************************************************************
  // typedefs
  // ************************************************************
  // pair(file_binary_hash, file_offset)
  typedef std::pair<std::string, uint64_t>    source_offset_pair_t;
  typedef std::set<source_offset_pair_t> source_offset_pairs_t;

  // pair(repository_name, filename)
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::set<source_name_t>             source_names_t;

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* version();

  // ************************************************************
  // settings
  // ************************************************************
  /**
   * Provides hashdb settings.
   *
   * Attributes:
   *   settings_version - The version of the settings record
   *   byte_alignment - Minimal step size of data, in bytes.  Blocks must
   *     align to this.
   *   block_size - Size, in bytes, of data blocks.
   *   max_source_offset_pairs - The maximum number of source hash,
   *     file offset pairs to store for a single hash value.
   *   hash_prefix_bits - The number of hash prefix bits to use as the
   *     key in the optimized hash storage.
   *   hash_suffix_bytes - The number of hash suffix bytes to use as the
   *     value in the optimized hash storage.
   */
  struct settings_t {
#ifndef SWIG
    static const uint32_t CURRENT_SETTINGS_VERSION = 3;
#endif
    uint32_t settings_version;
    uint32_t byte_alignment;
    uint32_t block_size;
    uint32_t max_source_offset_pairs;
    uint32_t hash_prefix_bits;
    uint32_t hash_suffix_bytes;
    settings_t();
    std::string settings_string() const;
  };

  // ************************************************************
  // scan modes
  // ************************************************************
  /**
   * The scan mode controls scan optimization and returned JSON content.
   */
  enum scan_mode_t {EXPANDED,
                    EXPANDED_OPTIMIZED,
                    COUNT_ONLY,
                    APPROXIMATE_COUNT};

  // ************************************************************
  // misc support interfaces
  // ************************************************************
  /**
   * Create a new hashdb.
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to create.  The path must not
   *     exist yet.
   *   settings - The hashdb settings.
   *   command_string - String to put into the new hashdb log.
   *
   * Returns:
   *   "" if successful else reason if not.
   */
  std::string create_hashdb(const std::string& hashdb_dir,
                            const hashdb::settings_t& settings,
                            const std::string& command_string);

  /**
   * Return hashdb settings else reason for failure.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to obtain the settings of.
   *   settings - The hashdb settings.
   *
   * Returns:
   *   True and "" if settings were retrieved, false and reason if not.
   */
  std::string read_settings(const std::string& hashdb_dir,
#ifdef SWIG
                            hashdb::settings_t& OUTPUT
#else
                            hashdb::settings_t& settings
#endif
                           );

  /**
   * Return binary string or empty if hexdigest length is not even
   * or has any invalid digits.
   */
  std::string hex_to_bin(const std::string& hex_string);

  /**
   * Return hexadecimal representation of the binary string.
   */
  std::string bin_to_hex(const std::string& binary_hash);

  /**
   * Calculate and ingest hashes from files recursively from a source
   * path.  Files with EWF extensions (.E01 files) will be ingested as
   * media images.
   *
   * Parameters:
   *   hashdb_dir - Path to the hashdb data store to import into.
   *   ingest_path - Path to a source file or directory to recursively
   *     ingest block hashes from.  May include E01 files.
   *   step_size - The step size to move along while calculating hashes.
   *     The step size must be divisible by the byte alignment defined in
   *     the database.
   *   repository_name - A repository name to attribute the sources to.
   *   whitelist_dir - Path to a whitelist hashdb data store.  Hashes
   *     matching these will not be ingested.
   *   disable_recursive_processing - Disable processing embedded data.
   *   disable_calculate_entropy - Disable calculating block entropy values.
   *   disable_calculate_labels - Disable calculating block entropy labels.
   *   command_string - String to put into the new hashdb log.
   *
   * Returns:
   *   "" if successful else reason if not.
   */
  std::string ingest(const std::string& hashdb_dir,
                     const std::string& ingest_path,
                     const size_t step_size,
                     const std::string& repository_name,
                     const std::string& whitelist_dir,
                     const bool disable_recursive_processing,
                     const bool disable_calculate_entropy,
                     const bool disable_calculate_labels,
                     const std::string& command_string);

  /**
   * Calculate and scan for hashes from the media image file.  Files with
   * EWF extensions (.E01 files) are recognized as media images.
   *
   * Parameters:
   *   hashdb_dir - Path to the hashdb data store to scan against.
   *   media_image_file - Path to a media image file, which can be a
   *     raw file or an E01 file.
   *   step_size - The step size to move along while calculating hashes.
   *     The step size must be divisible by the byte alignment defined in
   *     the database.
   *   disable_recursive_processing - Disable processing embedded data.
   *   scan_mode - The mode to use for performing the scan.  Controls
   *     scan optimization and returned JSON content.
   *
   * Returns:
   *   "" if successful else reason if not.
   */
  std::string scan_image(const std::string& hashdb_dir,
                     const std::string& media_image_file,
                     const size_t step_size,
                     const bool disable_recursive_processing,
                     const hashdb::scan_mode_t scan_mode);

  /**
   * Read raw bytes at the forensic path in the media image file.  Files
   * with EWF extensions (.E01 files) are recognized as media images.
   * Example forensic paths are "1000" and "1000-zip-0".
   *
   * Parameters:
   *   media_image_file - Path to a media image file, which can be a
   *     raw file or an E01 file.
   *   forensic_path - The offset into the media image file.
   *     Example forensic paths include "1000" and "1000-zip-0".
   *   count - The number of bytes to read.
   *
   * Returns:
   *   "" if successful else reason if not.
   */

  std::string read_bytes(const std::string& media_image_file,
                         const std::string& forensic_path,
                         const uint64_t count,
#ifndef SWIG
                         std::string& bytes
#else
                         std::string& OUTPUT // bytes
#endif
                        );

  /**
   * Read raw bytes at the given offset in the media image file.  Files
   * with EWF extensions (.E01 files) are recognized as media images.
   *
   * Parameters:
   *   media_image_file - Path to a media image file, which can be a
   *     raw file or an E01 file.
   *   offset - The offset into the media image file.
   *   forensic_path - The offset into the media image file.
   *     Example forensic paths include "1000" and "1000-zip-0".
   *   count - The number of bytes to read.
   *
   * Returns:
   *   "" if successful else reason if not.
   */
  std::string read_bytes(const std::string& media_image_file,
                         const uint64_t offset,
                         const uint64_t count,
#ifndef SWIG
                         std::string& bytes
#else
                         std::string& OUTPUT // bytes
#endif
                        );

  // ************************************************************
  // import
  // ************************************************************
  /**
   * Manage all LMDB updates.  All interfaces are locked and threadsafe.
   * A logger is opened for logging the command and for logging
   * timestamps and changes applied during the session.  Upon closure,
   * changes are written to the logger and the logger is closed.
   */
  class import_manager_t {

    private:
    lmdb_hash_data_manager_t* lmdb_hash_data_manager;
    lmdb_hash_manager_t* lmdb_hash_manager;
    lmdb_source_data_manager_t* lmdb_source_data_manager;
    lmdb_source_id_manager_t* lmdb_source_id_manager;
    lmdb_source_name_manager_t* lmdb_source_name_manager;

    logger_t* logger;
    hashdb::lmdb_changes_t* changes;

    public:
#ifndef SWIG
    // do not allow copy or assignment
    import_manager_t(const import_manager_t&) = delete;
    import_manager_t& operator=(const import_manager_t&) = delete;
#endif

    /**
     * Open hashdb for importing.
     *
     * Parameters:
     *   hashdb_dir - Path to the hashdb data store to import into.
     *   command_string - String to put into the new hashdb log.
     */
    import_manager_t(const std::string& hashdb_dir,
                     const std::string& command_string);

    /**
     * The destructor closes the log file and data store resources.
     */
    ~import_manager_t();

    /**
     * Insert the repository_name, filename pair associated with the
     * source.
     */
    void insert_source_name(const std::string& file_binary_hash,
                            const std::string& repository_name,
                            const std::string& filename);


    /**
     * Insert or change source data.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   filesize - The size of the source, in bytes.
     *   file_type - A string representing the type of the file.
     *   zero_count - The count of blocks skipped because they only
     *     contain the zero byte.
     *   nonprobative_count - The count of non-probative hashes
     *     identified for this source.
     */
    void insert_source_data(const std::string& file_binary_hash,
                            const uint64_t filesize,
                            const std::string& file_type,
                            const uint64_t zero_size,
                            const uint64_t nonprobative_count);

    /**
     * Insert or change the hash data associated with the binary_hash.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   file_offset - The byte offset into the file where the hash is
     *     located.
     *   entropy - A numeric entropy value for the associated block.
     *   block_label - Text indicating the type of the block or "" for
     *     no label.
     */
    void insert_hash(const std::string& binary_hash,
                     const std::string& file_binary_hash,
                     const uint64_t file_offset,
                     const float entropy,
                     const std::string& block_label);

    /**
     * Import hash or source information from a JSON record.
     *
     * Parameters:
     *   json_string - Hash or source text in JSON format.
     *
     *   Example hash syntax:
     *     {
     *       "block_hash": "c313ac...",
     *       "entropy": 8,
     *       "block_label": "W",
     *       "source_offset_pairs": ["b9e7...", 4096]
     *     }
     *
     *   Example source syntax:
     *     {
     *       "file_hash": "b9e7...",
     *       "filesize": 8000,
     *       "file_type": "exe",
     *       "zero_count": 1,
     *       "nonprobative_count": 4,
     *       "name_pairs": ["repository1", "filename1", "repo2", "f2"]
     *     }
     *
     * Returns:
     *   "" else error message if JSON is invalid.
     */
    std::string import_json(const std::string& json_string);

    /**
     * Return the sizes of LMDB databases in the data store.
     */
    std::string size() const;

    /**
     * Return the number of hash records.
     */
    size_t size_hashes() const;

    /**
     * Return the number of sources.
     */
    size_t size_sources() const;
  };

  // ************************************************************
  // scan
  // ************************************************************
  /**
   * Manage LMDB scans.  Interfaces should be threadsafe by LMDB design.
   */
  class scan_manager_t {

    private:
    lmdb_hash_data_manager_t* lmdb_hash_data_manager;
    lmdb_hash_manager_t* lmdb_hash_manager;
    lmdb_source_data_manager_t* lmdb_source_data_manager;
    lmdb_source_id_manager_t* lmdb_source_id_manager;
    lmdb_source_name_manager_t* lmdb_source_name_manager;

    // support find_expanded_hash_json when optimizing
    locked_member_t* hashes;
    locked_member_t* sources;

    // low-level find interfaces
    std::string find_expanded_hash_json(const bool optimizing,
                                     const std::string& binary_hash);
    std::string find_hash_count_json(const std::string& binary_hash) const;
    std::string find_approximate_hash_count_json(
                                     const std::string& binary_hash) const;
    public:
#ifndef SWIG
    // do not allow copy or assignment
    scan_manager_t(const scan_manager_t&) = delete;
    scan_manager_t& operator=(const scan_manager_t&) = delete;
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

#ifndef SWIG
    /**
     * Find hash, return pairs in object.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *   entropy - A numeric entropy value for the associated block.
     *   block_label - Text indicating the type of the block or "" for
     *     no label.
     *   source_offset_pairs - Set of pairs of source hash and file
     *     offset values.
     *
     * Returns:
     *   True if the hash is present, false if not.
     */
    bool find_hash(const std::string& binary_hash,
                   float& entropy,
                   std::string& block_label,
                   source_offset_pairs_t& source_offset_pairs) const;
#endif

    /**
     * JSON block_hash export text else "" if hash is not there.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   JSON block_hash export string if hash is present, false and ""
     *   if not.  Example syntax:
     *
     *     {
     *       "block_hash": "c313ac...",
     *       "entropy": 8,
     *       "block_label": "W",
     *       "source_offset_pairs": ["b9e7...", 4096]
     *     }
     */
    std::string export_hash_json(const std::string& binary_hash) const;

    /**
     * JSON file_hash export text else "" if file hash is not there.
     *
     * Parameters:
     *   file_binary_hash - The file hash in binary form.
     *
     * Returns:
     *   JSON file hash export text if file hash is present, false
     *   and "" if not.  Example syntax:
     *
     *     {
     *       "file_hash": "b9e7...",
     *       "filesize": 8000,
     *       "file_type": "exe",
     *       "zero_count": 1,
     *       "nonprobative_count": 4,
     *       "name_pairs": ["repository1", "filename1", "repo2", "f2"]
     *       }
     */
    std::string export_source_json(const std::string& file_binary_hash) const;

    /**
     * Find hash count.  Faster than find_hash.  Accesses the hash
     * information store.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   Approximate hash count.
     */
    size_t find_hash_count(const std::string& binary_hash) const;

    /**
     * Find approximate hash count.  Faster than find_hash, but can be wrong.
     * Accesses the hash store.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   Approximate hash count.
     */
    size_t find_approximate_hash_count(const std::string& binary_hash) const;

    /**
     * Find source data for the given source ID, false on no source ID.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   filesize - The size of the source, in bytes.
     *   file_type - A string representing the type of the file.
     *   zero_count - The count of blocks skipped because they only
     *     contain the zero byte.
     *   nonprobative_count - The count of non-probative hashes
     *     identified for this source.
     *
     * Returns:
     *   True if file binary hash is present.
     */
    bool find_source_data(const std::string& file_binary_hash,
#ifdef SWIG
                          uint64_t& OUTPUT,      // filesize
                          std::string& OUTPUT,   // file_type
                          uint64_t& OUTPUT,      // zero_count
                          uint64_t& OUTPUT       // nonprobative_count
#else
                          uint64_t& filesize,
                          std::string& file_type,
                          uint64_t& zero_count,
                          uint64_t& nonprobative_count
#endif
                         ) const;

#ifndef SWIG
    /**
     * Find source names for the given source ID, false on no source ID.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   source_names - Set of pairs of repository_name, filename
     *     attributed to this source ID.
     *
     * Returns:
     *   True if file binary hash is present.
     */
    bool find_source_names(const std::string& file_binary_hash,
                           source_names_t& source_names) const;
#endif

    /**
     * Find hash, return JSON text else "" if not there.
     *
     * Parameters:
     *   scan_mode - The mode to use for performing the scan.  Controls
     *     scan optimization and returned JSON content.
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   JSON text if hash is present, false and "" if not.  Example syntax
     *   based on mode:
     *     EXPANDED - always return all available data.  Example syntax:
     *       {
     *         "block_hash": "c313ac...",
     *         "entropy": 8,
     *         "block_label": "W",
     *         "source_list_id": 57,
     *         "sources": [{
     *           "file_hash": "f7035a...",
     *           "filesize": 800,
     *           "file_type": "exe",
     *           "zero_count": 1,
     *           "nonprobative_count": 2,
     *           "names": ["repository1", "filename1", "repo2", "f2"]
     *         }],
     *         "source_offset_pairs": ["f7035a...", 0, "f7035a...", 512]
     *       }
     *     EXPANDED_OPTIMIZED - return all available data the first time
     *       but suppress hash and source data after.  Example syntax
     *       when suppressed:
     *       { "block_hash": "c313ac..." }
     *     COUNT_ONLY - Only return the count.  Example syntax:
     *       { "block_hash": "c313ac...", "count": "1" }
     *     APPROXIMATE_COUNT - Return approximate count.  Example syntax:
     *       { "block_hash": "c313ac...", "approximate_count": "1" }
     */
    std::string find_hash_json(const scan_mode_t scan_mode,
                               const std::string& binary_hash);

    /**
     * Return the first block hash in the database.
     *
     * Returns:
     *   binary_hash if a first hash is available else "" if DB is empty.
     */
    std::string first_hash() const;

    /**
     * Return the next block hash in the database.  Error if last hash
     *   does not exist.
     *
     * Parameters:
     *   last_binary_hash - The previous block hash in binary form.
     *
     * Returns:
     *   binary_hash if a next hash is available else "" if at end.
     */
    std::string next_hash(const std::string& binary_hash) const;

    /**
     * Return the file_binary_hash of the first source in the database.
     *
     * Returns:
     *   file_binary_hash if a first source is available else "" if DB
     *   is empty.
     */
    std::string first_source() const;

    /**
     * Return the next source in the database.  Error if last_file_binary_hash
     *   does not exist.
     *
     * Parameters:
     *   last_file_binary_hash - The previous source file hash in binary form.
     *
     * Returns:
     *   next file_binary_hash if a next source is available else "" if at end.
     */
    std::string next_source(const std::string& file_binary_hash) const;

    /**
     * Return the sizes of LMDB databases in JSON format.
     */
    std::string size() const;

    /**
     * Return the number of hash records.
     */
    size_t size_hashes() const;

    /**
     * Return the number of sources.
     */
    size_t size_sources() const;
  };

  // ************************************************************
  // scan_stream
  // ************************************************************
  /**
   * Provide a threaded streaming scan interface.  Use put to enqueue
   * arrays of scan input.  Use get to receive arrays of scan output.
   *
   * If a thread cannot properly parse unscanned data, it will emit a
   * warning to stderr.
   */
  class scan_stream_t {
    private:
    const int num_threads;
    ::pthread_t* threads;
    scan_stream::scan_thread_data_t* scan_thread_data;
    bool done;

#ifndef SWIG
    // do not allow copy or assignment
    scan_stream_t(const scan_stream_t&);
    scan_stream_t& operator=(const scan_stream_t&);
#endif

    public:
    /**
     * Create a streaming scan service.
     *
     * Parameters:
     *   scan_manger - The hashdb scan manager to use for scanning.
     *   hash_size - The size, in bytes, of a binary hash, 16 for MD5.
     *   scan_mode - The mode to use for performing the scan.  Controls
     *     scan optimization and returned JSON content.
     */
    scan_stream_t(hashdb::scan_manager_t* const scan_manager,
                  const size_t hash_size,
                  const hashdb::scan_mode_t scan_mode);

    /**
     * Release scan_stream resources.
     */
    ~scan_stream_t();

    /**
     * Submit a string containing an array of records to scan.
     *
     * Paramters:
     *   unscanned_data - An array of records to scan, packed without
     *     delimiters.  Each record contains:
     *     - A binary hash to scan for, of length hash_size.
     *     - A 2-byte unsigned integer in native-Endian format indicating
     *       the length, in bytes, of the upcoming binary label associated
     *       with the scan record.
     *     - A binary label associated with the scan record, of the
     *       length just indicated.
     */
    void put(const std::string& unscanned_data);

    /**
     * Receive a string containing an array of records of matched scanned
     * data or "" if no data is available.
     *
     * Returns:
     *   An array of records of matched scanned data or "" if no data
     *   is available.  Each record conatins:
     *   - A binary hash that matched, of length hash_size.
     *   - A 2-byte unsigned integer in native-Endian format indicating
     *     the length, in bytes, of the upcoming binary label associated
     *     with the hash that matched.
     *   - A binary label associated with the scan record, of the
     *     length just indicated.
     *   - A 4-byte unsigned integer in native-Endian format indicating
     *     the length, in bytes, of the upcoming JSON text associated
     *     with the hash that matched.
     *   - JSON text formatted based on the scan mode selected, of the
     *     length just indicated.
     */
    std::string get();

    /**
     * Returns true if scan_stream is empty, meaning that there is no
     * unscanned data left to scan and there is no scanned data left to
     * retrieve.  If not empty, a thread yield is issued so that the
     * caller can busy-wait with less waste.
     *
     * Returns:
     *   true if scan_stream is empty.
     */
    bool empty();
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

#ifndef SWIG
    // do not allow copy or assignment
    timestamp_t(const timestamp_t&) = delete;
    timestamp_t& operator=(const timestamp_t&) = delete;
#endif

    /**
     * Create a named timestamp and return a JSON string in format
     * {"name":"name", "delta":delta, "total":total}.
     */
    std::string stamp(const std::string &name);
  };
}

#endif

