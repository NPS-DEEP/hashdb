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
 * Provides a hashdb iterator which wraps map_multimap_iterator_t<T>.
 * Dereferences through pair<key, source_lookup_encoding>
 * into hashdb_element using help from hashdb_element_lookup_t.
 */

#ifndef HASHDB_ITERATOR_HPP
#define HASHDB_ITERATOR_HPP
#include "map_multimap_iterator.hpp"
#include "hashdb_element.hpp"
#include "hashdb_element_lookup.hpp"
#include "dfxml/src/hash_t.h"

template<typename T>
class hashdb_iterator_t {
  private:

  // external resource required for creating a hashdb_element
//  const hashdb_element_lookup_t<T> hashdb_element_lookup;
  hashdb_element_lookup_t<T> hashdb_element_lookup;

  // the underlying map_multimap_iterator
  map_multimap_iterator_t<T> map_multimap_iterator;

  // the dereferenced value
  bool dereferenced_value_is_cached;
  hashdb_element_t<T> dereferenced_value;

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    dereferenced_value_is_cached = false;
    ++map_multimap_iterator;
  }

  // equal
  bool equal(hashdb_iterator_t const& other) const {
    // it is a program error if external resources differ
    if (!(this->hashdb_element_lookup == other.hashdb_element_lookup)) {
      assert(0);
    }

    return this->map_multimap_iterator == other.map_multimap_iterator;
  }

  // dereference
  void dereference() {
    if (dereferenced_value_is_cached) {
      // already valid
      return;
    }

    std::pair<T, uint64_t> hashdb_pair = *map_multimap_iterator;

    dereferenced_value = hashdb_element_lookup(hashdb_pair);
    dereferenced_value_is_cached = true;
  }

  public:
  // the constructor for the map_multimap type using the map_multimap iterators
  hashdb_iterator_t(map_multimap_iterator_t<T> p_map_multimap_iterator,
                    const hashdb_element_lookup_t<T> p_hashdb_element_lookup) :
               hashdb_element_lookup(p_hashdb_element_lookup),
               map_multimap_iterator(p_map_multimap_iterator),
               dereferenced_value_is_cached(false),
               dereferenced_value() {
  }

  // this useless default constructor is required by std::pair
  hashdb_iterator_t() :
                      hashdb_element_lookup(),
                      map_multimap_iterator(),
                      dereferenced_value_is_cached(false),
                      dereferenced_value() {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  hashdb_iterator_t& operator++() {
    increment();
    return *this;
  }
  hashdb_iterator_t operator++(int) {  // c++11 delete would be better.
    hashdb_iterator_t temp(*this);
    increment();
    return temp;
  }
  hashdb_element_t<T>& operator*() {
    dereference();
    return dereferenced_value;
  }
  hashdb_element_t<T>* operator->() {
    dereference();
    return &dereferenced_value;
  }
  bool operator==(const hashdb_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const hashdb_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

