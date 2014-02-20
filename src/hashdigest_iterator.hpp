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
 * Provides a hashdigest iterator which wraps map_multimap_iterator_t<T>.
 * Dereferences to pair<hashdigest_string, uint64_t>.
 */

#ifndef HASHDIGEST_ITERATOR_HPP
#define HASHDIGEST_ITERATOR_HPP
#include "map_manager.hpp"
#include "map_iterator.hpp"
//#include "dfxml/src/hash_t.h"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "hashdigest_types.h"

class hashdigest_iterator_t {
  private:

  // the hashdigest type that this iterator will be using
  hashdigest_type_t hashdigest_type;

  // one of these iterators will be used
  map_iterator_t<md5_t> md5_iterator;
  map_iterator_t<sha1_t> sha1_iterator;
  map_iterator_t<sha256_t> sha256_iterator;

  // the dereferenced value
  bool dereferenced_value_is_cached;
  std::pair<hashdigest_t, uint64_t> dereferenced_value;

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
  bool equal(hashdigest_iterator_t const& other) const {

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
    switch(hashdigest_type) {
      case HASHDIGEST_MD5:
        dereferenced_value = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(md5_iterator->first), md5_iterator->second);
//               source_lookup_encoding::get_count(md5_iterator->second)
        break;
      case HASHDIGEST_SHA1:
        dereferenced_value = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(sha1_iterator->first), sha1_iterator->second);
        break;
      case HASHDIGEST_SHA256:
        dereferenced_value = std::pair<hashdigest_t, uint64_t>(
               hashdigest_t(sha256_iterator->first), sha256_iterator->second);
        break;
      default: assert(0);
    }
    dereferenced_value_is_cached = true;
  }

  public:
  // the constructors for each map type
  hashdigest_iterator_t(map_iterator_t<md5_t> map_iterator) :
                   hashdigest_type(HASHDIGEST_MD5),
                   md5_iterator(map_iterator),
                   sha1_iterator(),
                   sha256_iterator(),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }
  hashdigest_iterator_t(map_iterator_t<sha1_t> map_iterator) :
                   hashdigest_type(HASHDIGEST_SHA1),
                   md5_iterator(),
                   sha1_iterator(map_iterator),
                   sha256_iterator(),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }
  hashdigest_iterator_t(map_iterator_t<sha256_t> map_iterator) :
                   hashdigest_type(HASHDIGEST_SHA256),
                   md5_iterator(),
                   sha1_iterator(),
                   sha256_iterator(map_iterator),
                   dereferenced_value_is_cached(false),
                   dereferenced_value() {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  hashdigest_iterator_t& operator++() {
    increment();
    return *this;
  }
  hashdigest_iterator_t operator++(int) {  // c++11 delete would be better.
    hashdigest_iterator_t temp(*this);
    increment();
    return temp;
  }
  std::pair<hashdigest_t, uint64_t>& operator*() {
    dereference();
    return dereferenced_value;
  }
  std::pair<hashdigest_t, uint64_t>* operator->() {
    dereference();
    return &dereferenced_value;
  }
  bool operator==(const hashdigest_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const hashdigest_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

