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
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "json_formatter.hpp"
#include "boost/crc.hpp"

class dfxml_scan_expanded_consumer_t {

  private:
  hashdb_manager_t* hashdb_manager;
  json_formatter_t json_formatter;
  std::set<uint32_t> source_list_ids;
  bool found_match = false;
  std::string filename;

  // do not allow copy or assignment
  dfxml_scan_expanded_consumer_t(const dfxml_scan_expanded_consumer_t&);
  dfxml_scan_expanded_consumer_t& operator=(const dfxml_scan_expanded_consumer_t&);

  public:
  dfxml_scan_expanded_consumer_t(hashdb_manager_t* p_hashdb_manager,
                                 uint32_t max_sources) :
          hashdb_manager(p_hashdb_manager),
          json_formatter(hashdb_manager, max_sources),
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
  void end_byte_run(const hashdb_element_t& hashdb_element) {

    // find matching range for this hash
    std::pair<hashdb_manager_t::multimap_iterator_t,
              hashdb_manager_t::multimap_iterator_t> it_pair =
                                     hashdb_manager->find(hashdb_element.key);

    // no action if no match
    if (it_pair.first == it_pair.second) {
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
    json_formatter.print_expanded(it_pair);
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

