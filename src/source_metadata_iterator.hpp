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
 * Provides an iterator which dereferences into source_metadata_t.
 */

#ifndef SOURCE_METADATA_ITERATOR_HPP
#define SOURCE_METADATA_ITERATOR_HPP
#include "boost/btree/index_helpers.hpp"
#include "boost/btree/btree_index_set.hpp"
#include "hash_t_selector.h"

class source_metadata_iterator_t {
  private:

  // btrees to manage multi-index lookups
  typedef typename boost::btree::btree_index_set<source_metadata_t> idx1_btree_t;
  typedef typename boost::btree::btree_index_set<source_metadata_t, default_traits, hash_ordering> idx2_btree_t;
  typedef typename boost::btree::btree_index_set<source_metadata_t, default_traits, file_size_ordering> idx3_btree_t;

  enum btree_iterator_type_t {IDX_NONE_BTREE, IDX1_BTREE, IDX2_BTREE, IDX3_BTREE};

  // state
  btree_iterator_type_t type;
  idx1_btree_t::const_iterator idx1_it;
  idx2_btree_t::const_iterator idx2_it;
  idx3_btree_t::const_iterator idx3_it;

  // the cached "dereferenced" source metadata
  source_metadata_t source_metadata;

  public:
  // the constructor for the source_metadata_iterator
  source_metadata_iterator_t(idx1_btree_t::const_iterator p_idx1_it) :
                  type(IDX1_BTREE),
                  idx1_it(p_idx1_it), idx2_it(), idx3_it(),
                  source_metadata() {
  }

  source_metadata_iterator_t(idx2_btree_t::const_iterator p_idx2_it) :
                  type(IDX2_BTREE),
                  idx1_it(), idx2_it(p_idx2_it), idx3_it(),
                  source_metadata() {
  }

  source_metadata_iterator_t(idx3_btree_t::const_iterator p_idx3_it) :
                  type(IDX3_BTREE),
                  idx1_it(), idx2_it(), idx3_it(p_idx3_it),
                  source_metadata() {
  }

  // this useless default constructor is required by std::pair
  source_metadata_iterator_t() :
                  type(IDX_NONE_BTREE),
                  idx1_it(), idx2_it(), idx3_it(),
                  source_metadata() {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  source_metadata_iterator_t& operator++() {
    switch(type) {
      case IDX1_BTREE: ++idx1_it; return *this;
      case IDX2_BTREE: ++idx2_it; return *this;
      case IDX3_BTREE: ++idx3_it; return *this;
      default: assert(0); std::exit(1);
    }
  }

  source_metadata_iterator_t operator++(int) {  // c++11 delete would be better.
    source_metadata_iterator_t temp(*this);
    switch(type) {
      case IDX1_BTREE: ++idx1_it; return temp;
      case IDX2_BTREE: ++idx2_it; return temp;
      case IDX3_BTREE: ++idx3_it; return temp;
      default: assert(0); std::exit(1);
    }
  }

  source_metadata_t& operator*() {
    switch(type) {
      case IDX1_BTREE: source_metadata = *idx1_it; break;
      case IDX2_BTREE: source_metadata = *idx2_it; break;
      case IDX3_BTREE: source_metadata = *idx3_it; break;
      default: assert(0); std::exit(1);
    }
    return source_metadata;
  }
  source_metadata_t* operator->() {
    switch(type) {
      case IDX1_BTREE: source_metadata = *idx1_it; break;
      case IDX2_BTREE: source_metadata = *idx2_it; break;
      case IDX3_BTREE: source_metadata = *idx3_it; break;
      default: assert(0); std::exit(1);
    }
    return &source_metadata;
  }

  bool operator==(const source_metadata_iterator_t& other) const {
    return this->type == other.type &&
           this->idx1_it == other.idx1_it &&
           this->idx2_it == other.idx2_it &&
           this->idx3_it == other.idx3_it;
  }
  bool operator!=(const source_metadata_iterator_t& other) const {
    return this->type != other.type ||
           this->idx1_it != other.idx1_it ||
           this->idx2_it != other.idx2_it ||
           this->idx3_it != other.idx3_it;
  }
};

#endif

