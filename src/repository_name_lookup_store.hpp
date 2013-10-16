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
 * Provides interfaces to the repository name lookup store.
 */

#ifndef REPOSITORY_NAME_LOOKUP_STORE_HPP
#define REPOSITORY_NAME_LOOKUP_STORE_HPP
#include <string>
#include "indexed_string_t.hpp"
#include "boost/btree/btree_index_set.hpp"

/**
 * Provides interfaces to the repository name lookup store.
 */
class repository_name_lookup_store_t {
  private:

    // maps
    typedef boost::btree::btree_index_set<indexed_string_t> map_by_index_t;
    typedef boost::btree::btree_index_set<indexed_string_t, default_traits, value_ordering> map_by_value_t;

    const std::string hashdb_dir;
    const file_mode_type_t file_mode;

    // pointers to map_by_index and map_by_value
    map_by_index_t* map_by_index;
    map_by_value_t* map_by_value;

    // disallow these
    repository_name_lookup_store_t(const repository_name_lookup_store_t&);
    repository_name_lookup_store_t& operator=(const repository_name_lookup_store_t&);

  public:
    repository_name_lookup_store_t (const std::string p_hashdb_dir,
                           file_mode_type_t p_file_mode) :
        hashdb_dir(p_hashdb_dir), file_mode_type(p_file_mode_type,
        map_by_index(0), map_by_value(0) {

      // derive the store filenames
      std::string 

      // instantiate the lookup store based on file mode
      if (file_mode == READ_ONLY) {
        map_by_index = new map_by_index_t(
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
