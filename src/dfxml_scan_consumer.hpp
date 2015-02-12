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

#ifndef DFXML_SCAN_CONSUMER_HPP
#define DFXML_SCAN_CONSUMER_HPP
#include "hashdb.hpp"
#include "lmdb_helper.h"
#include "lmdb_hash_store.hpp"
#include "lmdb_source_store.hpp"

class dfxml_scan_consumer_t {

  private:

  const lmdb_ro_manager_t* ro_manager;
  bool found_match;
  std::string filename;

  // do not allow copy or assignment
  dfxml_scan_consumer_t(const dfxml_scan_consumer_t&);
  dfxml_scan_consumer_t& operator=(const dfxml_scan_consumer_t&);

  public:
  dfxml_scan_consumer_t(const lmdb_ro_manager_t* p_ro_manager) :
          ro_manager(p_ro_manager),
          found_match(false),
          filename("") {
  }

  // end_fileobject_filename
  void end_fileobject_filename(std::string p_filename) {
    // capture the new filename
    filename = p_filename;
  }

  // end_byte_run
  void end_byte_run(const std::string& binary_hash,
                    uint64_t file_offset,
                    const lmdb_source_data_t& source_data) {

    // find count for this hash
    size_t count = ro_manager->find_count(binary_hash);

    // no action if no match
    if (count == 0) {
      return;
    }

    // print filename if first match for this fileobject
    if (found_match == false) {
      found_match = true;
      std::cout << "# begin-processing {\"filename\":\""
                << filename << "\"}"
                << std::endl;
    }

    // print the hash
    std::cout << "[\"" << lmdb_helper::binary_hash_to_hex(binary_hash)
              << "\",{\"count\":" << count << "}]" << std::endl;
  }

  // end_fileobject
  void end_fileobject(const lmdb_source_data_t& source_data) {

    // if matches were found then print closure
    if (found_match == true) {
      // add closure marking and flush
      std::cout << "# end-processing {\"filename\":\""
                << source_data.filename << "\"}"
                << std::endl;

      found_match = false;
    }
  }
};

#endif

