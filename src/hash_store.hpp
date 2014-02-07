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
 * Provides interfaces to the hash store
 * by providing glue interfaces to the actual storage maps used.
 */

#ifndef HASH_STORE_HPP
#define HASH_STORE_HPP
//#include "hashdb_types.h"
#include "hashdigest_types.h"
#include "settings.hpp"
#include "dfxml/src/hash_t.h"
#include "manager_modified.h"
#include <boost/iterator/iterator_facade.hpp>

typedef std::pair<md5_t, uint64_t> hash_store_element_t;

// glue typedefs map burst_manager types for hash store
typedef burst_manager_map<md5_t, uint64_t>           map_red_black_tree_t;
typedef burst_manager_flat_map<md5_t, uint64_t>      map_sorted_vector_t;
typedef burst_manager_unordered_map<md5_t, uint64_t> map_hash_t;
typedef burst_manager_btree_map<md5_t, uint64_t>     map_btree_t;

/**
 * Provides interfaces to the hash store
 * by providing glue interfaces to the actual storage maps used.
 */
class hash_store_t {

  private:
    friend class hash_store_iterator_t;

//    map_red_black_tree*  hash_store_map<map_red_black_tree_t>;
//    map_sorted_vector*   hash_store_map<map_sorted_vector_t>;
//    map_hash*            hash_store_map<map_hash_t>;
//    map_btree*           hash_store_map<map_btree_t>;

    map_red_black_tree_t*  map_red_black_tree;
    map_sorted_vector_t*   map_sorted_vector;
    map_hash_t*            map_hash;
    map_btree_t*           map_btree;

    const std::string filename;
    const file_mode_type_t file_mode_type;
    const map_type_t map_type;
    const uint32_t map_shard_count;

    const static uint64_t size = 1000000;
    const static uint64_t expected_size = 1000000;

    // disallow these
    hash_store_t(const hash_store_t&);
    hash_store_t& operator=(const hash_store_t&);

  public:
    /**
     * Create a hash store of the given map type and file mode type.
     */
    hash_store_t(const std::string& _filename,
                 file_mode_type_t _file_mode_type,
                 map_type_t _map_type,
                 uint32_t map_shard_count);

    /**
     * Close and release resources for the given map.
     */
    ~hash_store_t();

    /**
     * Add the element to the map else fail if already there.
     */
    void insert_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record);

    /**
     * Erase the element from the map else fail.
     */
    void erase_hash_element(const md5_t& md5);

    /**
     * Get the source lookup record from the map
     * else fail if the source lookup record does not exist.
     */
    bool has_source_lookup_record(const md5_t& md5,
                          uint64_t& source_lookup_record);

    /**
     * Change the existing value to a new value in the map,
     * failing if the element to be changed does not exist.
     */
    void change_source_lookup_record(const md5_t& md5,
                          uint64_t source_lookup_record);

    /**
     * Report status to consumer.
     */
    template <class T>
    void report_status(T& consumer) const;

    /**
     * Iterator for the entire collection of hash store objects,
     * where dereferenced values are
     * in the form of pair of md5_t and uint64_t.
     */
    class hash_store_iterator_t {

      private:
        const hash_store_t* hash_store;
        const map_type_t map_type; // convenience variable

        // the hash store map iterator to use
        map_red_black_tree_t::manager_iterator red_black_tree_iterator;
        map_sorted_vector_t::manager_iterator sorted_vector_iterator;
        map_hash_t::manager_iterator hash_iterator;
        map_btree_t::manager_iterator btree_iterator;

        // the composed hash store element to provide as a dereferenced value
        hash_store_element_t hash_store_element;

        // helper
        void set_hash_store_element();

      public:
        hash_store_iterator_t(const hash_store_t* _hash_store, bool at_end);
        hash_store_iterator_t(const hash_store_iterator_t& other);
        hash_store_iterator_t& operator++();
        hash_store_iterator_t& operator=(const hash_store_iterator_t& other);
        bool const operator==(const hash_store_iterator_t& other) const;
        bool const operator!=(const hash_store_iterator_t& other) const;
        hash_store_element_t const* operator->() const;
    };

    const hash_store_iterator_t begin() const;

    const hash_store_iterator_t end() const;
};
#endif
