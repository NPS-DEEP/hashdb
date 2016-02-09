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
 * Implementation code for the hashdb library.
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
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <climits>
#ifndef HAVE_CXX11
#include <cassert>
#endif
#include <sys/stat.h>   // for mkdir
#include <time.h>       // for timestamp
#include <sys/types.h>  // for timestamp
#include <sys/time.h>   // for timestamp
#include <pwd.h>        // for print_environment
#include <unistd.h>     // for print_environment
#include <iostream>     // for print_environment
#include "file_modes.h"
#include "settings_manager.hpp"
#include "lmdb_hash_data_manager.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_source_data_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include "logger.hpp"
#include "lmdb_changes.hpp"
#include "escape_json.hpp"
#include "crc32.h"      // for scan_expanded
#include "to_hex.hpp"      // for scan_expanded

// helper for producing expanded source for a source ID
static void provide_source_information(const hashdb::scan_manager_t& manager,
                                       const uint64_t source_id,
                                       std::stringstream& ss) {

  // fields to hold source information
  std::string file_binary_hash;
  uint64_t filesize;
  std::string file_type;
  uint64_t low_entropy_count;
  hashdb::source_names_t* source_names(new hashdb::source_names_t);

  // read source data
  manager.find_source_data(source_id, file_binary_hash, filesize,
                           file_type, low_entropy_count);

  // read source names
  manager.find_source_names(source_id, *source_names);

  // provide source data
  ss << "{\"source_id\":" << source_id
     << ",\"file_hash\":\"" << hashdb::to_hex(file_binary_hash) << "\""
     << ",\"filesize\":" << filesize
     << ",\"file_type\":\"" << file_type << "\""
     << ",\"low_entropy_count\":" << low_entropy_count
     ;

  // provide source names
  ss << ",\"names\":[";
  int i = 0;
  hashdb::source_names_t::const_iterator it;
  for (it = source_names->begin(); it != source_names->end();
       ++it, ++i) {

    // put comma before all but first name pair
    if (i > 0) {
      ss << ",";
    }

    // provide name pair
    ss << "{\"repository_name\":\"" << hashdb::escape_json(it->first)
       << "\",\"filename\":\"" << hashdb::escape_json(it->second)
       << "\"}";
  }

  // close source names list and source data
  ss << "]}";

  delete source_names;
}

// helper: read settings, report and fail on error
static hashdb::settings_t private_read_settings(const std::string& hashdb_dir) {
  hashdb::settings_t settings;
  std::pair<bool, std::string> pair = hashdb::read_settings(
                                                    hashdb_dir, settings);
  if (pair.first == false) {
    std::cerr << "Error: hashdb settings file not read.  Aborting.\n";
    exit(1);
  }
  return settings;
}

namespace hashdb {

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* hashdb_version() {
    return PACKAGE_VERSION;
  }

  // ************************************************************
  // misc support interfaces
  // ************************************************************
  /**
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const hashdb::settings_t& settings,
                     const std::string& command_string) {

    // path must be empty
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      return std::pair<bool, std::string>(false, "Path '"
                       + hashdb_dir + "' already exists.");
    }

    // create the new hashdb directory
    int status;
#ifdef WIN32
    status = mkdir(hashdb_dir.c_str());
#else
    status = mkdir(hashdb_dir.c_str(),0777);
#endif
    if (status != 0) {
      return std::pair<bool, std::string>(false,
                       "Unable to create new hashdb database at path '"
                       + hashdb_dir + "'.");
    }

    // create the settings file
    std::pair<bool, std::string> pair =
                hashdb::write_settings(hashdb_dir, settings);
    if (pair.first == false) {
      return pair;
    }

    // create new LMDB stores
    lmdb_hash_data_manager_t(hashdb_dir, RW_NEW,
                       settings.sector_size, settings.max_id_offset_pairs);
    lmdb_hash_manager_t(hashdb_dir, RW_NEW,
                       settings.hash_prefix_bits, settings.hash_suffix_bytes);
    lmdb_source_data_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_id_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_name_manager_t(hashdb_dir, RW_NEW);

    // create the log
    logger_t(hashdb_dir, command_string);

    return std::pair<bool, std::string>(true, "");
  }

  /**
   * Print environment information to the stream.
   */
  void print_environment(const std::string& command_line, std::ostream& os) {
    // version
    os << "# libhashdb version: " << PACKAGE_VERSION;
#ifdef GIT_COMMIT
    os << ", GIT commit: " << GIT_COMMIT;
#endif
    os << "\n";

    // command
    os << "# command: \"" << command_line << "\"\n";

    // username
#ifdef HAVE_GETPWUID
    os << "# username: " << getpwuid(getuid())->pw_name << "\n";
#endif

    // date
#define TM_FORMAT "%Y-%m-%dT%H:%M:%SZ"
    char buf[256];
    time_t t = time(0);
    strftime(buf,sizeof(buf),TM_FORMAT,gmtime(&t));
    os << "# start time " << buf << "\n";
  }

  // ************************************************************
  // settings
  // ************************************************************
  settings_t::settings_t() :
         settings_version(CURRENT_SETTINGS_VERSION),
         sector_size(512),
         block_size(512),
         max_id_offset_pairs(100000), // 100,000
         hash_prefix_bits(28),        // for 2^28
         hash_suffix_bytes(3) {       // for 2^(3*8)
  }

  std::string settings_t::settings_string() const {
    std::stringstream ss;
    ss << "{\"settings_version\":" << settings_version
       << ", \"sector_size\":" << sector_size
       << ", \"block_size\":" << block_size
       << ", \"max_id_offset_pairs\":" << max_id_offset_pairs
       << ", \"hash_prefix_bits\":" << hash_prefix_bits
       << ", \"hash_suffix_bytes\":" << hash_suffix_bytes
       << "}";
    return ss.str();
  }

  // ************************************************************
  // import
  // ************************************************************
  import_manager_t::import_manager_t(const std::string& hashdb_dir,
                                     const std::string& command_string) :
          // LMDB managers
          lmdb_hash_data_manager(0),
          lmdb_hash_manager(0),
          lmdb_source_data_manager(0),
          lmdb_source_id_manager(0),
          lmdb_source_name_manager(0),

          // log
          logger(new logger_t(hashdb_dir, command_string)),
          changes(new hashdb::lmdb_changes_t) {

    // read settings
    hashdb::settings_t settings = private_read_settings(hashdb_dir);

    // open managers
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir,
            RW_MODIFY, settings.sector_size, settings.max_id_offset_pairs);
    lmdb_hash_manager = new lmdb_hash_manager_t(hashdb_dir, RW_MODIFY,
            settings.hash_prefix_bits, settings.hash_suffix_bytes);
    lmdb_source_data_manager = new lmdb_source_data_manager_t(hashdb_dir,
                                                              RW_MODIFY);
    lmdb_source_id_manager = new lmdb_source_id_manager_t(hashdb_dir,
                                                              RW_MODIFY);
    lmdb_source_name_manager = new lmdb_source_name_manager_t(hashdb_dir,
                                                              RW_MODIFY);

  }

  import_manager_t::~import_manager_t() {

    // show changes
    logger->add_lmdb_changes(*changes);
    std::cout << *changes;

    // close resources
    delete lmdb_hash_data_manager;
    delete lmdb_hash_manager;
    delete lmdb_source_data_manager;
    delete lmdb_source_id_manager;
    delete lmdb_source_name_manager;
    delete logger;
    delete changes;
  }

  std::pair<bool, uint64_t> import_manager_t::insert_source_id(
                                       const std::string& file_binary_hash) {
    return lmdb_source_id_manager->insert(file_binary_hash, *changes);
  }


  void import_manager_t::insert_source_name(const uint64_t source_id,
                          const std::string& repository_name,
                          const std::string& filename) {
    lmdb_source_name_manager->insert(
                             source_id, repository_name, filename, *changes);
  }

  void import_manager_t::insert_source_data(const uint64_t source_id,
                          const std::string& file_binary_hash,
                          const uint64_t filesize,
                          const std::string& file_type,
                          const uint64_t low_entropy_count) {
    lmdb_source_data_manager->insert(source_id, file_binary_hash,
                        filesize, file_type, low_entropy_count, *changes);
  }

  void import_manager_t::insert_hash(const std::string& binary_hash,
                        const uint64_t source_id,
                        const uint64_t file_offset,
                        const uint64_t entropy,
                        const std::string& block_label) {

    // insert into hash manager and hash data manager
    const size_t count = lmdb_hash_data_manager->insert(binary_hash,
                    source_id, file_offset, entropy, block_label, *changes);
    lmdb_hash_manager->insert(binary_hash, count, *changes);
  }

  std::string import_manager_t::sizes() const {
    std::stringstream ss;
    ss << "{\"hash_data_store\":" << lmdb_hash_data_manager->size()
       << ", \"hash_store\":" << lmdb_hash_manager->size()
       << ", \"source_data_store\":" << lmdb_source_data_manager->size()
       << ", \"source_id_store\":" << lmdb_source_id_manager->size()
       << ", \"source_name_store\":" << lmdb_source_name_manager->size()
       << "}";
    return ss.str();
  }

  // ************************************************************
  // scan
  // ************************************************************
  scan_manager_t::scan_manager_t(const std::string& hashdb_dir) :
          // LMDB managers
          lmdb_hash_data_manager(0),
          lmdb_hash_manager(0),
          lmdb_source_data_manager(0),
          lmdb_source_id_manager(0),
          lmdb_source_name_manager(0),

          // for scan_expanded
          hashes(new std::set<std::string>),
          source_ids(new std::set<uint64_t>) {

    // read settings
    hashdb::settings_t settings = private_read_settings(hashdb_dir);

    // open managers
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir,
            READ_ONLY, settings.sector_size, settings.max_id_offset_pairs);
    lmdb_hash_manager = new lmdb_hash_manager_t(hashdb_dir, READ_ONLY,
            settings.hash_prefix_bits, settings.hash_suffix_bytes);
    lmdb_source_data_manager = new lmdb_source_data_manager_t(hashdb_dir,
                                                              READ_ONLY);
    lmdb_source_id_manager = new lmdb_source_id_manager_t(hashdb_dir,
                                                              READ_ONLY);
    lmdb_source_name_manager = new lmdb_source_name_manager_t(hashdb_dir,
                                                              READ_ONLY);
  }

  scan_manager_t::~scan_manager_t() {
    delete lmdb_hash_data_manager;
    delete lmdb_hash_manager;
    delete lmdb_source_data_manager;
    delete lmdb_source_id_manager;
    delete lmdb_source_name_manager;

    // for scan_expanded
    delete hashes;
    delete source_ids;
  }

  // Example abbreviated syntax:
  // [{"source_list_id":57}, {"sources":[{"source_id":1, "filesize":800,
  // "file_hash":"f7035a...", "names":[{"repository_name":"repository1",
  // "filename":"filename1"}]}]}, {"id_offset_pairs":[1,0,1,65536]}]
  bool scan_manager_t::find_expanded_hash(const std::string& binary_hash,
                                          std::string& expanded_text) {

    // fields to hold the scan
    uint64_t entropy;
    std::string block_label;
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;

    // scan
    bool matched = scan_manager_t::find_hash(binary_hash,
                                  entropy, block_label, *id_offset_pairs);

    // done if no match
    if (matched == false) {
      expanded_text.clear();
      delete id_offset_pairs;
      return false;
    }

    // done if matched before
    if (hashes->find(binary_hash) != hashes->end()) {
      expanded_text.clear();
      delete id_offset_pairs;
      return true;
    }

    // remember this hash match to skip it later
    hashes->insert(binary_hash);

    // prepare JSON
    std::stringstream *ss = new std::stringstream;
    *ss << "[";

    // provide JSON object[0]: source_list_id
    hashdb::id_offset_pairs_t::const_iterator it;
    uint32_t crc = 0;
    for (it = id_offset_pairs->begin(); it != id_offset_pairs->end(); ++it) {
      crc = hashdb::crc32(crc, reinterpret_cast<const uint8_t*>(&(it->first)),
                  sizeof(uint64_t));
    }
    *ss << "{\"source_list_id\":" << crc << "}";

    // provide JSON object[1]: sources with their source data and names list
    *ss << ",{\"sources\":[";

    // print any sources that have not been printed yet
    for (it = id_offset_pairs->begin(); it != id_offset_pairs->end(); ++it) {
      if (source_ids->find(it->first) == source_ids->end()) {
        // remember this source ID to skip it later
        source_ids->insert(it->first);

        // provide the complete source information for this source ID
        provide_source_information(*this, it->first, *ss);
      }
    }

    // close JSON object[1]
    *ss << "]}";

    // provide JSON object[2]: id_offset_pairs
    *ss << ",{\"id_offset_pairs\":[";
    int i=0;
    for (it = id_offset_pairs->begin(); it != id_offset_pairs->end();
          ++it, ++i) {

      // put comma before all but first id_offset pair
      if (i > 0) {
        *ss << ",";
      }

      // provide pair
      *ss << it->first << ","
          << it->second;
    }

    // close JSON object[2]
    *ss << "]}";
 
    // close JSON
    *ss << "]";

    // copy stream
    expanded_text = ss->str();

    delete id_offset_pairs;
    delete ss;
    return true;
  }

  bool scan_manager_t::find_hash(const std::string& binary_hash,
                      uint64_t& entropy,
                      std::string& block_label,
                      id_offset_pairs_t& pairs) const {
    if (lmdb_hash_manager->find(binary_hash) > 0) {
      // hash may be present so use hash data manager
      return lmdb_hash_data_manager->find(binary_hash,
                                          entropy, block_label, pairs);
    } else {
      // hash is not present so return false
      entropy = 0;
      block_label = "";
      pairs.clear();
      return false;
    }
  }

  size_t scan_manager_t::find_approximate_hash_count(
                                    const std::string& binary_hash) const {
    return lmdb_hash_manager->find(binary_hash);
  }

  bool scan_manager_t::find_source_data(const uint64_t source_id,
                        std::string& file_binary_hash,
                        uint64_t& filesize,
                        std::string& file_type,
                        uint64_t& low_entropy_count) const {
    return lmdb_source_data_manager->find(source_id,
                 file_binary_hash, filesize, file_type, low_entropy_count);
  }

  bool scan_manager_t::find_source_names(const uint64_t source_id,
                         source_names_t& source_names) const {
    return lmdb_source_name_manager->find(source_id, source_names);
  }

  std::pair<bool, uint64_t> scan_manager_t::find_source_id(
                              const std::string& file_binary_hash) const {
    return lmdb_source_id_manager->find(file_binary_hash);
  }

  std::pair<bool, std::string> scan_manager_t::hash_begin() const {
    return lmdb_hash_data_manager->find_begin();
  }

  std::pair<bool, std::string> scan_manager_t::hash_next(
                        const std::string& last_binary_hash) const {
    return lmdb_hash_data_manager->find_next(last_binary_hash);
  }

  std::pair<bool, uint64_t> scan_manager_t::source_begin() const {
    return lmdb_source_data_manager->find_begin();
  }

  std::pair<bool, uint64_t> scan_manager_t::source_next(
                                   const uint64_t last_source_id) const {
    return lmdb_source_data_manager->find_next(last_source_id);
  }

  std::string scan_manager_t::sizes() const {
    std::stringstream ss;
    ss << "{\"hash_data_store\":" << lmdb_hash_data_manager->size()
       << ", \"hash_store\":" << lmdb_hash_manager->size()
       << ", \"source_data_store\":" << lmdb_source_data_manager->size()
       << ", \"source_id_store\":" << lmdb_source_id_manager->size()
       << ", \"source_name_store\":" << lmdb_source_name_manager->size()
       << "}";
    return ss.str();
  }

  size_t scan_manager_t::size() const {
    return lmdb_hash_data_manager->size();
  }

  // ************************************************************
  // timestamp
  // ************************************************************
  timestamp_t::timestamp_t() :
              t0(new timeval()), t_last_timestamp(new timeval()) {
    gettimeofday(t0, 0);
    gettimeofday(t_last_timestamp, 0);
  }

  timestamp_t::~timestamp_t() {
    delete t0;
    delete t_last_timestamp;
  }

  /**
   * Take a timestamp and return a JSON string in format {"name":"name",
   * "delta":delta, "total":total}.
   */
  std::string timestamp_t::stamp(const std::string &name) {
    // adapted from dfxml_writer.cpp
    struct timeval t1;
    gettimeofday(&t1,0);
    struct timeval t;

    // timestamp delta against t_last_timestamp
    t.tv_sec = t1.tv_sec - t_last_timestamp->tv_sec;
    if(t1.tv_usec > t_last_timestamp->tv_usec){
        t.tv_usec = t1.tv_usec - t_last_timestamp->tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t_last_timestamp->tv_usec;
    }
    char delta[16];
    snprintf(delta, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // reset t_last_timestamp for the next invocation
    gettimeofday(t_last_timestamp,0);

    // timestamp total
    t.tv_sec = t1.tv_sec - t0->tv_sec;
    if(t1.tv_usec > t0->tv_usec){
        t.tv_usec = t1.tv_usec - t0->tv_usec;
    } else {
        t.tv_sec--;
        t.tv_usec = (t1.tv_usec+1000000) - t0->tv_usec;
    }
    char total_time[16];
    snprintf(total_time, 16, "%d.%06d", (int)t.tv_sec, (int)t.tv_usec);

    // return the named timestamp
    std::stringstream ss;
    ss << "{\"name\":\"" << hashdb::escape_json(name) << "\""
       << ", \"delta\":" << delta
       << ", \"total\":" << total_time << "}"
       ;
    return ss.str();
  }
}

