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
 * Provides interfaces to the hash map store using glue to the actual
 * storage maps used.
 */

#ifndef MULTIMAP_MANAGER_HPP
#define MULTIMAP_MANAGER_HPP
#include "multimap_btree.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "file_modes.h"
#include "multimap_types.h"
#include "multimap_iterator.hpp"

/**
 * Provides interfaces to the hash map store that use glue to the actual
 * storage maps used.
 */
template<class T>  // hash type used as key in multimaps
class multimap_manager_t {

  // multimap_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;
  const multimap_type_t multimap_type;

  // map models
  multimap_btree_t<T, uint64_t>*                 multimap_btree;
  multimap_flat_sorted_vector_t<T, uint64_t>*    multimap_flat_sorted_vector;
  multimap_red_black_tree_t<T, uint64_t>*        multimap_red_black_tree;
  multimap_unordered_hash_t<T, uint64_t>*        multimap_unordered_hash;

  // disallow copy and assignment
  multimap_manager_t(const multimap_manager_t&);
  multimap_manager_t& operator=(const multimap_manager_t&);

  public:

  /**
   * Create a hash store of the given map type and file mode type.
   */
  multimap_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode,
                 multimap_type_t p_map_type) :
       filename(p_hashdb_dir + "/hash_store"),
       file_mode(p_file_mode),
       multimap_type(p_map_type),

       multimap_btree(0),
       multimap_flat_sorted_vector(0),
       multimap_red_black_tree(0),
       multimap_unordered_hash(0) {

    switch(multimap_type) {
      case MULTIMAP_BTREE:
        multimap_btree = new multimap_btree_t<T, uint64_t>(filename, file_mode);
        return;
      case MULTIMAP_FLAT_SORTED_VECTOR:
        multimap_flat_sorted_vector = new multimap_flat_sorted_vector_t<T, uint64_t>(filename, file_mode);
        return;
      case MULTIMAP_RED_BLACK_TREE:
        multimap_red_black_tree = new multimap_red_black_tree_t<T, uint64_t>(filename, file_mode);
        return;
      case MULTIMAP_UNORDERED_HASH:
        multimap_unordered_hash = new multimap_unordered_hash_t<T, uint64_t>(filename, file_mode);
        return;
    }
  }

  ~multimap_manager_t() {
    switch(multimap_type) {
      case MULTIMAP_BTREE: delete multimap_btree; return;
      case MULTIMAP_FLAT_SORTED_VECTOR: delete multimap_flat_sorted_vector; return;
      case MULTIMAP_RED_BLACK_TREE: delete multimap_red_black_tree; return;
      case MULTIMAP_UNORDERED_HASH: delete multimap_unordered_hash; return;

      default:
        assert(0);
    }
  }

  // emplace
  bool emplace(const T& key, uint64_t source_lookup_encoding) {
    switch(multimap_type) {
      case MULTIMAP_BTREE:
        return multimap_btree->emplace(key, source_lookup_encoding);
      case MULTIMAP_FLAT_SORTED_VECTOR:
        return multimap_flat_sorted_vector->emplace(key, source_lookup_encoding);
      case MULTIMAP_RED_BLACK_TREE:
        return multimap_red_black_tree->emplace(key, source_lookup_encoding);
      case MULTIMAP_UNORDERED_HASH:
        return multimap_unordered_hash->emplace(key, source_lookup_encoding);

      default:
        assert(0);
    }
  }

  // erase
  bool erase(const T& key, uint64_t source_lookup_encoding) {
    switch(multimap_type) {
      case MULTIMAP_BTREE: return multimap_btree->erase(key, source_lookup_encoding);
      case MULTIMAP_FLAT_SORTED_VECTOR: return multimap_flat_sorted_vector->erase(key, source_lookup_encoding);
      case MULTIMAP_RED_BLACK_TREE: return multimap_red_black_tree->erase(key, source_lookup_encoding);
      case MULTIMAP_UNORDERED_HASH: return multimap_unordered_hash->erase(key, source_lookup_encoding);

      default:
        assert(0);
    }
  }

  // equal_range
  std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> > equal_range(const T& key) {
    switch(multimap_type) {
      case MULTIMAP_BTREE: {
        typename multimap_btree_t<T, uint64_t>::map_const_iterator_range_t it1 =
                  multimap_btree->equal_range(key);
        return std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >(
                  multimap_iterator_t<T>(it1.first, false),
                  multimap_iterator_t<T>(it1.second, true));
      }
      case MULTIMAP_FLAT_SORTED_VECTOR: {
        typename multimap_flat_sorted_vector_t<T, uint64_t>::map_const_iterator_range_t it2 =
                  multimap_flat_sorted_vector->equal_range(key);
        return std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >(
                  multimap_iterator_t<T>(it2.first, false),
                  multimap_iterator_t<T>(it2.second, true));
      }
      case MULTIMAP_RED_BLACK_TREE: {
        typename multimap_red_black_tree_t<T, uint64_t>::map_const_iterator_range_t it3 =
                  multimap_red_black_tree->equal_range(key);
        return std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >(
                  multimap_iterator_t<T>(it3.first, false),
                  multimap_iterator_t<T>(it3.second, true));
      }
      case MULTIMAP_UNORDERED_HASH: {
        typename multimap_unordered_hash_t<T, uint64_t>::map_const_iterator_range_t it4 =
                  multimap_unordered_hash->equal_range(key);
        return std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >(
                  multimap_iterator_t<T>(it4.first, false),
                  multimap_iterator_t<T>(it4.second, true));
      }

      default:
        assert(0);
    }
  }

  // has
  bool has(const T& key, uint64_t source_lookup_encoding) {
    switch(multimap_type) {
      case MULTIMAP_BTREE:
        return multimap_btree->has(key, source_lookup_encoding);
      case MULTIMAP_FLAT_SORTED_VECTOR:
        return multimap_flat_sorted_vector->has(key, source_lookup_encoding);
      case MULTIMAP_RED_BLACK_TREE:
        return multimap_red_black_tree->has(key, source_lookup_encoding);
      case MULTIMAP_UNORDERED_HASH:
        return multimap_unordered_hash->has(key, source_lookup_encoding);

      default:
        assert(0);
    }
  }

  // size
  size_t size() {
    switch(multimap_type) {
      case MULTIMAP_BTREE:
        return multimap_btree->size();
      case MULTIMAP_FLAT_SORTED_VECTOR:
        return multimap_flat_sorted_vector->size();
      case MULTIMAP_RED_BLACK_TREE:
        return multimap_red_black_tree->size();
      case MULTIMAP_UNORDERED_HASH:
        return multimap_unordered_hash->size();

      default:
        assert(0);
    }
  }
};
#endif
