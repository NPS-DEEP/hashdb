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
 * Provides iterator to identified_blocks_feature_t for an identified blocks reader.
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
#include "identified_blocks_feature.hpp"

class identified_blocks_reader_iterator_t {

  std::fstream* in;

  identified_blocks_feature_t cached_feature; // cached dereferenced value
  size_t line_count;
  bool at_end;

  // read feature or set at_end=true
  void read_feature() {

    // parse the next available line
    std::string line;
    while(getline(*in, line)) {
      ++line_count;

      // skip comment lines
      if (line[0] == '#') {
        // not valid
        continue;
      }

      // parse line of "offset tab hexdigest tab count"

      // find tabs
      size_t tab_index1 = line.find('\t');
      if (tab_index1 == std::string::npos) {
        continue;
      }
      size_t tab_index2 = line.find('\t', tab_index1 + 1);
      if (tab_index2 == std::string::npos) {
        continue;
      }

      // offset string
      std::string offset_string = line.substr(0, tab_index1);

      // key
      std::string hash_string = line.substr(tab_index1+1, tab_index2 - tab_index1 - 1);

      // validate the hash string
#ifdef USE_HASH_TYPE_STRAIGHT64
      // data is used directly as the hash
      if (hash_string.size() != hash_t::size())
#else
      // a hash value is calculated from the hash string
      if (hash_string.size() != hash_t::size()*2)
#endif
      {
        std::cerr << "Hash string '" << hash_string
                  << "' length " << hash_string.size()
                  << " is invalid for " << digest_name<hash_t>()
                  << ".\n";
        continue;
      }
      hash_t key = hash_t::fromhex(hash_string);

      // count
      // If there is a space after the count, truncate it there
      std::string count_string = line.substr(tab_index2+1);
      size_t space_index = count_string.find(' ');
      if (space_index != std::string::npos) {
          count_string.erase(space_index,std::string::npos);
      }

      uint32_t count;
      try {
        count = boost::lexical_cast<uint32_t>(count_string);
      } catch (...) {
        continue;
      }

      // got here, so line is valid
      cached_feature = identified_blocks_feature_t(offset_string, key, count);

      return;
    }

    // at eof
    at_end = true;
  }

  // increment
  void increment() {
    read_feature();
  }

  // equal
  bool equal(identified_blocks_reader_iterator_t const& other) const {
    // not equal if files are different
    if (this->in != other.in) {
      return false;
    }

    // only equal if same reader and both at end or both at same line
    if (this->at_end && other.at_end && (this->in == other.in)) {
      return true;
    }
    if (this->line_count == other.line_count) {
      return true;
    }
    return false;
  }

  public:
  identified_blocks_reader_iterator_t(std::fstream* p_in, bool p_at_end) :
             in(p_in),
             cached_feature(),
             line_count(0),
             at_end(p_at_end) {
    if (!at_end) {
      read_feature();
    }
  }

  // this is bad because it can live longer than &in
  identified_blocks_reader_iterator_t(const identified_blocks_reader_iterator_t& other) :
                    in(other.in),
                    cached_feature(other.cached_feature),
                    line_count(other.line_count),
                    at_end(other.at_end) {
  }

  // this is bad because it can live longer than &in
  identified_blocks_reader_iterator_t& operator=(const identified_blocks_reader_iterator_t& other) {
    in = other.in;
    cached_feature = other.cached_feature;
    line_count = other.line_count;
    at_end = other.at_end;
    return *this;
  }

  // this useless default constructor is required by std::pair
  identified_blocks_reader_iterator_t() :
                    in(),
                    cached_feature(),
                    line_count(0),
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
  identified_blocks_feature_t& operator*() {
    if (at_end) assert(0);
    return cached_feature;
  }
  identified_blocks_feature_t* operator->() {
    if (at_end) assert(0);
    return &cached_feature;
  }
  bool operator==(const identified_blocks_reader_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const identified_blocks_reader_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

