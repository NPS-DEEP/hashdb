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
#ifdef HAVE_PWD_H
#include <pwd.h>        // for print_environment
#endif
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
#include "rapidjson.h"
#include "crc32.h"      // for scan_expanded
#include "to_hex.hpp"      // for scan_expanded

// ************************************************************
// version of the hashdb library
// ************************************************************
/**
 * Version of the hashdb library, same as hashdb::version.
 */
extern "C"
const char* hashdb_version() {
  return PACKAGE_VERSION;
}

namespace hashdb {

  // ************************************************************
  // private helper functions
  // ************************************************************
  // return rapidjson::Value type from a std::string
  static rapidjson::Value v(const std::string& s,
                            rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value value;
    value.SetString(s.c_str(), s.size(), allocator);
    return value;
  }

  // helper for producing expanded source for a source ID
  static void provide_source_information(
                        const hashdb::scan_manager_t& manager,
                        const std::string file_binary_hash,
                        rapidjson::Document::AllocatorType& allocator,
                        rapidjson::Value& json_source) {

    // fields to hold source information
    uint64_t filesize;
    std::string file_type;
    uint64_t nonprobative_count;
    hashdb::source_names_t* source_names(new hashdb::source_names_t);

    // read source data
    manager.find_source_data(file_binary_hash, filesize, file_type,
                             nonprobative_count);

    // provide source data
    const std::string hex_hash = hashdb::to_hex(file_binary_hash);

    // value for strings
    json_source.AddMember("file_hash", v(hex_hash, allocator), allocator);
    json_source.AddMember("filesize", filesize, allocator);
    json_source.AddMember("file_type", v(file_type, allocator), allocator);
    json_source.AddMember("nonprobative_count", nonprobative_count, allocator);

    // read source names
    manager.find_source_names(file_binary_hash, *source_names);

    // name_pairs object
    rapidjson::Value json_name_pairs(rapidjson::kArrayType);

    // provide names
    hashdb::source_names_t::const_iterator it;
    for (it = source_names->begin(); it != source_names->end(); ++it) {
      // repository name
      json_name_pairs.PushBack(v(it->first, allocator), allocator);
      // filename
      json_name_pairs.PushBack(v(it->second, allocator), allocator);
    }
    json_source.AddMember("name_pairs", json_name_pairs, allocator);

    delete source_names;
  }

  // helper: read settings, report and fail on error
  static hashdb::settings_t private_read_settings(
                                       const std::string& hashdb_dir) {
    hashdb::settings_t settings;
    std::pair<bool, std::string> pair = hashdb::read_settings(
                                                    hashdb_dir, settings);
    if (pair.first == false) {
      std::cerr << "Error: hashdb settings file not read.  Aborting.\n";
      exit(1);
    }
    return settings;
  }

  // this computational complexity is nontrivial
  static uint32_t calculate_crc(
                       const hashdb::source_offset_pairs_t& pairs) {
    std::set<std::string>* source_set = new std::set<std::string>;

    // put source hashes in a sorted set without duplicates
    for (hashdb::source_offset_pairs_t::const_iterator it = pairs.begin();
         it != pairs.end(); ++it) {
      source_set->insert(it->first);
    }

    // now calculate the CRC for the sources
    uint32_t crc = 0;
    for (std::set<std::string>::const_iterator it = source_set->begin();
                      it != source_set->end(); ++it) {
      crc = hashdb::crc32(crc, static_cast<uint8_t*>(static_cast<void*>(
                          const_cast<char*>((*it).c_str()))), it->size());
    }
    delete source_set;
    return crc;
  }

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* version() {
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
                  settings.sector_size, settings.max_source_offset_pairs);
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
         settings_version(settings_t::CURRENT_SETTINGS_VERSION),
         sector_size(512),
         block_size(512),
         max_source_offset_pairs(100000), // 100,000
         hash_prefix_bits(28),            // for 2^28
         hash_suffix_bytes(3) {           // for 2^(3*8)
  }

  std::string settings_t::settings_string() const {
    std::stringstream ss;
    ss << "{\"settings_version\":" << settings_version
       << ", \"sector_size\":" << sector_size
       << ", \"block_size\":" << block_size
       << ", \"max_source_offset_pairs\":" << max_source_offset_pairs
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
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir, RW_MODIFY,
            settings.sector_size, settings.max_source_offset_pairs);
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

  void import_manager_t::insert_source_name(
                          const std::string& file_binary_hash,
                          const std::string& repository_name,
                          const std::string& filename) {
    const std::pair<bool, uint64_t> pair = lmdb_source_id_manager->insert(
                                                  file_binary_hash, *changes);
    lmdb_source_name_manager->insert(
                           pair.second, repository_name, filename, *changes);

    // If the source ID is new then add a blank source data record just to keep
    // from breaking the reverse look-up done in scan_manager_t.
    if (pair.first == true) {
      lmdb_source_data_manager->insert(pair.second, file_binary_hash,
                                       0, "", 0, *changes);
    }
  }

  void import_manager_t::insert_source_data(
                          const std::string& file_binary_hash,
                          const uint64_t filesize,
                          const std::string& file_type,
                          const uint64_t nonprobative_count) {
    const std::pair<bool, uint64_t> pair = lmdb_source_id_manager->insert(
                                                  file_binary_hash, *changes);
    lmdb_source_data_manager->insert(pair.second, file_binary_hash,
                        filesize, file_type, nonprobative_count, *changes);
  }

  void import_manager_t::insert_hash(const std::string& binary_hash,
                          const std::string& file_binary_hash,
                          const uint64_t file_offset,
                          const uint64_t entropy,
                          const std::string& block_label) {

    const std::pair<bool, uint64_t> pair = lmdb_source_id_manager->insert(
                                                  file_binary_hash, *changes);

    // insert hash into hash manager and hash data manager
    const size_t count = lmdb_hash_data_manager->insert(binary_hash,
                    pair.second, file_offset, entropy, block_label, *changes);
    lmdb_hash_manager->insert(binary_hash, count, *changes);

    // If the source ID is new then add a blank source data record just to keep
    // from breaking the reverse look-up done in scan_manager_t.
    if (pair.first == true) {
      lmdb_source_data_manager->insert(pair.second, file_binary_hash,
                                       0, "", 0, *changes);
    }
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
          sources(new std::set<std::string>) {

    // read settings
    hashdb::settings_t settings = private_read_settings(hashdb_dir);

    // open managers
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir, READ_ONLY,
            settings.sector_size, settings.max_source_offset_pairs);
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
    delete sources;
  }

  // Example abbreviated syntax:
  // {
  //   "entropy": 8,
  //   "block_label": "W",
  //   "source_list_id": 57,
  //   "sources": [{
  //     "file_hash": "f7035a...",
  //     "filesize": 800,
  //     "file_type": "exe",
  //     "nonprobative_count": 2,
  //     "name_pairs": ["r1", "f1", "r2", "f2]
  //   }],
  //   "source_offset_pairs": ["f7035a...", 0, "f7035a...", 512]
  // }
  bool scan_manager_t::find_expanded_hash(const std::string& binary_hash,
                                          std::string& expanded_text) {

    // clear any existing text
    expanded_text.clear();

    // fields to hold the scan
    uint64_t entropy;
    std::string block_label;
    hashdb::source_offset_pairs_t* source_offset_pairs =
                                  new hashdb::source_offset_pairs_t;

    // scan
    bool matched = scan_manager_t::find_hash(binary_hash,
                           entropy, block_label, *source_offset_pairs);

    // done if no match
    if (matched == false) {
      delete source_offset_pairs;
      return false;
    }

    // done if matched before
    if (hashes->find(binary_hash) != hashes->end()) {
      delete source_offset_pairs;
      return true;
    }

    // remember this hash match to skip it later
    hashes->insert(binary_hash);

    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();

    // entropy
    json_doc.AddMember("entropy", entropy, allocator);

    // block_label
    json_doc.AddMember("block_label", v(block_label, allocator), allocator);

    // source_list_id
    uint32_t crc = calculate_crc(*source_offset_pairs);
    json_doc.AddMember("source_list_id", crc, allocator);

    // sources array
    rapidjson::Value json_sources(rapidjson::kArrayType);

    // put in any sources that have not been put in yet
    for (hashdb::source_offset_pairs_t::const_iterator it =
                      source_offset_pairs->begin();
                      it != source_offset_pairs->end(); ++it) {
      if (sources->find(it->first) == sources->end()) {
        // remember this source to skip it later
        sources->insert(it->first);

        // provide the complete source information for this source
        // a json_source object for the json_sources array
        rapidjson::Value json_source(rapidjson::kObjectType);
        provide_source_information(*this, it->first, allocator, json_source);
        json_sources.PushBack(json_source, allocator);
      }
    }
    json_doc.AddMember("sources", json_sources, allocator);

    // source_offset_pairs
    rapidjson::Value json_pairs(rapidjson::kArrayType);

    for (hashdb::source_offset_pairs_t::const_iterator it =
         source_offset_pairs->begin();
         it != source_offset_pairs->end(); ++it) {

      // file hash
      json_pairs.PushBack(v(hashdb::to_hex(it->first), allocator), allocator);
      // file offset
      json_pairs.PushBack(it->second, allocator);
    }

    json_doc.AddMember("source_offset_pairs", json_pairs, allocator);

    // copy JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    expanded_text = strbuf.GetString();

    delete source_offset_pairs;
    return true;
  }

  bool scan_manager_t::find_hash(
               const std::string& binary_hash,
               uint64_t& entropy,
               std::string& block_label,
               source_offset_pairs_t& source_offset_pairs) const {

    // clear source offset pairs
    source_offset_pairs.clear();

    // first check hash store
    if (lmdb_hash_manager->find(binary_hash) == 0) {
      // hash is not present so return false
      entropy = 0;
      block_label = "";
      return false;
    }

    // hash may be present so read hash using hash data manager
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;
    bool hash_found = lmdb_hash_data_manager->find(binary_hash, entropy,
                                              block_label, *id_offset_pairs);
    if (hash_found) {
      // make space for unused returned source variables
      std::string file_binary_hash;
      uint64_t filesize;
      std::string file_type;
      uint64_t nonprobative_count;

      // build source_offset_pairs from id_offset_pairs
      for (hashdb::id_offset_pairs_t::const_iterator it =
              id_offset_pairs->begin(); it != id_offset_pairs->end(); ++it) {

        // get file_binary_hash from source_id
        bool source_data_found = lmdb_source_data_manager->find(
                                it->first, file_binary_hash,
                                filesize, file_type, nonprobative_count);

        // source_data must have a source_id to match the source_id in hash_data
        if (source_data_found == false) {
          assert(0);
        }

        // add the source
        source_offset_pairs.insert(hashdb::source_offset_pair_t(
                                   file_binary_hash, it->second));
      }
      delete id_offset_pairs;
      return true;

    } else {
      // no action, lmdb_hash_data_manager.find clears out fields
      delete id_offset_pairs;
      return false;
    }
  }

  size_t scan_manager_t::find_hash_count(
                                    const std::string& binary_hash) const {
    return lmdb_hash_data_manager->find_count(binary_hash);
  }

  size_t scan_manager_t::find_approximate_hash_count(
                                    const std::string& binary_hash) const {
    return lmdb_hash_manager->find(binary_hash);
  }

  bool scan_manager_t::find_source_data(
                        const std::string& file_binary_hash,
                        uint64_t& filesize,
                        std::string& file_type,
                        uint64_t& nonprobative_count) const {

    // read source_id into pair.second
    std::pair<bool, uint64_t> pair =
                            lmdb_source_id_manager->find(file_binary_hash);
    if (pair.first == false) {
      // no source ID for this file_binary_hash
      filesize = 0;
      file_type = "";
      nonprobative_count = 0;
      return false;
    } else {

      // read source data associated with this source ID
      std::string returned_file_binary_hash;
      bool source_data_found = lmdb_source_data_manager->find(pair.second,
                             returned_file_binary_hash, filesize, file_type,
                             nonprobative_count);

      // if source data is found, make sure the file binary hash is right
      if (source_data_found == true &&
                         file_binary_hash != returned_file_binary_hash) {
        assert(0);
      }
    }
    return true;
  }

  bool scan_manager_t::find_source_names(const std::string& file_binary_hash,
                         source_names_t& source_names) const {

    // read source_id into pair.second
    std::pair<bool, uint64_t> pair =
                            lmdb_source_id_manager->find(file_binary_hash);
    if (pair.first == false) {
      // no source ID for this file_binary_hash
      source_names.clear();
      return false;
    } else {
      // source
      return lmdb_source_name_manager->find(pair.second, source_names);
    }
  }

  std::pair<bool, std::string> scan_manager_t::hash_begin() const {
    return lmdb_hash_data_manager->find_begin();
  }

  std::pair<bool, std::string> scan_manager_t::hash_next(
                        const std::string& last_binary_hash) const {
    return lmdb_hash_data_manager->find_next(last_binary_hash);
  }

  std::pair<bool, std::string> scan_manager_t::source_begin() const {
    return lmdb_source_id_manager->find_begin();
  }

  std::pair<bool, std::string> scan_manager_t::source_next(
                        const std::string& last_file_binary_hash) const {
    return lmdb_source_id_manager->find_next(last_file_binary_hash);
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
    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();
    json_doc.AddMember("name", v(name, allocator), allocator);
    json_doc.AddMember("delta", v(std::string(delta), allocator), allocator);
    json_doc.AddMember("total", v(std::string(total_time), allocator),
                                                                  allocator);

    // copy JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    return strbuf.GetString();
  }
}

