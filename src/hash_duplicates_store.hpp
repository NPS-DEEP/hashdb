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
 * Provides interfaces to the hash duplicates store
 * by providing glue interfaces to the actual storage maps used.
 */

#ifndef HASH_DUPLICATES_STORE_HPP
#define HASH_DUPLICATES_STORE_HPP
#include "hashdb_types.h"
#include "hashdb_settings.h"
#include "manager_modified_multimap.h"
#include "dfxml/src/hash_t.h"
#include <vector>

// define glue typedefs
typedef burst_manager_multimap<md5_t, uint64_t>           multimap_red_black_tree_t;
typedef burst_manager_flat_multimap<md5_t, uint64_t>      multimap_sorted_vector_t;
typedef burst_manager_unordered_multimap<md5_t, uint64_t> multimap_hash_t;
typedef burst_manager_btree_multimap<md5_t, uint64_t>     multimap_btree_t;

/**
 * Provides interfaces to the hash duplicates store
 * by providing glue interfaces to the actual storage maps used.
 */
class hash_duplicates_store_t {
  private:
    multimap_red_black_tree_t*  multimap_red_black_tree;
    multimap_sorted_vector_t*   multimap_sorted_vector;
    multimap_hash_t*            multimap_hash;
    multimap_btree_t*           multimap_btree;

    const std::string filename;
    const file_mode_type_t file_mode_type;
    const hash_duplicates_store_settings_t hash_duplicates_store_settings;
    const multimap_type_t multimap_type;

    const static uint64_t size = 1000000;
    const static uint64_t expected_size = 1000000;

    // don't allow these
    hash_duplicates_store_t(const hash_duplicates_store_t&);
    hash_duplicates_store_t& operator=(const hash_duplicates_store_t&);

  public:
    hash_duplicates_store_t (const std::string& _filename,
                             file_mode_type_t _file_mode_type,
                             hash_duplicates_store_settings_t _hash_duplicates_store_settings);

    ~hash_duplicates_store_t();

    /**
     * Identify if the element is present in the map.
     */
    bool has_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record);

    /**
     * Insert a hash element; the element must not already be there.
     */
    void insert_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record);

    /**
     * Remove a hash element; the element must be there.
     */
    void erase_hash_element(const md5_t& md5,
                       uint64_t source_lookup_record);

    /**
     * Obtain the vector of source lookup records,
     * failing if there are less than two.
     */
    void get_source_lookup_record_vector(const md5_t& md5,
             std::vector<uint64_t>& source_lookup_record_vector);

    /**
     * Get the number of elements that match this md5.
     */
    size_t get_match_count(const md5_t& md5);

    /**
     * Report status to consumer.
     */
    template <class T>
    void report_status(T& consumer) const;
};

#endif
