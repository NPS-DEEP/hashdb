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
 * Provides a hashdb iterator which dereferences into hashdb_element_t.
 */

#ifndef HASHDB_ITERATOR_HPP
#define HASHDB_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"

template<typename T>
class hashdb_iterator_t {
  private:

  const source_lookup_index_manager_t* source_lookup_index_manager;
  uint32_t hash_block_size;

  // the underlying multimap iterator
  typedef boost::btree::btree_multimap<T, uint64_t> multimap_t;
  typedef typename multimap_t::const_iterator multimap_iterator_t;
  multimap_iterator_t multimap_iterator;

  // the cached "dereferenced" hashdb_element
  hashdb_element_t<T> hashdb_element;

  // get hashdb_element
  hashdb_element_t<T> get_hashdb_element() {
    std::pair<std::string, std::string> source_strings =
                          source_lookup_index_manager->find(
                 source_lookup_encoding::get_source_lookup_index(
                                  multimap_iterator->second));
    return hashdb_element_t<T>(
                 multimap_iterator->first,
                 hash_block_size,
                 source_strings.first,
                 source_strings.second,
                 source_lookup_encoding::get_file_offset(
                                  multimap_iterator->second));
  }

  public:
  // the constructor for the hashdb_iterator
  hashdb_iterator_t(
           const source_lookup_index_manager_t* p_source_lookup_index_manager,
           uint32_t p_hash_block_size,
           multimap_iterator_t p_multimap_iterator) :
               source_lookup_index_manager(p_source_lookup_index_manager),
               hash_block_size(p_hash_block_size),
               multimap_iterator(p_multimap_iterator),
               hashdb_element() {
  }

  // this useless default constructor is required by std::pair
  hashdb_iterator_t() :
                      source_lookup_index_manager(),
                      hash_block_size(0),
                      multimap_iterator(),
                      hashdb_element() {
  }

  // override the copy constructor and the assignment operator to quiet the
  // compiler about using a pointer data member
  hashdb_iterator_t(const hashdb_iterator_t& other) :
               source_lookup_index_manager(other.source_lookup_index_manager),
               hash_block_size(other.hash_block_size),
               multimap_iterator(other.multimap_iterator),
               hashdb_element(other.hashdb_element) {
  }
  hashdb_iterator_t& operator=(const hashdb_iterator_t& other) {
    source_lookup_index_manager = other.source_lookup_index_manager;
    hash_block_size = other.hash_block_size;
    multimap_iterator = other.multimap_iterator;
    hashdb_element = other.hashdb_element;
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  hashdb_iterator_t& operator++() {
    ++multimap_iterator;
    return *this;
  }
  hashdb_iterator_t operator++(int) {  // c++11 delete would be better.
    hashdb_iterator_t temp(*this);
    ++multimap_iterator;
    return temp;
  }
  hashdb_element_t<T>& operator*() {
    hashdb_element = get_hashdb_element();
    return hashdb_element;
  }
  hashdb_element_t<T>* operator->() {
    hashdb_element = get_hashdb_element();
    return &hashdb_element;
  }
  bool operator==(const hashdb_iterator_t& other) const {
    return this->multimap_iterator == other.multimap_iterator;
  }
  bool operator!=(const hashdb_iterator_t& other) const {
    return !(this->multimap_iterator == other.multimap_iterator);
  }
};

#endif

