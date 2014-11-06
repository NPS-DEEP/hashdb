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

#ifndef DFXML_SCAN_EXPANDED_SMC_HPP
#define DFXML_SCAN_EXPANDED_SMC_HPP
#include "hashdb.hpp"
#include "hashdb_manager.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "source_metadata_element.hpp"
#include "json_helper.hpp"

class dfxml_scan_expanded_smc_t{

  private:
  hashdb_manager_t* hashdb_manager;
  std::vector<hash_t>* hashes;
  // do not allow copy or assignment
  dfxml_scan_expanded_smc_t(const dfxml_scan_expanded_smc_t&);
  dfxml_scan_expanded_smc_t& operator=(const dfxml_scan_expanded_smc_t&);

  public:
  dfxml_scan_expanded_smc_t(hashdb_manager_t* p_hashdb_manager,
                              std::vector<hash_t>* p_hashes) :
           hashdb_manager(p_hashdb_manager), hashes(p_hashes) {
  }

  // to consume, have dfxml_hashdigest_reader call here
  void consume(const source_metadata_element_t& source_metadata_element) {
    bool found_match = false;
    for (std::vector<hash_t>::iterator it = hashes->begin(); it != hashes->end(); ++it) {

      // find matching range for this key
      std::pair<hashdb_manager_t::multimap_iterator_t,
                hashdb_manager_t::multimap_iterator_t> it_pair =
                                                  hashdb_manager->find(*it);

      // go through each source for this hash
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // print fileobject information header obtained from the DFXML file
        if (found_match == false) {
          found_match = true;
          std::cout << "# begin-processing {\"filename\":\"" << source_metadata_element.filename << "\"}\n";
        }

        // print the hash
        std::cout << "[\"" << it->hexdigest() << "\", {";

        // get source lookup index
        uint64_t source_id = hashdb_manager->source_id(it_pair.first);

        // print source fields
        json_helper_t::print_source_fields(*hashdb_manager, source_id);

        // close the JSON line
        std::cout << "}\n";
      }
    }
    if (found_match == true) {
      // add closure marking and flush
      std::cout << "# end-processing {\"filename\":\"" << source_metadata_element.filename << "\"}" << std::endl;
    }

    // clear out the hashes since they have been consumed
    hashes->clear();
  }
};

#endif

