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
 * To get by, we use this consumer.
 */

#ifndef DFXML_IMPORT_CONSUMER_HPP
#define DFXML_IMPORT_CONSUMER_HPP
#include "lmdb_source_data.hpp"
#include "lmdb_rw_manager.hpp"
#include "progress_tracker.hpp"

class dfxml_import_consumer_t {

  private:
  lmdb_rw_manager_t* rw_manager;
  progress_tracker_t* progress_tracker;

  // do not allow copy or assignment
  dfxml_import_consumer_t(const dfxml_import_consumer_t&);
  dfxml_import_consumer_t& operator=(const dfxml_import_consumer_t&);

  public:
  dfxml_import_consumer_t(
              lmdb_rw_manager_t* p_rw_manager,
              progress_tracker_t* p_progress_tracker) :
        rw_manager(p_rw_manager),
        progress_tracker(p_progress_tracker) {
  }

  // end_fileobject_filename
  void end_fileobject_filename(const std::string& filename) {
    // no action for this consumer
  }

  // end_byte_run
  void end_byte_run(const std::string& binary_hash,
                    uint64_t file_offset,
                    uint32_t hash_block_size,
                    const lmdb_source_data_t& source_data,
                    const std::string& hash_label) {

    // update progress tracker
    progress_tracker->track();

    // consume the element by importing it
    rw_manager->insert(binary_hash, file_offset, hash_block_size, source_data,
                       hash_label);
  }

  // end_fileobject
  void end_fileobject(const lmdb_source_data_t source_data) {

    // insert the source metadata
    rw_manager->add_source_data(source_data);
  }
};

#endif

