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
 * To get by, we use this consumer, which contains pointers to all resources
 * required to consume the imported hashdb elements.
 */

#ifndef DFXML_IMPORT_SOURCE_METADATA_CONSUMER_HPP
#define DFXML_IMPORT_SOURCE_METADATA_CONSUMER_HPP
#include "hashdb_element.hpp"
#include "hashdb_manager.hpp"
#include "progress_tracker.hpp"

class dfxml_import_source_metadata_consumer_t {

  private:
  hashdb_manager_t* hashdb_manager;

  // do not allow copy or assignment
  dfxml_import_source_metadata_consumer_t(
                            const dfxml_import_source_metadata_consumer_t&);
  dfxml_import_source_metadata_consumer_t& operator=(
                            const dfxml_import_source_metadata_consumer_t&);

  public:
  dfxml_import_source_metadata_consumer_t(
              hashdb_manager_t* p_hashdb_manager) :
        hashdb_manager(p_hashdb_manager) {
  }

  // to consume, have dfxml_hashdigest_reader call here
  void consume(const source_metadata_element_t& source_metadata_element) {

    // get existing or create new source ID
    uint64_t source_id = hashdb_manager->insert_source(
                                 source_metadata_element.repository_name,
                                 source_metadata_element.filename);

    // consume the source metadata
    hashdb_manager->insert_source_metadata(source_id,
                                           source_metadata_element.filesize,
                                           source_metadata_element.hashdigest);
  }
};

#endif

