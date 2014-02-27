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
 * Provides a source lookup index iterator allowing the ability to
 * provide the set of repository name, filename pairs that are in the
 * source dataset.
 *
 * Note that unused values can exist because unused values are not deleted.
 * Dereferences to pair<std::string repository_name, std::string filename>.
 */

#ifndef SOURCE_LOOKUP_INDEX_ITERATOR_HPP
#define SOURCE_LOOKUP_INDEX_ITERATOR_HPP
#include "boost/btree/btree_index_set.hpp" // for iterator
#include "bi_data_types.hpp"
#include "bi_store.hpp"

class source_lookup_index_iterator_t {
  private:

  // the stores behind the iterator
  typedef bi_store_t<bi_data_64_pair_t>  source_lookup_store_t;
  typedef bi_store_t<bi_data_64_sv_t>    repository_name_lookup_store_t;
  typedef bi_store_t<bi_data_64_sv_t>    filename_lookup_store_t;

  source_lookup_store_t*                 source_lookup_store;
  repository_name_lookup_store_t*        repository_name_lookup_store;
  filename_lookup_store_t*               filename_lookup_store;

  // the iterator to the source lookup store
  boost::btree::btree_index_set<bi_data_64_pair_t>::iterator source_lookup_store_it;

  // the dereferenced value
  bool dereferenced_value_is_cached;
  std::pair<std::string, std::string> dereferenced_value;

  // copied from source_lookup_index_manager to avoid circular referencing:
  // helper
  std::string to_string(const boost::string_view& sv) const {
    std::string s = "";
    boost::string_view::const_iterator it = sv.begin();
    while (it != sv.end()) {
      s.push_back(*it);
      ++it;
    }
    return s;
  }

  // semi-copied from source_lookup_index_manager to avoid circular referencing:
  std::pair<std::string, std::string> find(uint64_t source_lookup_index) const {

    // get the lookup pair from the index
    std::pair<uint64_t, uint64_t> lookup_pair;
    bool status1 = source_lookup_store->get_value(
                                     source_lookup_index, lookup_pair);
    if (status1 == false) {
      // program error
      assert(0);
    }

    // get repository name from repository name index
    boost::string_view repository_name_sv;
    bool status2 = repository_name_lookup_store->get_value(
                                    lookup_pair.first, repository_name_sv);
    if (status2 != true) {
      // program error
      assert(0);
    }

    // get filename from filename index
    boost::string_view filename_sv;
    bool status3 = filename_lookup_store->get_value(
                                    lookup_pair.second, filename_sv);
    if (status3 != true) {
      // program error
      assert(0);
    }

    std::pair<std::string, std::string> string_pair(to_string(repository_name_sv),
                                               to_string(filename_sv));
    return string_pair;
  }

  // elemental forward iterator accessors are increment, equal, and dereference
  // increment
  void increment() {
    dereferenced_value_is_cached = false;
    ++source_lookup_store_it;
  }

  // equal
  bool equal(source_lookup_index_iterator_t const& other) const {
    return this->source_lookup_store_it == other.source_lookup_store_it;
  }

  // dereference
  void dereference() {
    if (dereferenced_value_is_cached) {
      // already valid
      return;
    }

    // the dereferenced source lookup store
    bi_data_64_pair_t bi_data_64_pair = *source_lookup_store_it;

    // get repository name from repository name index
    boost::string_view repository_name_sv;
    bool status2 = repository_name_lookup_store->get_value(
                       bi_data_64_pair.value.first, repository_name_sv);
    if (status2 != true) {
      // program error
      assert(0);
    }

    // get filename from filename index
    boost::string_view filename_sv;
    bool status3 = filename_lookup_store->get_value(
                       bi_data_64_pair.value.second, filename_sv);
    if (status3 != true) {
      // program error
      assert(0);
    }

    dereferenced_value = std::pair<std::string, std::string>(
                                              to_string(repository_name_sv),
                                              to_string(filename_sv));

    dereferenced_value_is_cached = true;
  }

  public:
  // the constructor
  source_lookup_index_iterator_t(
             source_lookup_store_t* p_source_lookup_store,
             repository_name_lookup_store_t* p_repository_name_lookup_store,
             filename_lookup_store_t* p_filename_lookup_store,
             boost::btree::btree_index_set<bi_data_64_pair_t>::iterator 
                                                   p_source_lookup_store_it) :
         source_lookup_store(p_source_lookup_store),
         repository_name_lookup_store(p_repository_name_lookup_store),
         filename_lookup_store(p_filename_lookup_store),
         source_lookup_store_it(p_source_lookup_store_it),
         dereferenced_value_is_cached(false),
         dereferenced_value() {
  }

  // compiler requires this.
  // this is particularly bad because this class has iterators
  source_lookup_index_iterator_t(const source_lookup_index_iterator_t& other) :
         source_lookup_store(other.source_lookup_store),
         repository_name_lookup_store(other.repository_name_lookup_store),
         filename_lookup_store(other.filename_lookup_store),
         source_lookup_store_it(other.source_lookup_store_it),
         dereferenced_value_is_cached(false),
         dereferenced_value() {
  }

  // compiler requires this.
  // this is particularly bad because this class has iterators
  source_lookup_index_iterator_t& operator=(const source_lookup_index_iterator_t& other) {
    source_lookup_store = other.source_lookup_store;
    repository_name_lookup_store = other.repository_name_lookup_store;
    filename_lookup_store = other.filename_lookup_store;
    source_lookup_store_it = other.source_lookup_store_it;
    dereferenced_value = std::pair<std::string, std::string>();
    dereferenced_value_is_cached = false;
    return *this;
  }
 

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  source_lookup_index_iterator_t& operator++() {
    increment();
    return *this;
  }
  source_lookup_index_iterator_t operator++(int) {
    source_lookup_index_iterator_t temp(*this);
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
  bool operator==(const source_lookup_index_iterator_t& other) const {
    return equal(other);
  }
  bool operator!=(const source_lookup_index_iterator_t& other) const {
    return !equal(other);
  }
};

#endif

