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
 * Dereferences through pair<hexdigest_string, source_lookup_encoding>
 * into hashdb_element using help from hashdb_element_lookup_t.
 */

#ifndef HASHDB_ITERATOR_HPP
#define HASHDB_ITERATOR_HPP
#include "map_multimap_iterator.hpp"
#include "hashdb_element.hpp"
#include "hashdb_element_lookup.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "hashdigest_types.h"

class hashdb_iterator_t {
  private:

  // the hashdigest type that this iterator will be using
  hashdigest_type_t hashdigest_type;

  // external resource required for creating a hashdb_element
  const hashdb_element_lookup_t hashdb_element_lookup;

  // the hashdigest-specific iterators, one of which will be used,
  // based on how hashdb_iterator_t is instantiated
  map_multimap_iterator_t<md5_t> md5_iterator;
  map_multimap_iterator_t<sha1_t> sha1_iterator;
  map_multimap_iterator_t<sha256_t> sha256_iterator;

  // the dereferenced value
  bool dereferenced_value_is_cached;
  hashdb_element_t dereferenced_value;

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    dereferenced_value_is_cached = false;
    switch(hashdigest_type) {
      case HASHDIGEST_MD5:     ++md5_iterator; return;
      case HASHDIGEST_SHA1:    ++sha1_iterator; return;
      case HASHDIGEST_SHA256:  ++sha256_iterator; return;
      default: assert(0);
    }
  }

  // equal
  bool equal(hashdb_iterator_t const& other) const {
    // it is a program error if external resources differ
    if (!(this->hashdb_element_lookup == other.hashdb_element_lookup)) {
      assert(0);
    }

    // it is a program error if hashdigest types differ
    if (this->hashdigest_type != other.hashdigest_type) {
      assert(0);
    }

    switch(hashdigest_type) {
      case HASHDIGEST_MD5:
        return this->md5_iterator == other.md5_iterator;
      case HASHDIGEST_SHA1:
        return this->sha1_iterator == other.sha1_iterator;
      case HASHDIGEST_SHA256:
        return this->sha256_iterator == other.sha256_iterator;
      default: assert(0);
    }
  }

  // dereference
  void dereference() {
    std::pair<hashdigest_t, uint64_t> hashdb_pair;
    switch(hashdigest_type) {
      case HASHDIGEST_MD5:
        hashdb_pair = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(md5_iterator->first), md5_iterator->second);
        break;
      case HASHDIGEST_SHA1:
        hashdb_pair = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(sha1_iterator->first), sha1_iterator->second);
        break;
      case HASHDIGEST_SHA256:
        hashdb_pair = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(sha256_iterator->first), sha256_iterator->second);
        break;
      default: assert(0);
    }

    dereferenced_value = hashdb_element_lookup.do_lookup(hashdb_pair);
    dereferenced_value_is_cached = true;
  }

  public:
  // the constructors for each map_multimap type using native iterators
  hashdb_iterator_t(map_multimap_iterator_t<md5_t> map_multimap_iterator,
           const hashdb_element_lookup_t& p_hashdb_element_lookup) :
                   hashdigest_type(HASHDIGEST_MD5),
                   hashdb_element_lookup(p_hashdb_element_lookup),
                   md5_iterator(map_multimap_iterator),
                   sha1_iterator(),
                   sha256_iterator(),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }
  hashdb_iterator_t(map_multimap_iterator_t<sha1_t> map_multimap_iterator,
           const hashdb_element_lookup_t& p_hashdb_element_lookup) :
                   hashdigest_type(HASHDIGEST_SHA1),
                   hashdb_element_lookup(p_hashdb_element_lookup),
                   md5_iterator(),
                   sha1_iterator(map_multimap_iterator),
                   sha256_iterator(),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }
  hashdb_iterator_t(map_multimap_iterator_t<sha256_t> map_multimap_iterator,
           const hashdb_element_lookup_t& p_hashdb_element_lookup) :
                   hashdigest_type(HASHDIGEST_SHA256),
                   hashdb_element_lookup(p_hashdb_element_lookup),
                   md5_iterator(),
                   sha1_iterator(),
                   sha256_iterator(map_multimap_iterator),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }

  // this useless default constructor is required by std::pair
  hashdb_iterator_t() :
                      hashdigest_type(HASHDIGEST_UNDEFINED),
                      hashdb_element_lookup(),
                      md5_iterator(),
                      sha1_iterator(),
                      sha256_iterator(),
                      dereferenced_value_is_cached(false),
                      dereferenced_value() {
  }

/*
  // Beware that I am not properly managing pointers here.
  hashdb_iterator_t(const hashdb_iterator_t& other) :
               hashdigest_type(other.hashdigest_type),
               hashdb_element_lookup(other.hashdb_element_lookup),
               md5_iterator(other.md5_iterator),
               sha1_iterator(other.sha1_iterator),
               sha256_iterator(other.sha256_iterator),
               dereferenced_value_is_cached(false),
               dereferenced_value() {
  }
*/

/*
  // copy capability is required by std::pair
  // Beware that I am not properly managing pointers here.
  hashdb_iterator_t& operator=(const hashdb_iterator_t& other) {
    hashdigest_type = other.hashdigest_type;
    hashdb_element_lookup = other.hashdb_element_lookup;
    md5_iterator = other.md5_iterator;
    sha1_iterator = other.sha1_iterator;
    sha256_iterator = other.sha256_iterator;
    dereferenced_value_is_cached = false;
    dereferenced_value = hashdb_element_t();
    return *this;
  }
*/

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
  hashdb_element_t& operator*() {
    dereference();
    return dereferenced_value;
  }
  hashdb_element_t* operator->() {
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

