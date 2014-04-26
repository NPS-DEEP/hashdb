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
 * Provides iterator to array<pair<string forensic_path, string hexdigest>>
 * for an identified blocks reader.
 */

#ifndef IDENTIFIED_BLOCKS_READER_ITERATOR_HPP
#define IDENTIFIED_BLOCKS_READER_ITERATOR_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
//#include <algorithm>
//#include <vector>
#include <errno.h>

class identified_blocks_reader_iterator_t {

  std::fstream* in;

  std::pair<std::string, std::string> dereferenced_value;
  size_t feature_count;
  bool at_end;

  // read feature or set at_end=true
  void read_feature() {

    // parse the next available line
    std::string line;
    while(getline(*in, line)) {

      // skip comment lines
      if (line[0] == '#') {
        continue;
      }

      // parse next feature

      // find valid hash feature line
      size_t tab_index1 = line.find('\t');
      if (tab_index1 == std::string::npos) {
        continue;
      }
      size_t tab_index2 = line.find('\t', tab_index1 + 1);
      if (tab_index2 == std::string::npos) {
        continue;
      }

      // get the feature
      dereferenced_value = std::pair<std::string, std::string>
                      (line.substr(0, tab_index1),
                      line.substr(tab_index1+1, tab_index2 - tab_index1 - 1));
      ++feature_count;
//std::cout << "ibri.v1 '" << dereferenced_value.first << "'\n";
//std::cout << "ibri.v2 '" << dereferenced_value.second << "'\n";
      return;
    }

    // at eof
    at_end = true;
  }

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    read_feature();
  }

  // equal
  bool equal(identified_blocks_reader_iterator_t const& other) const {
    // only equal if same reader and both at end or both at same count
    if (this->at_end && other.at_end && (this->in == other.in)) {
      return true;
    }
    if (this->feature_count == other.feature_count) {
      return true;
    }
    return false;
  }

  // dereference
  void dereference() {
    if (at_end) {
      std::cout << "identified_blocks_reader_iterator invalid dereference\n";
// compiler warning      throw std::runtime_error("dereferenced when at end");
    }
  }

  public:
  identified_blocks_reader_iterator_t(std::fstream* p_in, bool p_at_end) :
             in(p_in),
             dereferenced_value(),
             feature_count(0),
             at_end(p_at_end) {
    if (!at_end) {
      read_feature();
    }
  }

  // this is bad because it can live longer than &in
  identified_blocks_reader_iterator_t(const identified_blocks_reader_iterator_t& other) :
                    in(other.in),
                    dereferenced_value(other.dereferenced_value),
                    feature_count(other.feature_count),
                    at_end(other.at_end) {
  }

  // this is bad because it can live longer than &in
  identified_blocks_reader_iterator_t& operator=(const identified_blocks_reader_iterator_t& other) {
    in = other.in;
    dereferenced_value = other.dereferenced_value;
    feature_count = other.feature_count;
    at_end = other.at_end;
    return *this;
  }

  // this useless default constructor is required by std::pair
  identified_blocks_reader_iterator_t() :
                    in(),
                    dereferenced_value(),
                    feature_count(0),
                    at_end(true) {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  identified_blocks_reader_iterator_t& operator++() {
    increment();
    return *this;
  }
  identified_blocks_reader_iterator_t operator++(int) {
    identified_blocks_reader_iterator_t temp(*this);
    increment();
    return temp;
  }
  std::pair<std::string, std::string>& operator*() {
    dereference();
    return dereferenced_value;
  }
  std::pair<std::string, std::string>* operator->() {
    dereference();
    return &dereferenced_value;
  }
  bool operator==(const identified_blocks_reader_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const identified_blocks_reader_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

