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
 * hashdb manager.
 */

#ifndef DFXML_SCAN_EXPANDED_CONSUMER_HPP
#define DFXML_SCAN_EXPANDED_CONSUMER_HPP
#include "hashdb_element.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_iterator.hpp"

class dfxml_scan_expanded_consumer_t {

  private:
  hashdb_manager_t* hashdb_manager;

  // do not allow copy or assignment
  dfxml_scan_expanded_consumer_t(const dfxml_scan_expanded_consumer_t&);
  dfxml_scan_expanded_consumer_t& operator=(const dfxml_scan_expanded_consumer_t&);

  public:
  dfxml_scan_expanded_consumer_t(hashdb_manager_t* p_hashdb_manager) :
        hashdb_manager(p_hashdb_manager) {
  }

  // to consume, have dfxml_hashdigest_reader call here
  void consume(const hashdb_element_t& hashdb_element) {

    // consume the hashdb_element by scanning and displaying results immediately

    // get range of hash match
    std::pair<hashdb_iterator_t, hashdb_iterator_t > range_it_pair(
                                     hashdb_manager->find(hashdb_element.key));

    while (range_it_pair.first != range_it_pair.second) {
      hashdb_element_t e = *(range_it_pair.first);
      std::cout << e.key.hexdigest() << "\t"
                << "repository_name='" << e.repository_name
                << "', filename='" << e.filename << "\n";
      ++range_it_pair.first;
    }
  }
};

#endif

