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
 * Provides a hashdb map only iterator which wraps map_iterator_t<T>.
 * Dereferences to pair<hexdigest_string, uint32_t count>.
 */

#ifndef HASHDB_MAP_ONLY_ITERATOR_HPP
#define HASHDB_MAP_ONLY_ITERATOR_HPP
#include "map_iterator.hpp"
#include "dfxml/src/hash_t.h"
#include "hashdigest_types.h"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "source_lookup_encoding.hpp" // to get count

class hashdb_map_only_iterator_t {
  private:
  // the hashdigest type that this iterator will be using
  hashdigest_type_t hashdigest_type;

  // the hashdigest-specific iterators, one of which will be used,
  // based on how hashdb_map_only_iterator_t is instantiated
  map_iterator_t<md5_t> md5_iterator;
  map_iterator_t<sha1_t> sha1_iterator;
  map_iterator_t<sha256_t> sha256_iterator;

//  // the type this iterator dereferences to
//  typedef std::pair<hashdigest_t, uint32_t> map_only_pair_t;

  // the dereferenced value
  std::pair<hashdigest_t, uint32_t> dereferenced_value;

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    switch(hashdigest_type) {
      case HASHDIGEST_MD5:     ++md5_iterator; return;
      case HASHDIGEST_SHA1:    ++sha1_iterator; return;
      case HASHDIGEST_SHA256:  ++sha256_iterator; return;
      default: assert(0);
    }
  }

  // equal
  bool equal(hashdb_map_only_iterator_t const& other) const {
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
        dereferenced_value = std::pair<hashdigest_t, uint32_t>(
               hashdigest_t(md5_iterator->first),
               source_lookup_encoding::get_count(md5_iterator->second));
        break;
      case HASHDIGEST_SHA1:
        dereferenced_value = std::pair<hashdigest_t, uint32_t>(
               hashdigest_t(sha1_iterator->first),
               source_lookup_encoding::get_count(sha1_iterator->second));
        break;
      case HASHDIGEST_SHA256:
        dereferenced_value = std::pair<hashdigest_t, uint32_t>(
               hashdigest_t(sha256_iterator->first),
               source_lookup_encoding::get_count(sha256_iterator->second));
        break;
      default: assert(0);
    }
  }

  public:
  // the constructors for each map type available
  hashdb_map_only_iterator_t(map_iterator_t<md5_t> p_map_iterator) :
                             hashdigest_type(HASHDIGEST_MD5),
                             md5_iterator(p_map_iterator),
                             sha1_iterator(),
                             sha256_iterator(),
                             dereferenced_value() {
  }
  hashdb_map_only_iterator_t(map_iterator_t<sha1_t> p_map_iterator) :
                             hashdigest_type(HASHDIGEST_SHA1),
                             md5_iterator(),
                             sha1_iterator(p_map_iterator),
                             sha256_iterator(),
                             dereferenced_value() {
  }
  hashdb_map_only_iterator_t(map_iterator_t<sha256_t> p_map_iterator) :
                             hashdigest_type(HASHDIGEST_SHA256),
                             md5_iterator(),
                             sha1_iterator(),
                             sha256_iterator(p_map_iterator),
                             dereferenced_value() {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  hashdb_map_only_iterator_t& operator++() {
    increment();
    return *this;
  }
  hashdb_map_only_iterator_t operator++(int) {  // c++11 delete would be better.
    hashdb_map_only_iterator_t temp(*this);
    increment();
    return temp;
  }
  std::pair<hashdigest_t, uint32_t>& operator*() {
    dereference();
    return dereferenced_value;
  }
  std::pair<hashdigest_t, uint32_t>* operator->() {
    dereference();
    return &dereferenced_value;
  }
  bool operator==(const hashdb_map_only_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const hashdb_map_only_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

