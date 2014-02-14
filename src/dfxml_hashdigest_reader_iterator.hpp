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
 * This input iterator dereferences to hashdb_element_t to provide
 * iterator access to hash entries in a DFXML file.
 *
 * It is currently not efficient: it uses dfxml_hashdigest_reader
 * to fill a vector of hashdb elements, then offers these elements
 * through an input iterator.
 *
 * It would be nice to change dfxml_hashdigest_reader to iterate, instead.
 * But the interface presented is clean.
 */

#ifndef DFXML_HASHDIGEST_READER_ITERATOR_HPP
#define DFXML_HASHDIGEST_READER_ITERATOR_HPP
#include "hashdb_element.hpp"
#include "dfxml_hashdigest_reader.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
//#include <algorithm>
#include <vector>

class dfxml_hashdigest_reader_iterator_t {

  private:

  // during initialization, this reader uses this consumer with
  // dfxml_hashdigest_reader's static do_read() function.
  class reader_consumer_t {
    std::vector<hashdb_element_t>* elements;

    reader_consumer_t(vector<hashdb_elements_t>* p_elements) {
    }

    void consume(const hashdb_element_t& hashdb_element) {
      elements->push_back(hashdb_element);
    }
  };

  const std::string dfxml_filename;
  const std::string default_repository_name;
  std::vector<hashdb_element_t> elements;
  std::vector<hashdb_element_t>::const_iterator elements_iterator;
  hashdb_element_t dereferenced_hashdb_element;
  bool dereference_is_cached;

  // elemental iterator accessors
  // increment
  void increment() {
    dereference_is_valid = false;
    ++elements_iterator;
  }

  // equal
  bool equal(dfxml_hashdigest_reader_iterator_t const& other) const {
    return this->elements_iterator == other.elements_iterator;
  }

  // dereference
  void dereference() {
    dereferenced_hashdb_element = *elements_iterator;
  }

  public:
  dfxml_hashdigest_reader_iterator_t(std::string p_dfxml_filename,
                                     std::string p_default_repository_name) :
           dfxml_filename(p_dfxml_filename),
           default_repository_name(p_default_repository_name),
           elements(),
           elements_iterator(),
           dereferenced_hashdb_element(),
           dereference_is_cached(false) {

    // reader consumer with handle to elements array
    reader_consumer_t consumer(&elements);
    // read all of DFXML into the elements vector
    dfxml_hashdigest_reader_t<reader_consumer_t>::do_read(
                 dfxml_filename,
                 default_repository_name,
                 consumer);

    elements_iterator = elements.begin();
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  dfxml_hashdigest_reader_iterator_t& operator++() {
    increment();
    return *this;
  }
  dfxml_hashdigest_reader_iterator_t operator++(int) {  // c++11 delete would be better.
    dfxml_hashdigest_reader_iterator_t temp(*this);
    increment();
    return temp;
  }
  hashdb_element_t& operator*() {
    dereference();
    return dereferenced_hashdb_element;
  }
  hashdb_element_t* operator->() {
    dereference();
    return &dereferenced_hashdb_element;
  }
  bool operator==(const dfxml_hashdigest_reader_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const dfxml_hashdigest_reader_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

