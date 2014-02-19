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
 * Provides map_singles iterator for iterating over elements in map
 * that have count=1.
 */

#ifndef MAP_SINGLES_ITERATOR_HPP
#define MAP_SINGLES_ITERATOR_HPP
#include "map_manager.hpp"
#include "map_iterator.hpp"
#include "source_lookup_encoding.hpp" // to obtain count from payload

template<class T>
class map_singles_iterator_t {
  private:

  // map manager
  map_manager_t<T>* map_manager;

  // internal state
  map_iterator_t<T> map_iterator;

  // the dereferenced value, specifically, std::pair<T, uint64_t>
  std::pair<T, uint64_t> dereferenced_value;

  // use this helper to get onto a single element
  void walk_to_single() {
    // increment the iterator until it is on a single
    while (map_iterator != map_manager->end()
           && source_lookup_encoding::get_count(map_iterator->second) != 1) {
      ++map_iterator;
    }
  }

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    ++map_iterator;
    walk_to_single();
  }

  // equal
  bool equal(map_singles_iterator_t<T> const& other) const {

    if (map_manager != other.map_manager) {
      // program error, wrong map manager
      assert(0);
    }

    if (map_iterator == other.map_iterator) {
      return true;
    } else {
      return false;
    }
  }

  // dereference
  void dereference() {
    // cache the dereferenced map iterator
    dereferenced_value = *map_iterator;
  }

  public:
  // the constructors for each map type using native map iterators
  map_singles_iterator_t(map_manager_t<T>* p_map_manager, bool p_is_end) :
                             map_manager(p_map_manager),
                             map_iterator(),
                             dereferenced_value() {
    if (p_is_end) {
      // set up with end iterator
      map_iterator = map_manager->end();
    } else {
      // set up with begin iterator
      map_iterator = map_manager->begin();
      walk_to_single();
    }
  }

  // zz this is bad because it can live longer than the map zzzzzzzzzzzzzz
  map_singles_iterator_t(const map_singles_iterator_t& other) :
                             map_manager(other.map_manager),
                             map_iterator(other.map_iterator),
                             dereferenced_value(other.dereferenced_value) {
  }

  // this useless default constructor is required by std::pair
  map_singles_iterator_t() :
                             map_manager(0),
                             map_iterator(),
                             dereferenced_value() {
  }

  // copy capability is required by std::pair
  map_singles_iterator_t& operator=(const map_singles_iterator_t& other) {
    map_manager = other.map_manager;
    map_iterator = other.map_iterator;
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  map_singles_iterator_t& operator++() {
    increment();
    return *this;
  }
  map_singles_iterator_t operator++(int) {  // c++11 delete would be better.
    map_singles_iterator_t temp(*this);
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
  bool operator==(const map_singles_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const map_singles_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

