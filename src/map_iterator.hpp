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
 * Provides map manager iterator.
 */

#ifndef MAP_ITERATOR_HPP
#define MAP_ITERATOR_HPP
#include "map_btree.hpp"
#include "map_flat_sorted_vector.hpp"
#include "map_red_black_tree.hpp"
#include "map_unordered_hash.hpp"
#include "multimap_btree.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "map_types.h"
#include "dfxml/src/hash_t.h"
#include "hash_algorithm_types.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/functional/hash.hpp>

template<class T>
class map_iterator_t : public boost::iterator_facade<
                               map_iterator_t<T>,
                               std::pair<T, uint64_t>,
                               boost::forward_traversal_tag
                              > {
//template<class T>
//class map_iterator_t 
  private:
  friend class boost::iterator_core_access;

  typedef typename map_btree_t<T, uint64_t>::map_const_iterator_t
                                      btree_const_iterator_t;
  typedef typename map_flat_sorted_vector_t<T, uint64_t>::map_const_iterator_t
                                      flat_sorted_vector_const_iterator_t;
  typedef typename map_red_black_tree_t<T, uint64_t>::map_const_iterator_t
                                      red_black_tree_const_iterator_t;
  typedef typename map_unordered_hash_t<T, uint64_t>::map_const_iterator_t
                                      unordered_hash_const_iterator_t;

  map_type_t map_type;
//  const bool at_end;

  // the four iterators, one of which will be used, depending on map_type
  btree_const_iterator_t               btree_const_iterator;
  flat_sorted_vector_const_iterator_t  flat_sorted_vector_const_iterator;
  red_black_tree_const_iterator_t      red_black_tree_const_iterator;
  unordered_hash_const_iterator_t      unordered_hash_const_iterator;

  // the dereferenced value, specifically, std::pair<T, uint64_t>
  std::pair<T, uint64_t> dereferenced_value;

  public:
  // the constructors for each map type using native map iterators
  map_iterator_t(btree_const_iterator_t p_it) :
                      map_type(MAP_BTREE),
                      btree_const_iterator(p_it),
                      flat_sorted_vector_const_iterator(),
                      red_black_tree_const_iterator(),
                      unordered_hash_const_iterator(),
                      dereferenced_value() {
  }

  map_iterator_t(flat_sorted_vector_const_iterator_t p_it) :
                      map_type(MAP_FLAT_SORTED_VECTOR),
                      btree_const_iterator(),
                      flat_sorted_vector_const_iterator(p_it),
                      red_black_tree_const_iterator(),
                      unordered_hash_const_iterator() {
  }

  map_iterator_t(red_black_tree_const_iterator_t p_it) :
                      map_type(MAP_RED_BLACK_TREE),
                      btree_const_iterator(),
                      flat_sorted_vector_const_iterator(),
                      red_black_tree_const_iterator(p_it),
                      unordered_hash_const_iterator() {
  }

  map_iterator_t(unordered_hash_const_iterator_t p_it) :
                      map_type(MAP_UNORDERED_HASH),
                      btree_const_iterator(),
                      flat_sorted_vector_const_iterator(),
                      red_black_tree_const_iterator(),
                      unordered_hash_const_iterator(p_it) {
  }

  // useless default constructor is required by std::pair
  map_iterator_t() :
                      map_type(MAP_BTREE),   // had to pick one
                      btree_const_iterator(),
                      flat_sorted_vector_const_iterator(),
                      red_black_tree_const_iterator(),
                      unordered_hash_const_iterator() {
  }

  // copy capability is required by std::pair
  map_iterator_t& operator=(const map_iterator_t& other) {
    map_type = other.map_type;
    btree_const_iterator = other.btree_const_iterator;
    flat_sorted_vector_const_iterator = other.flat_sorted_vector_const_iterator;
    red_black_tree_const_iterator = other.red_black_tree_const_iterator;
    unordered_hash_const_iterator = other.unordered_hash_const_iterator;
    return *this;
  }

/* no
  // keep warning quiet even though this is a POD
  ~map_iterator_t() {
  }
*/

  // for iterator_facade
  void increment() {
    switch(map_type) {
      case MAP_BTREE:              ++btree_const_iterator; return;
      case MAP_FLAT_SORTED_VECTOR: ++flat_sorted_vector_const_iterator; return;
      case MAP_RED_BLACK_TREE:     ++red_black_tree_const_iterator; return;
      case MAP_UNORDERED_HASH:     ++unordered_hash_const_iterator; return;
      default: assert(0);
    }
  }

  // for iterator_facade
  bool equal(map_iterator_t<T> const& other) const {
    switch(map_type) {
      case MAP_BTREE: return this->btree_const_iterator ==
                             other.btree_const_iterator;
      case MAP_FLAT_SORTED_VECTOR: return this->flat_sorted_vector_const_iterator ==
                             other.flat_sorted_vector_const_iterator;
      case MAP_RED_BLACK_TREE: return this-> red_black_tree_const_iterator ==
                             other.red_black_tree_const_iterator;
      case MAP_UNORDERED_HASH: return this-> unordered_hash_const_iterator ==
                             other.unordered_hash_const_iterator;
      default: assert(0);
    }
  }

  // for iterator_facade
  std::pair<T, uint64_t>& dereference() const {
std::pair<T, uint64_t> temp;
    switch(map_type) {
      case MAP_BTREE:
        temp = *btree_const_iterator; return temp;
      case MAP_FLAT_SORTED_VECTOR:
        temp = *flat_sorted_vector_const_iterator; return temp;
      case MAP_RED_BLACK_TREE:
        temp = *red_black_tree_const_iterator; return temp;
      case MAP_UNORDERED_HASH:
        temp = *unordered_hash_const_iterator; return temp;
//      case MAP_BTREE: return *btree_const_iterator;
//      case MAP_FLAT_SORTED_VECTOR: return *flat_sorted_vector_const_iterator;
//      case MAP_RED_BLACK_TREE: return *red_black_tree_const_iterator;
//      case MAP_UNORDERED_HASH: return *unordered_hash_const_iterator;
      default: assert(0);
    }
  }
};

/*
typedef map_iterator__<md5_t> map_iterator_md5_t;
typedef map_iterator__<sha1_t> map_iterator_sha1_t;
typedef map_iterator__<sha256_t> map_iterator_sha256_t;
*/

#endif

