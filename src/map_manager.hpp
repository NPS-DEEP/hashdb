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

#ifndef MAP_MANAGER_HPP
#define MAP_MANAGER_HPP
#include "map_btree.hpp"
#include "map_flat_sorted_vector.hpp"
#include "map_red_black_tree.hpp"
#include "map_unordered_hash.hpp"
#include "multimap_btree.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "file_modes.h"
#include "map_types.h"
#include "dfxml/src/hash_t.h"
#include "hash_algorithm_types.h"
#include "map_iterator.hpp"
#include <boost/functional/hash.hpp>

/*
// provide these for the unordered hash map and multimap
inline std::size_t hash_value(const md5_t& key) {
  return boost::hash_value<unsigned char,16>(key.digest);
}
inline std::size_t hash_value(const sha1_t& key) {
  return boost::hash_value<unsigned char,20>(key.digest);
}
inline std::size_t hash_value(const sha256_t& key) {
  return boost::hash_value<unsigned char,32>(key.digest);
}
*/

/**
 * Provides interfaces to the hash map store that use glue to the actual
 * storage maps used.
 */
template<class T>  // hash type used as key in maps
class map_manager_t {

  // hash_map_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;
  const map_type_t map_type;

  // map models
  map_btree_t<T, uint64_t>*                 map_btree;
  map_flat_sorted_vector_t<T, uint64_t>*    map_flat_sorted_vector;
  map_red_black_tree_t<T, uint64_t>*        map_red_black_tree;
  map_unordered_hash_t<T, uint64_t>*        map_unordered_hash;

  // disallow copy and assignment
  map_manager_t(const map_manager_t&);
  map_manager_t& operator=(const map_manager_t&);

  // helper to translate a specific iterator, bool pair to a generic pair
  std::pair<map_iterator_t<T>, bool> map_pair(
          class map_btree_t<T, uint64_t>::map_pair_it_bool_t pair_it_bool) {
//class map_btree_t<T, uint64_t>::map_pair_it_bool_t temp_pair = pair_it_bool;

    // make generic iterator from specific iterator
    map_iterator_t<T> map_it(pair_it_bool.first);
    // make generic pair from generic iterator and specific bool
    std::pair<map_iterator_t<T>, bool> generic_pair(map_it, pair_it_bool.second);
    return generic_pair;
  }
  std::pair<map_iterator_t<T>, bool> map_pair(
          class map_flat_sorted_vector_t<T, uint64_t>::map_pair_it_bool_t pair_it_bool) {
    // make generic iterator from specific iterator
    map_iterator_t<T> map_it(pair_it_bool.first);
    // make generic pair from generic iterator and specific bool
    std::pair<map_iterator_t<T>, bool> generic_pair(map_it, pair_it_bool.second);
    return generic_pair;
  }
  std::pair<map_iterator_t<T>, bool> map_pair(
          class map_red_black_tree_t<T, uint64_t>::map_pair_it_bool_t pair_it_bool) {
    // make generic iterator from specific iterator
    map_iterator_t<T> map_it(pair_it_bool.first);
    // make generic pair from generic iterator and specific bool
    std::pair<map_iterator_t<T>, bool> generic_pair(map_it, pair_it_bool.second);
    return generic_pair;
  }
  std::pair<map_iterator_t<T>, bool> map_pair(
          class map_unordered_hash_t<T, uint64_t>::map_pair_it_bool_t pair_it_bool) {
    // make generic iterator from specific iterator
    map_iterator_t<T> map_it(pair_it_bool.first);
    // make generic pair from generic iterator and specific bool
    std::pair<map_iterator_t<T>, bool> generic_pair(map_it, pair_it_bool.second);
    return generic_pair;
  }

  public:

  /**
   * Create a hash store of the given map type and file mode type.
   */
  map_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode,
                 map_type_t p_map_type) :
       filename(p_hashdb_dir + "/hash_store"),
       file_mode(p_file_mode),
       map_type(p_map_type),

       map_btree(0),
       map_flat_sorted_vector(0),
       map_red_black_tree(0),
       map_unordered_hash(0) {

    switch(map_type) {
      case MAP_BTREE:
        map_btree = new map_btree_t<T, uint64_t>(filename, file_mode);
        return;
      case MAP_FLAT_SORTED_VECTOR:
        map_flat_sorted_vector = new map_flat_sorted_vector_t<T, uint64_t>(filename, file_mode);
        return;
      case MAP_RED_BLACK_TREE:
        map_red_black_tree = new map_red_black_tree_t<T, uint64_t>(filename, file_mode);
        return;
      case MAP_UNORDERED_HASH:
        map_unordered_hash = new map_unordered_hash_t<T, uint64_t>(filename, file_mode);
        return;
    }
  }

  ~map_manager_t() {
    switch(map_type) {
      case MAP_BTREE: delete map_btree; return;
      case MAP_FLAT_SORTED_VECTOR: delete map_flat_sorted_vector; return;
      case MAP_RED_BLACK_TREE: delete map_red_black_tree; return;
      case MAP_UNORDERED_HASH: delete map_unordered_hash; return;

      default:
        assert(0);
    }
  }

  // emplace
  // return pair from composed map iterator and actual map's bool
  std::pair<map_iterator_t<T>, bool> emplace(const T& key, uint64_t source_lookup_encoding) {
    switch(map_type) {
      case MAP_BTREE:
        return map_pair(map_btree->emplace(key, source_lookup_encoding));
      case MAP_FLAT_SORTED_VECTOR:
        return map_pair(map_flat_sorted_vector->emplace(key, source_lookup_encoding));
      case MAP_RED_BLACK_TREE:
        return map_pair(map_red_black_tree->emplace(key, source_lookup_encoding));
      case MAP_UNORDERED_HASH:
        return map_pair(map_unordered_hash->emplace(key, source_lookup_encoding));

      default:
        assert(0);
    }
  }

  // erase
  size_t erase(const T& key) {
    switch(map_type) {
      case MAP_BTREE: return map_btree->erase(key);
      case MAP_FLAT_SORTED_VECTOR: return map_flat_sorted_vector->erase(key);
      case MAP_RED_BLACK_TREE: return map_red_black_tree->erase(key);
      case MAP_UNORDERED_HASH: return map_unordered_hash->erase(key);

      default:
        assert(0);
    }
  }

  // change
  // return pair from composed map iterator and actual map's bool
  std::pair<map_iterator_t<T>, bool> change(const T& key, uint64_t source_lookup_encoding) {
    switch(map_type) {
      case MAP_BTREE:
        return map_pair(map_btree->change(key, source_lookup_encoding));
      case MAP_FLAT_SORTED_VECTOR:
        return map_pair(map_flat_sorted_vector->change(key, source_lookup_encoding));
      case MAP_RED_BLACK_TREE:
        return map_pair(map_red_black_tree->change(key, source_lookup_encoding));
      case MAP_UNORDERED_HASH:
        return map_pair(map_unordered_hash->change(key, source_lookup_encoding));

      default:
        assert(0);
    }
  }


  // find
  map_iterator_t<T> find(const T& key) {
    switch(map_type) {
      case MAP_BTREE:
        return map_iterator_t<T>(map_btree->find(key));
      case MAP_FLAT_SORTED_VECTOR:
        return map_iterator_t<T>(map_flat_sorted_vector->find(key));
      case MAP_RED_BLACK_TREE:
        return map_iterator_t<T>(map_red_black_tree->find(key));
      case MAP_UNORDERED_HASH:
        return map_iterator_t<T>(map_unordered_hash->find(key));

      default:
        assert(0);
    }
  }

  // has
  bool has(const T& key) {
    switch(map_type) {
      case MAP_BTREE:
        return map_btree->has(key);
      case MAP_FLAT_SORTED_VECTOR:
        return map_flat_sorted_vector->has(key);
      case MAP_RED_BLACK_TREE:
        return map_red_black_tree->has(key);
      case MAP_UNORDERED_HASH:
        return map_unordered_hash->has(key);

      default:
        assert(0);
    }
  }

  // size
  size_t size() {
    switch(map_type) {
      case MAP_BTREE:
        return map_btree->size();
      case MAP_FLAT_SORTED_VECTOR:
        return map_flat_sorted_vector->size();
      case MAP_RED_BLACK_TREE:
        return map_red_black_tree->size();
      case MAP_UNORDERED_HASH:
        return map_unordered_hash->size();

      default:
        assert(0);
    }
  }

/*
  // map stats
inline std::ostream& operator<<(std::ostream& os,
        const class map_stats_t& s) {
  os << "(filename=" << s.filename
     << ", file_mode=" << file_mode_type_to_string(s.file_mode)
     << ", data type name=" << s.data_type_name
     << ", segment size=" << s.segment_size
     << ", count size=" << s.count_size
     << ")";
  return os;
}

  void report_stats(dfxml_writer& x) const {
    x.push("map_stats");
    x.xmlout("filename", filename);
    x.xmlout("file_mode", file_mode_type_to_string(file_mode));
    x.xmlout("data_type_name", data_type_name);
    x.xmlout("segment_size",segment_size);
    x.xmlout("count_size",count_size);
    x.pop();
  }
*/

  // begin
  map_iterator_t<T> const begin() const {
    switch(map_type) {
      case MAP_BTREE:
        return map_iterator_t<T>(map_btree->begin());
      case MAP_FLAT_SORTED_VECTOR:
        return map_iterator_t<T>(map_flat_sorted_vector->begin());
      case MAP_RED_BLACK_TREE:
        return map_iterator_t<T>(map_red_black_tree->begin());
      case MAP_UNORDERED_HASH:
        return map_iterator_t<T>(map_unordered_hash->begin());

      default:
        assert(0);
    }
  }

  // end
  map_iterator_t<T> const end() const {
    switch(map_type) {
      case MAP_BTREE:
        return map_iterator_t<T>(map_btree->end());
      case MAP_FLAT_SORTED_VECTOR:
        return map_iterator_t<T>(map_flat_sorted_vector->end());
      case MAP_RED_BLACK_TREE:
        return map_iterator_t<T>(map_red_black_tree->end());
      case MAP_UNORDERED_HASH:
        return map_iterator_t<T>(map_unordered_hash->end());

      default:
        assert(0);
    }
  }
};
#endif
