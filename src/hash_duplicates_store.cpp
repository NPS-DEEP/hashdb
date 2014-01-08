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

#include <config.h>
#include "hashdb_types.h"
#include "dfxml/src/hash_t.h"
#include "dfxml/src/dfxml_writer.h"
#include "manager_modified_multimap.h"
#include "hash_duplicates_store.hpp"
#include <vector>

// map names
const std::string MULTIMAP_RED_BLACK_TREE_NAME = "red_black_tree_duplicates";
const std::string MULTIMAP_SORTED_VECTOR_NAME =  "sorted_vector_duplicates";
const std::string MULTIMAP_HASH_NAME =           "hash_duplicates";
const std::string MULTIMAP_BTREE_NAME =          "btree_duplicates";


    hash_duplicates_store_t::hash_duplicates_store_t (
             const std::string& _filename,
             file_mode_type_t _file_mode_type,
             multimap_type_t _multimap_type,
             uint32_t _multimap_shard_count) :
                multimap_red_black_tree(0),
                multimap_sorted_vector(0),
                multimap_hash(0),
                multimap_btree(0),
                filename(_filename),
                file_mode_type(_file_mode_type),
                multimap_type(_multimap_type),
                multimap_shard_count(_multimap_shard_count) {

      // instantiate the multimap type being used
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          multimap_red_black_tree = new multimap_red_black_tree_t(
                MULTIMAP_RED_BLACK_TREE_NAME,
                filename, 
                size,
                expected_size,
                multimap_shard_count,
                file_mode_type);
        break;
        case MULTIMAP_SORTED_VECTOR:
          multimap_sorted_vector = new multimap_sorted_vector_t(
                MULTIMAP_SORTED_VECTOR_NAME,
                filename, 
                size,
                expected_size,
                multimap_shard_count,
                file_mode_type);
        break;
        case MULTIMAP_HASH:
          multimap_hash = new multimap_hash_t(
                MULTIMAP_HASH_NAME,
                filename, 
                size,
                expected_size,
                multimap_shard_count,
                file_mode_type);
        break;
        case MULTIMAP_BTREE:
          multimap_btree = new multimap_btree_t(
                MULTIMAP_BTREE_NAME,
                filename, 
                size,
                expected_size,
                multimap_shard_count,
                file_mode_type);
        break;
        default:
          assert(0);
      }
    }

    /**
     * Close and release resources for the given map.
     */
    hash_duplicates_store_t::~hash_duplicates_store_t() {
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          delete multimap_red_black_tree;
          break;
        case MULTIMAP_SORTED_VECTOR:
          delete multimap_sorted_vector;
          break;
        case MULTIMAP_HASH:
          delete multimap_hash;
          break;
        case MULTIMAP_BTREE:
          delete multimap_btree;
          break;
        default:
          assert(0);
      }
    }

    /**
     * Identify if the element is present in the map.
     */
    bool hash_duplicates_store_t::has_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record) {

      bool has;
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          has = multimap_red_black_tree->has_element(md5, source_lookup_record);
          break;
        case MULTIMAP_SORTED_VECTOR:
          has = multimap_sorted_vector->has_element(md5, source_lookup_record);
          break;
        case MULTIMAP_HASH:
          has = multimap_hash->has_element(md5, source_lookup_record);
          break;
        case MULTIMAP_BTREE:
          has = multimap_btree->has_element(md5, source_lookup_record);
          break;

        default:
          assert(0);
          has = false;
      }
//std::cout << "hash_duplicates_store.has_hash_element md5:" << md5 << ", source_lookup_record:" << source_lookup_record << " " << has << "\n";
      return has;
    }

    /**
     * Add the element to the map else fail if already there.
     */
    void hash_duplicates_store_t::insert_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record) {
//std::cout << "hash_duplicates_store.insert_hash_element md5:" << md5 << ", source_lookup_record:" << source_lookup_record << "\n";
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          multimap_red_black_tree->insert_element(md5, source_lookup_record);
          break;
        case MULTIMAP_SORTED_VECTOR:
          multimap_sorted_vector->insert_element(md5, source_lookup_record);
          break;
        case MULTIMAP_HASH:
          multimap_hash->insert_element(md5, source_lookup_record);
          break;
        case MULTIMAP_BTREE:
          multimap_btree->insert_element(md5, source_lookup_record);
          break;
        default:
          assert(0);
      }
    }

    /**
     * Erase the element from the map else fail.
     */
    void hash_duplicates_store_t::erase_hash_element(const md5_t& md5,
                       uint64_t source_lookup_record) {
//std::cout << "hash_duplicates_store.erase_hash_element md5:" << md5 << ", source_lookup_record:" << source_lookup_record << "\n";
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          multimap_red_black_tree->erase_element(md5, source_lookup_record);
          break;
        case MULTIMAP_SORTED_VECTOR:
          multimap_sorted_vector->erase_element(md5, source_lookup_record);
          break;
        case MULTIMAP_HASH:
          multimap_hash->erase_element(md5, source_lookup_record);
          break;
        case MULTIMAP_BTREE:
          multimap_btree->erase_element(md5, source_lookup_record);
          break;
        default:
          assert(0);
      }
    }

    void hash_duplicates_store_t::get_source_lookup_record_vector(const md5_t& md5,
             std::vector<uint64_t>& source_lookup_record_vector) {

      // clear the recycled vector
      source_lookup_record_vector.clear();

      // append source lookup records
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          multimap_red_black_tree->append_pay_vector(md5, source_lookup_record_vector);
          break;
        case MULTIMAP_SORTED_VECTOR:
          multimap_sorted_vector->append_pay_vector(md5, source_lookup_record_vector);
          break;
        case MULTIMAP_HASH:
          multimap_hash->append_pay_vector(md5, source_lookup_record_vector);
          break;
        case MULTIMAP_BTREE:
          multimap_btree->append_pay_vector(md5, source_lookup_record_vector);
          break;
        default:
          assert(0);
      }
//std::cout << "hash_duplicates_store.get_source_lookup_record_vector md5:" <<  md5 << " count " << source_lookup_record_vector.size() << "\n";
//for (std::vector<uint64_t>::const_iterator it = source_lookup_record_vector.begin(); it!= source_lookup_record_vector.end(); ++it) {
//std::cout << "hash_duplicates_store.get_source_lookup_record#i:" << *it << "\n";
//}

      // it is a program error if number of records < 2
      if (source_lookup_record_vector.size() < 2) {
//std::cout << "hds.slrv size " << source_lookup_record_vector.size() << "\n";
        assert(0);
      }
    }

    // this may never be used or just used for validation
    size_t hash_duplicates_store_t::get_match_count(const md5_t& md5) {
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          return multimap_red_black_tree->get_match_count(md5);
        case MULTIMAP_SORTED_VECTOR:
          return multimap_sorted_vector->get_match_count(md5);
        case MULTIMAP_HASH:
          return multimap_hash->get_match_count(md5);
        case MULTIMAP_BTREE:
          return multimap_btree->get_match_count(md5);
        default:
          assert(0);
          return 0;
      }
    }

    /**
     * Report status to consusmer.
     */
    template void hash_duplicates_store_t::report_status(std::ostream&) const;
    template void hash_duplicates_store_t::report_status(dfxml_writer&) const;
    template <class T>
    void hash_duplicates_store_t::report_status(T& consumer) const {
      switch(multimap_type) {
        case MULTIMAP_RED_BLACK_TREE:
          multimap_red_black_tree->report_status(consumer);
          break;
        case MULTIMAP_SORTED_VECTOR:
          multimap_sorted_vector->report_status(consumer);
          break;
        case MULTIMAP_HASH:
          multimap_hash->report_status(consumer);
          break;
        case MULTIMAP_BTREE:
          multimap_btree->report_status(consumer);
          break;
        default:
          assert(0);
      }
    }

