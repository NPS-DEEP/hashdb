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
 * Implementation code for the hashdb library.
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
  #include "fsync.h"      // for simulation of linux fsync
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
#include <fcntl.h>      // scan_stream_f
#include <time.h>       // for timestamp
#include <sys/types.h>  // for timestamp
#include <sys/time.h>   // for timestamp
#include <unistd.h>     // for pipe
#include "file_modes.h"
#include "settings_manager.hpp"
#include "lmdb_hash_data_manager.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_source_data_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include "logger.hpp"
#include "locked_member.hpp"
#include "lmdb_changes.hpp"
#include "rapidjson.h"
#include "writer.h"
#include "document.h"
#include "crc32.h"      // for find_expanded_hash_json

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
  // obtain rapidjson::Value type from a std::string
  static rapidjson::Value v(const std::string& s,
                            rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value value;
    value.SetString(s.c_str(), s.size(), allocator);
    return value;
  }

  // helper for producing expanded source for a source ID
  static void provide_source_information(
                        const hashdb::scan_manager_t& manager,
                        const std::string file_hash,
                        rapidjson::Document::AllocatorType& allocator,
                        rapidjson::Value& json_source) {

    // fields to hold source information
    uint64_t filesize;
    std::string file_type;
    uint64_t zero_count;
    uint64_t nonprobative_count;
    hashdb::source_names_t* source_names(new hashdb::source_names_t);

    // read source data
    manager.find_source_data(file_hash, filesize, file_type,
                             zero_count, nonprobative_count);

    // provide source data
    const std::string hex_file_hash = hashdb::bin_to_hex(file_hash);

    // value for strings
    json_source.AddMember("file_hash", v(hex_file_hash, allocator), allocator);
    json_source.AddMember("filesize", filesize, allocator);
    json_source.AddMember("file_type", v(file_type, allocator), allocator);
    json_source.AddMember("zero_count", zero_count, allocator);
    json_source.AddMember("nonprobative_count", nonprobative_count, allocator);

    // read source names
    manager.find_source_names(file_hash, *source_names);

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

  static uint32_t calculate_crc(
                       const hashdb::source_sub_counts_t& source_sub_counts) {

    // calculate the CRC for the sources
    uint32_t crc = 0;
    for (hashdb::source_sub_counts_t::const_iterator it =
         source_sub_counts.begin(); it != source_sub_counts.end(); ++it) {
      crc = hashdb::crc32(crc, static_cast<uint8_t*>(static_cast<void*>(
                          const_cast<char*>((*it).file_hash.c_str()))),
                          it->file_hash.size());
    }
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
   * Return "" if hashdb is created else reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   */
  std::string create_hashdb(const std::string& hashdb_dir,
                            const hashdb::settings_t& settings,
                            const std::string& command_string) {

    // path must be empty
    if (access(hashdb_dir.c_str(), F_OK) == 0) {
      return "Path '" + hashdb_dir + "' already exists.";
    }

    // create the new hashdb directory
    int status;
#ifdef WIN32
    status = mkdir(hashdb_dir.c_str());
#else
    status = mkdir(hashdb_dir.c_str(),0777);
#endif
    if (status != 0) {
      return "Unable to create new hashdb database at path '"
                     + hashdb_dir + "'.";
    }

    // create the settings file
    std::string error_message = hashdb::write_settings(hashdb_dir, settings);
    if (error_message.size() != 0) {
      return error_message;
    }

    // create new LMDB stores
    lmdb_hash_data_manager_t(hashdb_dir, RW_NEW);
    lmdb_hash_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_data_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_id_manager_t(hashdb_dir, RW_NEW);
    lmdb_source_name_manager_t(hashdb_dir, RW_NEW);

    // create the log
    logger_t(hashdb_dir, command_string);

    return "";
  }

  // ************************************************************
  // source sub_counts
  // ************************************************************
  source_sub_count_t::source_sub_count_t(const std::string& p_file_hash,
                    const uint64_t p_sub_count) :
          file_hash(p_file_hash),
          sub_count(p_sub_count) {
    }
  bool source_sub_count_t::operator<(const source_sub_count_t& that) const {
    return (file_hash < that.file_hash);
  }

  // ************************************************************
  // settings
  // ************************************************************
  settings_t::settings_t() :
         settings_version(settings_t::CURRENT_SETTINGS_VERSION),
         block_size(512) {
  }

  std::string settings_t::settings_string() const {
    std::stringstream ss;
    ss << "{\"settings_version\":" << settings_version
       << ", \"block_size\":" << block_size
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

    // open managers
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir,
                                                          RW_MODIFY);
    lmdb_hash_manager = new lmdb_hash_manager_t(hashdb_dir, RW_MODIFY);
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
                          const std::string& file_hash,
                          const std::string& repository_name,
                          const std::string& filename) {
    if (file_hash.size() == 0) {
      std::cerr << "Error: insert_source_name called with empty file_hash\n";
      return;
    }
    uint64_t source_id;
    bool is_new_id = lmdb_source_id_manager->insert(file_hash, *changes,
                                                    source_id);
    lmdb_source_name_manager->insert(source_id, repository_name, filename,
                                     *changes);

    // If the source ID is new then add a blank source data record just to keep
    // from breaking the reverse look-up done in scan_manager_t.
    if (is_new_id == true) {
      lmdb_source_data_manager->insert(source_id, file_hash, 0, "", 0, 0,
                                       *changes);
    }
  }

  void import_manager_t::insert_source_data(
                          const std::string& file_hash,
                          const uint64_t filesize,
                          const std::string& file_type,
                          const uint64_t zero_count,
                          const uint64_t nonprobative_count) {
    if (file_hash.size() == 0) {
      std::cerr << "Error: insert_source_data called with empty file_hash\n";
      return;
    }
    uint64_t source_id;
    lmdb_source_id_manager->insert(file_hash, *changes, source_id);
    lmdb_source_data_manager->insert(source_id, file_hash,
               filesize, file_type, zero_count, nonprobative_count, *changes);
  }

  // add whether file hash is present or not, used during ingest
  void import_manager_t::insert_hash(const std::string& block_hash,
                          const uint64_t k_entropy,
                          const std::string& block_label,
                          const std::string& file_hash) {

    if (block_hash.size() == 0) {
      std::cerr << "Error: insert_hash called with empty block_hash\n";
      return;
    }
    if (file_hash.size() == 0) {
      std::cerr << "Error: insert_hash called with empty file_hash\n";
      return;
    }

    uint64_t source_id;
    bool is_new_id = lmdb_source_id_manager->insert(file_hash, *changes,
                                                    source_id);

    // insert hash into hash data manager and hash manager
    const size_t count = lmdb_hash_data_manager->insert(
                 block_hash, k_entropy, block_label,
                 source_id, *changes);
    lmdb_hash_manager->insert(block_hash, count, *changes);

    // If the source ID is new then add a blank source data record just to keep
    // from breaking the reverse look-up done in scan_manager_t.
    if (is_new_id == true) {
      lmdb_source_data_manager->insert(source_id, file_hash, 0, "", 0, 0,
                                       *changes);
    }
  }

  // add only if file hash is not present, use during merge
  void import_manager_t::merge_hash(const std::string& block_hash,
                                    const uint64_t k_entropy,
                                    const std::string& block_label,
                                    const std::string& file_hash,
                                    const uint64_t sub_count) {

    if (block_hash.size() == 0) {
      std::cerr << "Error: insert_hash called with empty block_hash\n";
      return;
    }
    if (file_hash.size() == 0) {
      std::cerr << "Error: insert_hash called with empty file_hash\n";
      return;
    }

    uint64_t source_id;
    bool is_new_id = lmdb_source_id_manager->insert(file_hash, *changes,
                                                    source_id);

    // merge hash into hash data manager
    const size_t count = lmdb_hash_data_manager->merge(
                 block_hash, k_entropy, block_label,
                 source_id, sub_count, *changes);

    // insert hash into hash manager
    lmdb_hash_manager->insert(block_hash, count, *changes);

    // If the source ID is new then add a blank source data record just to keep
    // from breaking the reverse look-up done in scan_manager_t.
    if (is_new_id == true) {
      lmdb_source_data_manager->insert(source_id, file_hash, 0, "", 0, 0,
                                       *changes);
    }
  }

  // import JSON hash or source, return "" or error
  std::string import_manager_t::import_json(
                          const std::string& json_string) {

    // open input as a JSON DOM document
    rapidjson::Document document;
    if (document.Parse(json_string.c_str()).HasParseError() ||
        !document.IsObject()) {
      return "Invalid JSON syntax";
    }

    // block_hash or file_hash
    if (document.HasMember("block_hash")) {

      // block_hash
      if (!document["block_hash"].IsString()) {
        return "Invalid block_hash field";
      }
      const std::string block_hash = hashdb::hex_to_bin(
                                         document["block_hash"].GetString());

      // entropy (optional)
      uint64_t k_entropy = 0;
      if (document.HasMember("k_entropy")) {
        if (document["k_entropy"].IsUint64()) {
          k_entropy = document["k_entropy"].GetUint64();
        } else {
          return "Invalid k_entropy field";
        }
      }

      // block_label (optional)
      std::string block_label = "";
      if (document.HasMember("block_label")) {
        if (document["block_label"].IsString()) {
          block_label = document["block_label"].GetString();
        } else {
          return "Invalid block_label field";
        }
      }

      // source_sub_counts:[]
      if (!document.HasMember("source_sub_counts") ||
                    !document["source_sub_counts"].IsArray()) {
        return "Invalid source_sub_counts field";
      }
      const rapidjson::Value& json_source_sub_counts =
                                        document["source_sub_counts"];
      hashdb::source_sub_counts_t* source_sub_counts =
                                        new hashdb::source_sub_counts_t;
      for (rapidjson::SizeType i = 0;
           i+1 < json_source_sub_counts.Size(); i+=2) {

        // source hash
        if (!json_source_sub_counts[i+0].IsString()) {
          delete source_sub_counts;
          return "Invalid source hash in source_sub_counts";
        }
        const std::string file_hash = hashdb::hex_to_bin(
                                        json_source_sub_counts[i].GetString());

        // sub_count
        if (!json_source_sub_counts[i+1].IsUint64()) {
          delete source_sub_counts;
          return "Invalid sub_count in source_sub_counts";
        }
        const uint64_t sub_count = json_source_sub_counts[i+1].GetUint64();

        // add hash data for this source and source sub_count
        merge_hash(block_hash, k_entropy, block_label, file_hash, sub_count);
      }

      delete source_sub_counts;
      return "";

    } else if (document.HasMember("file_hash")) {

      // parse file_hash
      if (!document.HasMember("file_hash") ||
                    !document["file_hash"].IsString()) {
        return "Invalid file_hash field";
      }
      const std::string file_hash = hashdb::hex_to_bin(
                                       document["file_hash"].GetString());

      // parse filesize
      if (!document.HasMember("filesize") ||
                    !document["filesize"].IsUint64()) {
        return "Invalid filesize field";
      }
      const uint64_t filesize = document["filesize"].GetUint64();

      // parse file_type (optional)
      std::string file_type = "";
      if (document.HasMember("file_type")) {
        if (document["file_type"].IsString()) {
          file_type = document["file_type"].GetString();
        } else {
          return "Invalid file_type field";
        }
      }

      // zero_count (optional)
      uint64_t zero_count = 0;
      if (document.HasMember("zero_count")) {
        if (document["zero_count"].IsUint64()) {
          zero_count = document["zero_count"].GetUint64();
        } else {
          return "Invalid zero_count field";
        }
      }

      // nonprobative_count (optional)
      uint64_t nonprobative_count = 0;
      if (document.HasMember("nonprobative_count")) {
        if (document["nonprobative_count"].IsUint64()) {
          nonprobative_count = document["nonprobative_count"].GetUint64();
        } else {
          return "Invalid nonprobative_count field";
        }
      }

      // parse name_pairs:[]
      if (!document.HasMember("name_pairs") ||
                    !document["name_pairs"].IsArray()) {
        return "Invalid name_pairs field";
      }
      const rapidjson::Value& json_names = document["name_pairs"];
      hashdb::source_names_t* names = new hashdb::source_names_t;
      for (rapidjson::SizeType i = 0; i< json_names.Size(); i+=2) {

        // parse repository name
        if (!json_names[i].IsString()) {
          delete names;
          return "Invalid repository name in name_pairs field";
        }
        const std::string repository_name = json_names[i].GetString();

        // parse filename
        if (!json_names[i+1].IsString()) {
          delete names;
          return "Invalid filename in name_pairs field";
        }
        const std::string filename = json_names[i+1].GetString();

        // add repository name, filename pair
        names->insert(hashdb::source_name_t(repository_name, filename));
      }

      // everything worked so insert the source data and source names
      insert_source_data(file_hash,
                         filesize, file_type, zero_count, nonprobative_count);
      for (hashdb::source_names_t::const_iterator it = names->begin();
           it != names->end(); ++it) {
        insert_source_name(file_hash, it->first, it->second);
      }

      delete names;
      return "";

    } else {
      return "A block_hash or file_hash field is required";
    }
  }

  bool import_manager_t::has_source(const std::string& file_hash) const {
    uint64_t source_id;
    return lmdb_source_id_manager->find(file_hash, source_id);
  }

  std::string import_manager_t::first_source() const {
    return lmdb_source_id_manager->first_source();
  }

  std::string import_manager_t::next_source(const std::string& file_hash) const {
    return lmdb_source_id_manager->next_source(file_hash);
  }

  std::string import_manager_t::size() const {
    std::stringstream ss;
    ss << "{\"hash_data_store\":" << lmdb_hash_data_manager->size()
       << ", \"hash_store\":" << lmdb_hash_manager->size()
       << ", \"source_data_store\":" << lmdb_source_data_manager->size()
       << ", \"source_id_store\":" << lmdb_source_id_manager->size()
       << ", \"source_name_store\":" << lmdb_source_name_manager->size()
       << "}";
    return ss.str();
  }

  size_t import_manager_t::size_hashes() const {
    return lmdb_hash_data_manager->size();
  }

  size_t import_manager_t::size_sources() const {
    return lmdb_source_id_manager->size();
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

          // for find_expanded_hash_json
          hashes(new locked_member_t),
          sources(new locked_member_t) {

    // open managers
    lmdb_hash_data_manager = new lmdb_hash_data_manager_t(hashdb_dir,
                                                          READ_ONLY);
    lmdb_hash_manager = new lmdb_hash_manager_t(hashdb_dir, READ_ONLY);
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

    // for find_expanded_hash_json
    delete hashes;
    delete sources;
  }

  std::string scan_manager_t::find_hash_json(
                   const hashdb::scan_mode_t scan_mode,
                   const std::string& block_hash) {

    // delegate to low-level handler
    switch(scan_mode) {

      // EXPANDED
      case hashdb::scan_mode_t::EXPANDED:
        return find_expanded_hash_json(false, block_hash);

      // EXPANDED_OPTIMIZED
      case hashdb::scan_mode_t::EXPANDED_OPTIMIZED:
        return find_expanded_hash_json(true, block_hash);

      // COUNT
      case hashdb::scan_mode_t::COUNT:
        return find_hash_count_json(block_hash);

      // APPROXIMATE_COUNT
      case hashdb::scan_mode_t::APPROXIMATE_COUNT:
        return find_approximate_hash_count_json(block_hash);

      default: assert(0); std::exit(1);
    }
  }

  // Find expanded hash, optimized with caching, return JSON.
  // If optimizing, cache hashes and sources.
  std::string scan_manager_t::find_expanded_hash_json(
                    const bool optimizing, const std::string& block_hash) {

    // fields to hold the scan
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                           new hashdb::source_sub_counts_t;

    // scan
    bool matched = scan_manager_t::find_hash(block_hash,
                           k_entropy, block_label, count, *source_sub_counts);

    // done if no match
    if (matched == false) {
      delete source_sub_counts;
      return "";
    }

    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();

    // block_hash
    std::string hex_block_hash = hashdb::bin_to_hex(block_hash);
    json_doc.AddMember("block_hash", v(hex_block_hash, allocator), allocator);

    // report hash if not caching or this is the first time for the hash
    if (!optimizing || hashes->locked_insert(block_hash)) {

      // add entropy
      json_doc.AddMember("k_entropy", k_entropy, allocator);

      // add block_label
      json_doc.AddMember("block_label", v(block_label, allocator), allocator);

      // add count
      json_doc.AddMember("count", count, allocator);

      // add source_list_id
      uint32_t crc = calculate_crc(*source_sub_counts);
      json_doc.AddMember("source_list_id", crc, allocator);

      // the sources array
      rapidjson::Value json_sources(rapidjson::kArrayType);

      // add each source object
      for (hashdb::source_sub_counts_t::const_iterator it =
           source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {
        if (!optimizing || sources->locked_insert(it->file_hash)) {

          // create a json_source object for the json_sources array
          rapidjson::Value json_source(rapidjson::kObjectType);

          // provide the complete source information for this source
          provide_source_information(*this, it->file_hash, allocator,
                                     json_source);
          json_sources.PushBack(json_source, allocator);
        }
      }
      json_doc.AddMember("sources", json_sources, allocator);

      // add source_sub_counts as pairs of file hash, sub_count
      rapidjson::Value json_source_sub_counts(rapidjson::kArrayType);

      for (hashdb::source_sub_counts_t::const_iterator it =
           source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

        // file hash
        json_source_sub_counts.PushBack(
                   v(hashdb::bin_to_hex(it->file_hash), allocator), allocator);

        // sub_count
        json_source_sub_counts.PushBack(it->sub_count, allocator);

      }
      json_doc.AddMember("source_sub_counts", json_source_sub_counts,
                         allocator);
    }

    delete source_sub_counts;

    // return JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    return strbuf.GetString();
  }

  // find hash, return associated hash and source data
  bool scan_manager_t::find_hash(
               const std::string& block_hash,
               uint64_t& k_entropy,
               std::string& block_label,
               uint64_t& count,
               source_sub_counts_t& source_sub_counts) const {

    // clear fields
    k_entropy = 0;
    block_label = "";
    count = 0;
    source_sub_counts.clear();

    if (block_hash.size() == 0) {
      std::cerr << "Error: find_hash called with empty block_hash\n";
      return false;
    }

    // first check hash store
    if (lmdb_hash_manager->find(block_hash) == 0) {
      // hash is not present so return false
      return false;
    }

    // hash may be present so read hash using hash data manager
    hashdb::source_id_sub_counts_t* source_id_sub_counts =
                new hashdb::source_id_sub_counts_t;
    bool has_hash = lmdb_hash_data_manager->find(block_hash, k_entropy,
                                  block_label, count, *source_id_sub_counts);
    if (has_hash) {
      // build source_sub_count from source_id_sub_count
      for (hashdb::source_id_sub_counts_t::const_iterator it =
           source_id_sub_counts->begin(); it != source_id_sub_counts->end();
           ++it) {

        // space for unused returned source variables
        std::string file_hash;
        uint64_t filesize;
        std::string file_type;
        uint64_t zero_count;
        uint64_t nonprobative_count;

        // get file_hash from source_id
        bool source_data_found = lmdb_source_data_manager->find(
                                it->source_id, file_hash,
                                filesize, file_type,
                                zero_count, nonprobative_count);

        // source_data must have a source_id to match the source_id in hash_data
        if (source_data_found == false) {
          assert(0);
        }

        // add the source sub_counts
        source_sub_counts.insert(hashdb::source_sub_count_t(file_hash,
                                                            it->sub_count));
      }
      delete source_id_sub_counts;
      return true;

    } else {
      // no action, lmdb_hash_data_manager.find clears out fields
      delete source_id_sub_counts;
      return false;
    }
  }

  // export hash, return result as JSON string
  std::string scan_manager_t::export_hash_json(
               const std::string& block_hash) const {

    // hash fields
    uint64_t k_entropy;
    std::string block_label;
    uint64_t unused_count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                new hashdb::source_sub_counts_t;

    // scan
    bool found_hash = find_hash(block_hash, k_entropy, block_label,
                                unused_count, *source_sub_counts);

    std::string json_hash_string;
    if (found_hash) {

      // prepare JSON
      rapidjson::Document json_doc;
      rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
      json_doc.SetObject();

      // put in hash data
      std::string hex_block_hash = hashdb::bin_to_hex(block_hash);
      json_doc.AddMember("block_hash", v(hex_block_hash, allocator), allocator);
      json_doc.AddMember("k_entropy", k_entropy, allocator);
      json_doc.AddMember("block_label", v(block_label, allocator), allocator);

      // put in source_sub_counts as pairs of file hash, sub_count
      rapidjson::Value json_source_sub_counts(rapidjson::kArrayType);

      for (hashdb::source_sub_counts_t::const_iterator it =
           source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

        // file hash
        json_source_sub_counts.PushBack(
                   v(hashdb::bin_to_hex(it->file_hash), allocator), allocator);

        // sub_count
        json_source_sub_counts.PushBack(it->sub_count, allocator);

      }
      json_doc.AddMember("source_sub_counts", json_source_sub_counts,
                         allocator);

      // write JSON text
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      json_doc.Accept(writer);
      json_hash_string = strbuf.GetString();

    } else {
      // not found
      json_hash_string = "";
    }

    delete source_sub_counts;
    return json_hash_string;
  }

  // find hash count
  size_t scan_manager_t::find_hash_count(
                                    const std::string& block_hash) const {

    if (block_hash.size() == 0) {
      std::cerr << "Error: find_hash_count called with empty block_hash\n";
      return 0;
    }

    return lmdb_hash_data_manager->find_count(block_hash);
  }

  // find hash count JSON
  std::string scan_manager_t::find_hash_count_json(
                                    const std::string& block_hash) const {

    // get count
    size_t count = find_hash_count(block_hash);

    // no match
    if (count == 0) {
      return "";
    }

    // return JSON with count
    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();

    // block hash
    std::string hex_block_hash = hashdb::bin_to_hex(block_hash);
    json_doc.AddMember("block_hash", v(hex_block_hash, allocator), allocator);

    // count
    json_doc.AddMember("count", (uint64_t)count, allocator);

    // write JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    return strbuf.GetString();
  }

  size_t scan_manager_t::find_approximate_hash_count(
                                    const std::string& block_hash) const {
    if (block_hash.size() == 0) {
      std::cerr << "Error: find_approximate_hash_count called with empty block_hash\n";
      return 0;
    }

    return lmdb_hash_manager->find(block_hash);
  }

  // find approximate hash count JSON
  std::string scan_manager_t::find_approximate_hash_count_json(
                                    const std::string& block_hash) const {

    // get approximate count
    size_t approximate_count =
           find_approximate_hash_count(block_hash);

    // no match
    if (approximate_count == 0) {
      return "";
    }

    // return JSON with approximate count
    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();

    // block hash
    std::string hex_block_hash = hashdb::bin_to_hex(block_hash);
    json_doc.AddMember("block_hash", v(hex_block_hash, allocator), allocator);

    // approximate count
    json_doc.AddMember("approximate_count",
                                  (uint64_t)approximate_count, allocator);

    // write JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    return strbuf.GetString();
  }

  bool scan_manager_t::find_source_data(
                        const std::string& file_hash,
                        uint64_t& filesize,
                        std::string& file_type,
                        uint64_t& zero_count,
                        uint64_t& nonprobative_count) const {

    if (file_hash.size() == 0) {
      std::cerr << "Error: find_source_data called with empty file_hash\n";
      return false;
    }

    // read source_id
    uint64_t source_id;
    bool has_id = lmdb_source_id_manager->find(file_hash, source_id);
    if (has_id == false) {
      // no source ID for this file_hash
      filesize = 0;
      file_type = "";
      zero_count = 0;
      nonprobative_count = 0;
      return false;
    } else {

      // read source data associated with this source ID
      std::string returned_file_hash;
      bool source_data_found = lmdb_source_data_manager->find(source_id,
                             returned_file_hash, filesize, file_type,
                             zero_count, nonprobative_count);

      // if source data is found, make sure the file binary hash is right
      if (source_data_found == true &&
                         file_hash != returned_file_hash) {
        assert(0);
      }
    }
    return true;
  }

  bool scan_manager_t::find_source_names(const std::string& file_hash,
                         source_names_t& source_names) const {

    if (file_hash.size() == 0) {
      std::cerr << "Error: find_source_names called with empty file_hash\n";
      return false;
    }

    // read source_id
    uint64_t source_id;
    bool has_id = lmdb_source_id_manager->find(file_hash, source_id);
    if (has_id == false) {
      // no source ID for this file_hash
      source_names.clear();
      return false;
    } else {
      // source
      return lmdb_source_name_manager->find(source_id, source_names);
    }
  }

  // export source, return result as JSON string
  std::string scan_manager_t::export_source_json(
                               const std::string& file_hash) const {

    // source fields
    uint64_t filesize;
    std::string file_type;
    uint64_t zero_count;
    uint64_t nonprobative_count;

    // prepare JSON
    rapidjson::Document json_doc;
    rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
    json_doc.SetObject();

    // get source data
    bool has_source_data = find_source_data(file_hash, filesize,
                                 file_type, zero_count, nonprobative_count);
    if (!has_source_data) {
      return "";
    }

    // source found

    // set source data
    std::string hex_file_hash = hashdb::bin_to_hex(file_hash);
    json_doc.AddMember("file_hash", v(hex_file_hash, allocator), allocator);
    json_doc.AddMember("filesize", filesize, allocator);
    json_doc.AddMember("file_type", v(file_type, allocator), allocator);
    json_doc.AddMember("zero_count", zero_count, allocator);
    json_doc.AddMember("nonprobative_count", nonprobative_count, allocator);

    // get source names
    hashdb::source_names_t* source_names = new hashdb::source_names_t;
    find_source_names(file_hash, *source_names);

    // name_pairs object
    rapidjson::Value json_name_pairs(rapidjson::kArrayType);

    // provide names
    for (hashdb::source_names_t::const_iterator it = source_names->begin();
         it != source_names->end(); ++it) {
      // repository name
      json_name_pairs.PushBack(v(it->first, allocator), allocator);
      // filename
      json_name_pairs.PushBack(v(it->second, allocator), allocator);
    }
    json_doc.AddMember("name_pairs", json_name_pairs, allocator);

    // done with source names
    delete source_names;

    // write JSON text
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    json_doc.Accept(writer);
    return strbuf.GetString();
  }

  std::string scan_manager_t::first_hash() const {
    return lmdb_hash_data_manager->first_hash();
  }

  std::string scan_manager_t::next_hash(const std::string& block_hash) const {
    if (block_hash.size() == 0) {
      std::cerr << "Error: next_hash called with empty block_hash\n";
      return "";
    }
    return lmdb_hash_data_manager->next_hash(block_hash);
  }

  std::string scan_manager_t::first_source() const {
    return lmdb_source_id_manager->first_source();
  }

  std::string scan_manager_t::next_source(const std::string& file_hash) const {
    if (file_hash.size() == 0) {
      std::cerr << "Error: next_source called with empty file_hash\n";
      return "";
    }
    return lmdb_source_id_manager->next_source(file_hash);
  }

  std::string scan_manager_t::size() const {
    std::stringstream ss;
    ss << "{\"hash_data_store\":" << lmdb_hash_data_manager->size()
       << ", \"hash_store\":" << lmdb_hash_manager->size()
       << ", \"source_data_store\":" << lmdb_source_data_manager->size()
       << ", \"source_id_store\":" << lmdb_source_id_manager->size()
       << ", \"source_name_store\":" << lmdb_source_name_manager->size()
       << "}";
    return ss.str();
  }

  size_t scan_manager_t::size_hashes() const {
    return lmdb_hash_data_manager->size();
  }

  size_t scan_manager_t::size_sources() const {
    return lmdb_source_id_manager->size();
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

