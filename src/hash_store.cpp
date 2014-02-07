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

#include <config.h>
#include "hash_store.hpp"
#include "hashdb_types.h"
#include "settings.hpp"
#include "dfxml/src/hash_t.h"
#include "dfxml/src/dfxml_writer.h"
#include "manager_modified.h"
#include <cassert>
#include <boost/iterator/iterator_facade.hpp>

// map names
const std::string MAP_RED_BLACK_TREE_NAME = "red_black_tree";
const std::string MAP_SORTED_VECTOR_NAME =  "sorted_vector";
const std::string MAP_HASH_NAME =           "hash";
const std::string MAP_BTREE_NAME =          "btree";

    /**
     * Create a hash store of the given map type and file mode type.
     */
    hash_store_t::hash_store_t (const std::string& _filename,
                  file_mode_type_t _file_mode_type,
                  map_type_t _map_type,
                  uint32_t _map_shard_count) :
                        map_red_black_tree(0),
                        map_sorted_vector(0),
                        map_hash(0),
                        map_btree(0),
                        filename(_filename),
                        file_mode_type(_file_mode_type),
                        map_type(_map_type),
                        map_shard_count(_map_shard_count) {

      // instantiate the map type being used
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          map_red_black_tree = new map_red_black_tree_t(
                MAP_RED_BLACK_TREE_NAME,
                filename, 
                size,
                expected_size,
                map_shard_count,
                file_mode_type);
        break;
        case MAP_SORTED_VECTOR:
          map_sorted_vector = new map_sorted_vector_t(
                MAP_SORTED_VECTOR_NAME,
                filename, 
                size,
                expected_size,
                map_shard_count,
                file_mode_type);
        break;
        case MAP_HASH:
          map_hash = new map_hash_t(
                MAP_HASH_NAME,
                filename, 
                size,
                expected_size,
                map_shard_count,
                file_mode_type);
        break;
        case MAP_BTREE:
          map_btree = new map_btree_t(
                MAP_BTREE_NAME,
                filename, 
                size,
                expected_size,
                map_shard_count,
                file_mode_type);
        break;
        default:
          assert(0);
      }
    }

    /**
     * Close and release resources for the given map.
     */
    hash_store_t::~hash_store_t() {
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          delete map_red_black_tree;
          break;
        case MAP_SORTED_VECTOR:
          delete map_sorted_vector;
          break;
        case MAP_HASH:
          delete map_hash;
          break;
        case MAP_BTREE:
          delete map_btree;
          break;
        default:
          assert(0);
      }
    }

    /**
     * Add the element to the map else fail if already there.
     */
    void hash_store_t::insert_hash_element(const md5_t& md5,
                          uint64_t source_lookup_record) {
//std::cout << "hash_store.insert_hash_element md5:" << md5 << ", source_lookup_record:" << source_lookup_record << "\n";
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          map_red_black_tree->insert_element(md5, source_lookup_record);
          break;
        case MAP_SORTED_VECTOR:
          map_sorted_vector->insert_element(md5, source_lookup_record);
          break;
        case MAP_HASH:
          map_hash->insert_element(md5, source_lookup_record);
          break;
        case MAP_BTREE:
          map_btree->insert_element(md5, source_lookup_record);
          break;
        default:
          assert(0);
      }
    }

    /**
     * Erase the element from the map else fail.
     */
    void hash_store_t::erase_hash_element(const md5_t& md5) {
//std::cout << "hash_store.erase_hash_element md5:" << md5 << "\n";
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          map_red_black_tree->erase_key(md5);
          break;
        case MAP_SORTED_VECTOR:
          map_sorted_vector->erase_key(md5);
          break;
        case MAP_HASH:
          map_hash->erase_key(md5);
          break;
        case MAP_BTREE:
          map_btree->erase_key(md5);
          break;
        default:
          assert(0);
      }
    }

    /**
     * Get the source lookup record from the map if it exists.
     */
    bool hash_store_t::has_source_lookup_record(const md5_t& md5,
                          uint64_t& source_lookup_record) {
      bool has;
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          has = map_red_black_tree->has_key(md5, source_lookup_record);
          break;
        case MAP_SORTED_VECTOR:
          has = map_sorted_vector->has_key(md5, source_lookup_record);
          break;
        case MAP_HASH:
          has = map_hash->has_key(md5, source_lookup_record);
          break;
        case MAP_BTREE:
          has = map_btree->has_key(md5, source_lookup_record);
          break;
        default:
          has = false;
          assert(0);
      }
//std::cout << "hash_store.has_source_lookup_record md5:" << md5 << ", source_lookup_record:" << source_lookup_record << "\n";
      return has;
    }

    /**
     * Change the existing value to a new value in the map,
     * failing if the element to be changed does not exist.
     */
    void hash_store_t::change_source_lookup_record(const md5_t& md5,
                          uint64_t source_lookup_record) {

//std::cout << "hash_store.change_source_lookup_record md5:" << md5 << ", source_lookup_record:" << source_lookup_record << "\n";
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          map_red_black_tree->change_pay(md5, source_lookup_record);
          break;
        case MAP_SORTED_VECTOR:
          map_sorted_vector->change_pay(md5, source_lookup_record);
          break;
        case MAP_HASH:
          map_hash->change_pay(md5, source_lookup_record);
          break;
        case MAP_BTREE:
          map_btree->change_pay(md5, source_lookup_record);
          break;
        default:
          assert(0);
      }
    }

    /**
     * Report status to consumer.
     */
    template void hash_store_t::report_status(std::ostream&) const;
    template void hash_store_t::report_status(dfxml_writer&) const;
    template <class T>
    void hash_store_t::report_status(T& consumer) const {
      switch(map_type) {
        case MAP_RED_BLACK_TREE:
          map_red_black_tree->report_status(consumer);
          break;
        case MAP_SORTED_VECTOR:
          map_sorted_vector->report_status(consumer);
          break;
        case MAP_HASH:
          map_hash->report_status(consumer);
          break;
        case MAP_BTREE:
          map_btree->report_status(consumer);
          break;
        default:
          assert(0);
      }
    }

        // iterator class
        hash_store_t::hash_store_iterator_t::hash_store_iterator_t(const hash_store_t* _hash_store, bool at_end) :
                              hash_store(_hash_store),
                              map_type(_hash_store->map_type),
                              red_black_tree_iterator(),
                              sorted_vector_iterator(),
                              hash_iterator(),
                              btree_iterator(),
                              hash_store_element() {

          if (!at_end) {
            // set begin iterator
            switch(map_type) {
              case MAP_RED_BLACK_TREE:
                red_black_tree_iterator = hash_store->map_red_black_tree->begin();
                hash_store_t::hash_store_iterator_t::set_hash_store_element();
                break;
              case MAP_SORTED_VECTOR:
                sorted_vector_iterator = hash_store->map_sorted_vector->begin();
                set_hash_store_element();
                break;
              case MAP_HASH:
                hash_iterator = hash_store->map_hash->begin();
                set_hash_store_element();
                break;
              case MAP_BTREE:
                btree_iterator = hash_store->map_btree->begin();
                set_hash_store_element();
                break;
              default:
                assert(0);
            }
          } else {
            // set end iterator
            switch(map_type) {
              case MAP_RED_BLACK_TREE:
                red_black_tree_iterator = hash_store->map_red_black_tree->end();
                break;
              case MAP_SORTED_VECTOR:
                sorted_vector_iterator = hash_store->map_sorted_vector->end();
                break;
              case MAP_HASH:
                hash_iterator = hash_store->map_hash->end();
                break;
              case MAP_BTREE:
                btree_iterator = hash_store->map_btree->end();
                break;
              default:
                assert(0);
            }
          }
        }

        hash_store_t::hash_store_iterator_t::hash_store_iterator_t(
                         const hash_store_iterator_t& other) :
              hash_store(other.hash_store),
              map_type(other.map_type),
              red_black_tree_iterator(other.red_black_tree_iterator),
              sorted_vector_iterator(other.sorted_vector_iterator),
              hash_iterator(other.hash_iterator),
              btree_iterator(other.btree_iterator),
              hash_store_element() {
        }

        void hash_store_t::hash_store_iterator_t::set_hash_store_element() {
          switch(map_type) {
            case MAP_RED_BLACK_TREE:
              if (not (red_black_tree_iterator == hash_store->map_red_black_tree->end())) {
                hash_store_element = hash_store_element_t(
                                     red_black_tree_iterator.key(),
                                     red_black_tree_iterator.pay());
              } else {
                hash_store_element = hash_store_element_t();
              }
              break;
            case MAP_SORTED_VECTOR:
              if (not (sorted_vector_iterator == hash_store->map_sorted_vector->end())) {
                hash_store_element = hash_store_element_t(
                                     sorted_vector_iterator.key(),
                                     sorted_vector_iterator.pay());
              } else {
                hash_store_element = hash_store_element_t();
              }
              break;
            case MAP_HASH:
              if (not (hash_iterator == hash_store->map_hash->end())) {
                hash_store_element = hash_store_element_t(
                                     hash_iterator.key(),
                                     hash_iterator.pay());
              } else {
                hash_store_element = hash_store_element_t();
              }
              break;
            case MAP_BTREE:
              if (not (btree_iterator == hash_store->map_btree->end())) {
                hash_store_element = hash_store_element_t(
                                     btree_iterator.key(),
                                     btree_iterator.pay());
              } else {
                hash_store_element = hash_store_element_t();
              }
              break;
            default:
              assert(0);
          }
        }

        hash_store_t::hash_store_iterator_t& hash_store_t::hash_store_iterator_t::operator++() {
          switch(map_type) {
            case MAP_RED_BLACK_TREE:
              ++red_black_tree_iterator;
              set_hash_store_element();
              break;
            case MAP_SORTED_VECTOR:
              ++sorted_vector_iterator;
              set_hash_store_element();
              break;
            case MAP_HASH:
              ++hash_iterator;
              set_hash_store_element();
              break;
            case MAP_BTREE:
              ++btree_iterator;
              set_hash_store_element();
              break;
            default:
              assert(0);
          }
          return *this;
        }

        bool const hash_store_t::hash_store_iterator_t::operator==(const hash_store_t::hash_store_iterator_t& other) const {

          // the iterators must apply to the same map type
          if (this->map_type != other.map_type) {
            return false;
          }

          // the iterators of the specified map type must match
          switch(map_type) {
            case MAP_RED_BLACK_TREE:
              return (this->red_black_tree_iterator) == (other.red_black_tree_iterator);
            case MAP_SORTED_VECTOR:
              return (this->sorted_vector_iterator) == (other.sorted_vector_iterator);
            case MAP_HASH:
              return (this->hash_iterator) == (other.hash_iterator);
            case MAP_BTREE:
              return (this->btree_iterator) == (other.btree_iterator);
            default:
              assert(0);
              return false;
          }
        }

        bool const hash_store_t::hash_store_iterator_t::operator!=(const hash_store_t::hash_store_iterator_t& other) const {
          return (!(*this == other));
        }

        hash_store_element_t const* hash_store_t::hash_store_iterator_t::operator->() const {
          // return the active value
          return &hash_store_element;
        }

    const hash_store_t::hash_store_iterator_t hash_store_t::begin() const {
      return hash_store_iterator_t(this, false);
    }

    const hash_store_t::hash_store_iterator_t hash_store_t::end() const {
      return hash_store_iterator_t(this, true);
    }

