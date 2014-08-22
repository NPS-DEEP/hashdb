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

#ifndef DFXML_HASHDIGEST_SCAN_CONSUMER_HPP
#define DFXML_HASHDIGEST_SCAN_CONSUMER_HPP
#include "hashdb.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"

class dfxml_scan_consumer_t {

  private:
  std::vector<hash_t>* scan_input;

  // do not allow copy or assignment
  dfxml_scan_consumer_t(const dfxml_scan_consumer_t&);
  dfxml_scan_consumer_t& operator=(const dfxml_scan_consumer_t&);

  public:
  dfxml_scan_consumer_t(
              std::vector<hash_t>* p_scan_input) :
        scan_input(p_scan_input) {
  }

  // to consume, have dfxml_hashdigest_reader call here
  void consume(const hashdb_element_t& hashdb_element) {

    // consume the hashdb_element by adding it to scan_input
    scan_input->push_back(hashdb_element.key);
  }
};

#endif

