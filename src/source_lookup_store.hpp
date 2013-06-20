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
 * Provides interfaces to the source lookup store.
 */

#ifndef SOURCE_LOOKUP_STORE_HPP
#define SOURCE_LOOKUP_STORE_HPP
#include "hashdb_types.h"
#include "hashdb_settings.h"
#include "source_location_record.hpp"
#include "manager_multi_index_container.h"
#include <string>

// provide purposed name for multi_index
typedef manager_multi_index_container_t<uint64_t, fixed_size_source_location_record_t> multi_index_container_t;

/**
 * Provides interfaces to the source lookup store.
 */
class source_lookup_store_t {
  private:
    // each map type, currently just BIMAP
    multi_index_container_t* multi_index_container;

    const std::string filename;
    const file_mode_type_t file_mode_type;
    const source_lookup_settings_t source_lookup_settings;
    const multi_index_container_type_t multi_index_container_type;
    uint64_t next_source_lookup_index;

    const static uint64_t size=1000000;

    // disallow these
    source_lookup_store_t(const source_lookup_store_t&);
    source_lookup_store_t& operator=(const source_lookup_store_t&);

  public:
    source_lookup_store_t (const std::string _filename,
                           file_mode_type_t _file_mode_type,
                           source_lookup_settings_t _source_lookup_settings);

    /**
     * Close and release resources.
     */
    ~source_lookup_store_t();

    /**
     * Determine if a source location record exists.
     */
    bool has_source_location_record(
                    const source_location_record_t& source_location_record);

    /**
     * Insert else fail if left or right already exist.
     */
    void insert_source_lookup_element(uint64_t source_lookup_index,
                       const source_location_record_t& source_location_record);

    /**
     * Get the source location record from the source lookup index else fail.
     */
    void get_source_location_record(uint64_t source_lookup_index,
                       source_location_record_t& source_location_record);

    /**
     * Get the source lookup index from the source location record.
     */
    void get_source_lookup_index(
                  const source_location_record_t& source_location_record,
                  uint64_t& source_lookup_index);

    /**
     * Get the next unallocated source lookup index to use
     * or fail if they are all used up.
     */
    uint64_t new_source_lookup_index();

    /**
     * Report status to consumer.
     */
    template <class T>
    void report_status(T& consumer) const;
};

#endif
