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
 * Provides multimap iterator.
 * Constructor takes pair from equal_range.
 * If not an end iterator, then it can increment from begin to end of range.
 * If an end iterator, then it will be at end of range, and cannot increment.
 */

#ifndef MULTIMAP_ITERATOR_HPP
#define MULTIMAP_ITERATOR_HPP
#include "multimap_btree.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "multimap_types.h"

template<class T>
class multimap_iterator_t {
  private:

  typedef typename multimap_btree_t<T, uint64_t>::map_const_iterator_t
                                      btree_it_t;
  typedef typename multimap_flat_sorted_vector_t<T, uint64_t>::map_const_iterator_t
                                      flat_sorted_vector_it_t;
  typedef typename multimap_red_black_tree_t<T, uint64_t>::map_const_iterator_t
                                      red_black_tree_it_t;
  typedef typename multimap_unordered_hash_t<T, uint64_t>::map_const_iterator_t
                                      unordered_hash_it_t;

  typedef typename std::pair<btree_it_t, btree_it_t>
                                      btree_it_pair_t;
  typedef typename std::pair<flat_sorted_vector_it_t, flat_sorted_vector_it_t>
                                      flat_sorted_vector_it_pair_t;
  typedef typename std::pair<red_black_tree_it_t, red_black_tree_it_t>
                                      red_black_tree_it_pair_t;
  typedef typename std::pair<unordered_hash_it_t, unordered_hash_it_t>
                                      unordered_hash_it_pair_t;



  multimap_type_t map_type;

  // the four iterator sets, one of which will be used, depending on map_type
  btree_it_t                btree_it;
  btree_it_t                btree_end_it;
  flat_sorted_vector_it_t   flat_sorted_vector_it;
  flat_sorted_vector_it_t   flat_sorted_vector_end_it;
  red_black_tree_it_t       red_black_tree_it;
  red_black_tree_it_t       red_black_tree_end_it;
  unordered_hash_it_t       unordered_hash_it;
  unordered_hash_it_t       unordered_hash_end_it;

  // the dereferenced value, specifically, std::pair<T, uint64_t>
  std::pair<T, uint64_t> dereferenced_value;

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    switch(map_type) {
      case MULTIMAP_BTREE: {
        // program error to increment begin iterator when it is at end
        if (btree_it == btree_end_it) {
          assert(0);
        }
        ++btree_it;
        return;
      }
      case MULTIMAP_FLAT_SORTED_VECTOR: {
        // program error to increment begin iterator when it is at end
        if (flat_sorted_vector_it == flat_sorted_vector_end_it) {
          assert(0);
        }
        ++flat_sorted_vector_it;
        return;
      }
      case MULTIMAP_RED_BLACK_TREE: {
        // program error to increment begin iterator when it is at end
        if (red_black_tree_it == red_black_tree_end_it) {
          assert(0);
        }
        ++red_black_tree_it;
        return;
      }
      case MULTIMAP_UNORDERED_HASH: {
        // program error to increment begin iterator when it is at end
        if (unordered_hash_it == unordered_hash_end_it) {
          assert(0);
        }
        ++unordered_hash_it;
        return;
      }
      default: assert(0);
    }
  }

  // equal
  bool equal(multimap_iterator_t<T> const& other) const {
    // it is a program error to check equality of differing map types
    if (this->map_type != other.map_type) {
      assert(0);
    }

    switch(map_type) {
      case MULTIMAP_BTREE:
        return this->btree_it == other.btree_it;
      case MULTIMAP_FLAT_SORTED_VECTOR:
        return this->flat_sorted_vector_it == other.flat_sorted_vector_it;
      case MULTIMAP_RED_BLACK_TREE:
        return this->red_black_tree_it == other.red_black_tree_it;
      case MULTIMAP_UNORDERED_HASH:
        return this->unordered_hash_it == other.unordered_hash_it;
      default: assert(0); std::exit(1);
    }
  }

  // dereference
  void dereference() {
    switch(map_type) {
      case MULTIMAP_BTREE: {
        // program error to increment begin iterator when it is at end
        if (btree_it == btree_end_it) {
          assert(0);
        }
        dereferenced_value = *btree_it;
        return;
      }
      case MULTIMAP_FLAT_SORTED_VECTOR: {
        // program error to increment begin iterator when it is at end
        if (flat_sorted_vector_it == flat_sorted_vector_end_it) {
          assert(0);
        }
        dereferenced_value = *flat_sorted_vector_it;
        return;
      }
      case MULTIMAP_RED_BLACK_TREE: {
        // program error to increment begin iterator when it is at end
        if (red_black_tree_it == red_black_tree_end_it) {
          assert(0);
        }
        dereferenced_value = *red_black_tree_it;
        return;
      }
      case MULTIMAP_UNORDERED_HASH: {
        // program error to increment begin iterator when it is at end
        if (unordered_hash_it == unordered_hash_end_it) {
          assert(0);
        }
        dereferenced_value = *unordered_hash_it;
        return;
      }
      default: assert(0);
    }
  }

  public:
  // the constructors for each map type using native map iterators
  multimap_iterator_t(btree_it_pair_t p_it, bool p_is_end) :
                      map_type(MULTIMAP_BTREE),
                      btree_it((p_is_end) ? p_it.second : p_it.first),
                      btree_end_it(p_it.second),
                      flat_sorted_vector_it(),
                      flat_sorted_vector_end_it(),
                      red_black_tree_it(),
                      red_black_tree_end_it(),
                      unordered_hash_it(),
                      unordered_hash_end_it(),
                      dereferenced_value() {
  }

  multimap_iterator_t(flat_sorted_vector_it_pair_t p_it, bool p_is_end) :
                      map_type(MULTIMAP_FLAT_SORTED_VECTOR),
                      btree_it(),
                      btree_end_it(),
                      flat_sorted_vector_it((p_is_end) ? p_it.second : p_it.first),
                      flat_sorted_vector_end_it(p_it.second),
                      red_black_tree_it(),
                      red_black_tree_end_it(),
                      unordered_hash_it(),
                      unordered_hash_end_it(),
                      dereferenced_value() {
  }

  multimap_iterator_t(red_black_tree_it_pair_t p_it, bool p_is_end) :
                      map_type(MULTIMAP_RED_BLACK_TREE),
                      btree_it(),
                      btree_end_it(),
                      flat_sorted_vector_it(),
                      flat_sorted_vector_end_it(),
                      red_black_tree_it((p_is_end) ? p_it.second : p_it.first),
                      red_black_tree_end_it(p_it.second),
                      unordered_hash_it(),
                      unordered_hash_end_it(),
                      dereferenced_value() {
  }

  multimap_iterator_t(unordered_hash_it_pair_t p_it, bool p_is_end) :
                      map_type(MULTIMAP_UNORDERED_HASH),
                      btree_it(),
                      btree_end_it(),
                      flat_sorted_vector_it(),
                      flat_sorted_vector_end_it(),
                      red_black_tree_it(),
                      red_black_tree_end_it(),
                      unordered_hash_it((p_is_end) ? p_it.second : p_it.first),
                      unordered_hash_end_it(p_it.second),
                      dereferenced_value() {
  }

  // this useless default constructor is required by std::pair
  multimap_iterator_t() :
                      map_type(MULTIMAP_BTREE),   // had to pick one
                      btree_it(),
                      btree_end_it(),
                      flat_sorted_vector_it(),
                      flat_sorted_vector_end_it(),
                      red_black_tree_it(),
                      red_black_tree_end_it(),
                      unordered_hash_it(),
                      unordered_hash_end_it(),
                      dereferenced_value() {
  }

  // copy capability is required by std::pair
  multimap_iterator_t& operator=(const multimap_iterator_t& other) {
    map_type = other.map_type;
    btree_it = other.btree_it;
    btree_end_it = other.btree_end_it;
    flat_sorted_vector_it = other.flat_sorted_vector_it;
    flat_sorted_vector_end_it = other.flat_sorted_vector_end_it;
    red_black_tree_it = other.red_black_tree_it;
    red_black_tree_end_it = other.red_black_tree_end_it;
    unordered_hash_it = other.unordered_hash_it;
    unordered_hash_end_it = other.unordered_hash_end_it;
    dereferenced_value = other.dereferenced_value; // not necessary.
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  multimap_iterator_t& operator++() {
    increment();
    return *this;
  }
  multimap_iterator_t operator++(int) {  // c++11 delete would be better.
    multimap_iterator_t temp(*this);
    increment();
    return temp;
  }
  std::pair<T, uint64_t>& operator*() {
    dereference();
    return dereferenced_value;
  }
  std::pair<T, uint64_t>* operator->() {
    dereference();
    return &dereferenced_value;
  }
  bool operator==(const multimap_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const multimap_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

