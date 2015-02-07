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
 * Unfortunately, the existing hashdigest reader output is hard to consume.
 * To get by, we use this consumer, which contains the pointer to the
 * scan input data structure.
 */

#ifndef DFXML_SCAN_EXPANDED_CONSUMER_HPP
#define DFXML_SCAN_EXPANDED_CONSUMER_HPP
#include "hashdb.hpp"
#include "lmdb_hash_store.hpp"
#include "lmdb_source_store.hpp"
#include "json_formatter.hpp"

class dfxml_scan_expanded_consumer_t {

  private:

  lmdb_hash_store_t* hash_store;
  lmdb_source_store_t* source_store;
  json_formatter_t json_formatter;
  std::set<uint32_t> source_list_ids;
  bool found_match;
  std::string filename;

  // do not allow copy or assignment
  dfxml_scan_expanded_consumer_t(const dfxml_scan_expanded_consumer_t&);
  dfxml_scan_expanded_consumer_t& operator=(const dfxml_scan_expanded_consumer_t&);

  public:
  dfxml_scan_expanded_consumer_t(hash_store_t* p_hash_store,
                                 source_store_t* p_soure_store,
                                 uint32_t max_sources) :
          hash_store(p_hash_store),
          source_store(p_source_store),
          json_formatter(hash_store, source_store, max_sources),
          source_list_ids(),
          found_match(false),
          filename("") {
  }

  // end_fileobject_filename
  void end_fileobject_filename(std::string p_filename) {
    // capture the new filename
    filename = p_filename;
  }

  // end_byte_run
  void end_byte_run(const std::string& binary_hash) {

    // find matching range for this hash
    lmdb_hash_it_data_t hash_it_data = hash_store.find_first(binary_hash);

    // no action if no match
    if (hash_it_data.is_vlid == false) {
      return;
    }

    // print filename if first match for this fileobject
    if (found_match == false) {
      found_match = true;
      std::cout << "# begin-processing {\"filename\":\""
                << filename << "\"}"
                << std::endl;
    }

    // print the expanded hash
    json_formatter.print_expanded(hash_store, source_store, hash_it_data);
    std::cout << std::endl;
  }

  // end_fileobject
  void end_fileobject(const std::string& repository_name,
                      const std::string& p_filename,
                      const std::string& hashdigest_type,
                      const std::string& hashdigest,
                      const std::string& filesize) {

    // if matches were found then print closure
    if (found_match == true) {
      // add closure marking and flush
      std::cout << "# end-processing {\"filename\":\""
                << p_filename << "\"}"
                << std::endl;

      found_match = false;
    }
  }
};

#endif

