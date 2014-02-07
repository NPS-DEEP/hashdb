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
 * Provides map_multimap iterator, encapsulating map and multimap.
 */

#ifndef MAP_MULTIMAP_ITERATOR_HPP
#define MAP_MULTIMAP_ITERATOR_HPP
#include "map_manager.hpp"
#include "map_iterator.hpp"
#include "multimap_manager.hpp"
#include "multimap_iterator.hpp"
#include "source_lookup_encoding.hpp" // to obtain count from payload

template<class T>
class map_multimap_iterator_t {
  private:

  // multimap manager
  map_manager_t<T>* map_manager;
  multimap_manager_t<T>* multimap_manager;

  // internal state
  map_iterator_t<T> map_iterator;
  multimap_iterator_t<T> multimap_iterator;
  multimap_iterator_t<T> multimap_end_iterator;
  bool in_multimap_iterator;

  // the dereferenced value, specifically, std::pair<T, uint64_t>
  std::pair<T, uint64_t> dereferenced_value;

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {

    if (in_multimap_iterator) {
      // increment the multimap iterator
      ++multimap_iterator;
      if (multimap_iterator == multimap_end_iterator) {
        // that's it for multimap iterator, go back to the map iterator
        ++map_iterator;
        set_multimap_iterator_state();
      }
      return;
    } else {
      // increment the map iterator
      ++map_iterator;
      set_multimap_iterator_state();
      return;
    }
  }

  // equal
  bool equal(map_multimap_iterator_t<T> const& other) const {

    if (map_manager != other.map_manager ||
        multimap_manager != other.multimap_manager) {
      // program error, wrong manager
      assert(0);
    }

    // do not compare multimap iterator unless it is in use
    // because it may not be defined
    if (map_iterator == other.map_iterator &&
        (!in_multimap_iterator ||
         (multimap_iterator == other.multimap_iterator))) {
      return true;
    } else {
      return false;
    }
  }

  // dereference
  void dereference() {
    if (in_multimap_iterator) {
      // dereference the multimap iterator
      dereferenced_value = *multimap_iterator;
    } else {
      // dereference the map iterator
      dereferenced_value = *map_iterator;
    }
  }

  // call this after changing the map iterator
  void set_multimap_iterator_state() {
    if (map_iterator == map_manager->end()) {
      // done
      in_multimap_iterator = false;

    } else {
      // see if this map entry uses the multimap
      uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
      if (count == 1) {
        // use map
        in_multimap_iterator = false;
      } else {
        // use multimap
        in_multimap_iterator = true;
        std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >
                      multimap_range =
                      multimap_manager->equal_range(map_iterator->first);
        multimap_iterator = multimap_range.first;
        multimap_end_iterator = multimap_range.second;

        // by design, count >= 2 elements, but it is computationally cheap
        // to make sure count > 0
        if (multimap_iterator == multimap_end_iterator) {
          assert(0);
        }

      }
    }
  }

  public:
  // the constructors for each map type using native map iterators
  map_multimap_iterator_t(map_manager_t<T>* p_map_manager,
                    multimap_manager_t<T>* p_multimap_manager,
                    bool p_is_end) :
                             map_manager(p_map_manager),
                             multimap_manager(p_multimap_manager),
                             map_iterator(),
                             multimap_iterator(),
                             multimap_end_iterator(),
                             in_multimap_iterator(false),
                             dereferenced_value() {
    if (p_is_end) {
      // set up with end iterator
      map_iterator = map_manager->end();
    } else {
      // set up with begin iterator
      map_iterator = map_manager->begin();
    }
    // set state for the multimap iterator
    set_multimap_iterator_state();
  }

  // zz this is bad because it can live longer than the map zzzzzzzzzzzzzz
  map_multimap_iterator_t(const map_multimap_iterator_t& other) :
                             map_manager(other.map_manager),
                             multimap_manager(other.multimap_manager),
                             map_iterator(other.map_iterator),
                             multimap_iterator(other.multimap_iterator),
                             multimap_end_iterator(other.multimap_end_iterator),
                             in_multimap_iterator(other.in_multimap_iterator),
                             dereferenced_value(other.dereferenced_value) {
  }

  // this useless default constructor is required by std::pair
  map_multimap_iterator_t() :
                             map_manager(0),
                             multimap_manager(0),
                             map_iterator(),
                             multimap_iterator(),
                             multimap_end_iterator(),
                             in_multimap_iterator(false),
                             dereferenced_value() {
  }

  // copy capability is required by std::pair
  map_multimap_iterator_t& operator=(const map_multimap_iterator_t& other) {
    map_manager = other.map_manager;
    multimap_manager = other.multimap_manager;
    map_iterator = other.map_iterator;
    multimap_iterator = other.multimap_iterator;
    multimap_end_iterator = other.multimap_end_iterator;
    in_multimap_iterator = other.in_multimap_iterator;
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  map_multimap_iterator_t& operator++() {
    increment();
    return *this;
  }
  map_multimap_iterator_t operator++(int) {  // c++11 delete would be better.
    map_multimap_iterator_t temp(*this);
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
  bool operator==(const map_multimap_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const map_multimap_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

