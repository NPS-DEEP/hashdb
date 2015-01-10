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
 * Static source helpers for preparing json outuput.
 */

#ifndef JSON_FORMATTER_HPP
#define JSON_FORMATTER_HPP
#include "hashdb_manager.hpp"
#include "source_metadata.hpp"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/crc.hpp>

class json_formatter_t {

  private:
  hashdb_manager_t* hashdb_manager;
  uint32_t max_sources;
  std::set<uint64_t>* source_ids;
  std::set<hash_t>* hashes;

  // do not allow copy or assignment
  json_formatter_t(const json_formatter_t&);
  json_formatter_t& operator=(const json_formatter_t&);

  // get source list count
  uint32_t source_list_count(
                 hash_store_key_iterator_range_t it_pair) {

    size_t count = 0;
    // add each source to count
    for (; it_pair.first != it_pair.second; ++it_pair.first) {
      ++count;
    }
    return count;
  }

  // get source list CRC
  uint32_t source_list_id(
                 hash_store_key_iterator_range_t it_pair) {

    // start a source list ID CRC hash
    boost::crc_32_type source_list_crc;

    // add each source ID to the CRC hash
    for (; it_pair.first != it_pair.second; ++it_pair.first) {

      // get source lookup index
      uint64_t source_id = hashdb_manager->source_id(it_pair.first);

      // add source ID to source list ID
      source_list_crc.process_bytes(&source_id, sizeof(source_id));
    }

    return source_list_crc.checksum();
  }

  // print the source list
  void print_source_list(
                 hash_store_key_iterator_range_t it_pair) {

    bool at_start = true;

    // add each source ID to the CRC hash
    for (; it_pair.first != it_pair.second; ++it_pair.first) {

      // manage start of list
      if (at_start) {
        // print opening
        std::cout << ", \"sources\":[";
        at_start = false;
      } else {
        // print continuation
        std::cout << ",";
      }

      // get source lookup index and file offset
      uint64_t source_lookup_index = hashdb_manager->source_id(it_pair.first);
      uint64_t file_offset = hashdb_manager->file_offset(it_pair.first);

      // print the source ID and file offset
      std::cout << "{\"source_id\":" << source_lookup_index
                << ",\"file_offset\":" << file_offset;

      // print full source information the first time
      if (source_ids->find(source_lookup_index) == source_ids->end()) {

        // print the repository name and filename
        std::pair<bool, std::pair<std::string, std::string> > source_pair =
                             hashdb_manager->find_source(source_lookup_index);
        if (source_pair.first == false) {
          assert(0);
        }
        if (source_pair.first == true) {
          std::cout << ",\"repository_name\":\"" << source_pair.second.first
                    << "\",\"filename\":\"" << source_pair.second.second
                    << "\"";
        }

        // if available, print filesize and file hashdigest
        std::pair<bool, source_metadata_t> metadata_pair =
                    hashdb_manager->find_source_metadata(source_lookup_index);
        if (metadata_pair.first == true) {
          // print the metadata
          std::cout << ",\"filesize\":" << metadata_pair.second.filesize
                    << ",\"file_hashdigest\":\""
                    << metadata_pair.second.hashdigest.hexdigest()
                    << "\"";
        }

        // record that this source ID has been printed
        source_ids->insert(source_lookup_index);
      }

      // print source closure
      std::cout << "}";
    }

    // print sources closure
    std::cout << "]";
  }

  public:
  json_formatter_t(hashdb_manager_t* p_hashdb_manager,
                   uint32_t p_max_sources) :
          hashdb_manager(p_hashdb_manager),
          max_sources(p_max_sources),
          source_ids(),
          hashes() {
    source_ids = new std::set<uint64_t>;
    hashes = new std::set<hash_t>;
  }

  ~json_formatter_t() {
    delete source_ids;
    delete hashes;
  }

  // helper to get valid json output, taken from
  // http://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
  std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
    //C++98/03:
    //for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
      switch (*iter) {
        case '\\': ss << "\\\\"; break;
        case '"': ss << "\\\""; break;
        case '/': ss << "\\/"; break;
        case '\b': ss << "\\b"; break;
        case '\f': ss << "\\f"; break;
        case '\n': ss << "\\n"; break;
        case '\r': ss << "\\r"; break;
        case '\t': ss << "\\t"; break;
        default: ss << *iter; break;
      }
    }
    return ss.str();
  }

  // print expanded source information unless the hash has been printed already
  void print_expanded(
          const hash_store_key_iterator_range_t& it_pair) {

//    // skip if hash already processed
//    if (hashes->find(it_pair.first->first) != hashes->end()) {
//      return;
//    }

    // print the block hashdigest
    std::cout << "{\"block_hashdigest\":\"" << key(it_pair.first).hexdigest() << "\"";

    // print the count
    size_t count = source_list_count(it_pair);
    std::cout << ", \"count\":" << count;

    // print the source list ID
    std::cout << ", \"source_list_id\":" << source_list_id(it_pair);

    // print the list of sources unless it is too long
    // or the list for this hash has been printed before
    if (count <= max_sources) {
      if (hashes->find(key(it_pair.first)) == hashes->end()) {
        print_source_list(it_pair);

        // record that expanded information has been printed for this hash
        hashes->insert(key(it_pair.first));
      }
    }

    // close line
    std::cout << "}";

  }
};

#endif

