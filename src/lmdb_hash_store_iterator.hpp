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
 * Provides a lmdb_hash_store iterator which dereferences into pair type.
 * Internally, position is tracked by hash_pair_t
 */

#ifndef LMDB_HASH_STORE_ITERATOR_HPP
#define LMDB_HASH_STORE_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "lmdb.h"
#include "lmdb_resources.h"
#include "lmdb_resources_manager.hpp"

class lmdb_hash_store_iterator_t {
  private:
  lmdb_resources_manager_t* resources_manager;
  hash_pair_t pair;
  bool at_end;

  void increment() {
    if (at_end) {
      std::cerr << "lmdb_hash_store_iterator: increment requested when at end\n";
      assert(0);
    }

    // get resources
    lmdb_resources_t* resources = resources_manager->open_resources();

    pair_to_mdb(pair.first, pair.second, resources->key, resources->data);

    // set cursor onto current key, data pair
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data, MDB_GET_BOTH);
    if (rc != 0) {
      // invalid usage
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(resources->cursor,
                        &resources->key, &resources->data, MDB_NEXT);
    if (rc == 0) {
      pair = mdb_to_pair(resources->key, resources->data);
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
      pair = hash_pair_t();
    } else if (rc != 0) {
      // program error
      assert(0);
    }

    // close resources
    resources_manager->close_resources();
  }

  public:
  // set to key, data pair
  lmdb_hash_store_iterator_t(
                      lmdb_resources_manager_t* p_resources_manager,
                      hash_t p_hash, uint64_t p_value):
               resources_manager(p_resources_manager),
               pair(),
               at_end(false) {

    // get resources
    lmdb_resources_t* resources = resources_manager->open_resources();

    // set the cursor to this key, data position
    pair_to_mdb(p_hash, p_value, resources->key, resources->data);
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data, MDB_GET_BOTH);
    if (rc == 0) {
      pair = mdb_to_pair(resources->key, resources->data);
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else {
      // program error
      assert(0);
    }

    // close resources
    resources_manager->close_resources();
  }

  // set to lower bound or upper bound for key
  lmdb_hash_store_iterator_t(
                      lmdb_resources_manager_t* p_resources_manager,
                      hash_t p_hash, bool is_lower_bound) :
               resources_manager(p_resources_manager),
               pair(),
               at_end(false) {

    // get resources
    lmdb_resources_t* resources = resources_manager->open_resources();

    // set the cursor to this key position
    pair_to_mdb(p_hash, 0, resources->key, resources->data);
    int rc;

    if (is_lower_bound) {
      // first record with key
      rc = mdb_cursor_get(resources->cursor,
                          &resources->key, &resources->data, MDB_SET);
      if (rc == 0) {
        pair = mdb_to_pair(resources->key, resources->data);
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
        assert(0);
      }
    } else {
      // first record with key
      rc = mdb_cursor_get(resources->cursor,
                          &resources->key, &resources->data, MDB_SET);
      if (rc == 0) {
        // has key
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
        assert(0);
      }

      // now move to first record after key
      rc = mdb_cursor_get(resources->cursor,
                          &resources->key, &resources->data, MDB_NEXT_NODUP);
      if (rc == 0) {
        pair = mdb_to_pair(resources->key, resources->data);
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
        assert(0);
      }
    }

    // close resources
    resources_manager->close_resources();
  }

  // set to begin or end of DB
  lmdb_hash_store_iterator_t(
                      lmdb_resources_manager_t* p_resources_manager,
                      bool is_begin) :
               resources_manager(p_resources_manager),
               pair(),
               at_end(true) {

    // set the cursor
    if (is_begin) {
      // get resources
      lmdb_resources_t* resources = resources_manager->open_resources();
      int rc;
      rc = mdb_cursor_get(resources->cursor,
                          &resources->key, &resources->data, MDB_FIRST);
      if (rc == 0) {
        pair = mdb_to_pair(resources->key, resources->data);
        at_end = false;
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
        assert(0);
      }

      // close resources
      resources_manager->close_resources();

    } else {
      at_end = true;
    }
  }

  // this useless default constructor is required by std::pair
  lmdb_hash_store_iterator_t() : resources_manager(0),
                                 pair(), at_end(true) {
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  lmdb_hash_store_iterator_t& operator++() {
    increment();
    return *this;
  }

  lmdb_hash_store_iterator_t operator++(int) {  // c++11 delete would be better.
    lmdb_hash_store_iterator_t temp(*this);
    increment();
    return temp;
  }

  const hash_pair_t& operator*() const {
    if (at_end) {
      assert(0);
    }
    return pair;
  }

  const hash_pair_t* operator->() const {
    if (at_end) {
      assert(0);
    }
    return &pair;
  }

  bool operator==(const lmdb_hash_store_iterator_t& other) const {
    if (at_end && other.at_end) return true;
    if (at_end || other.at_end) return false;
    return (pair.first == other.pair.first &&
            pair.second == other.pair.second);
  }

  bool operator!=(const lmdb_hash_store_iterator_t& other) const {
    if (at_end && other.at_end) return false;
    if (at_end || other.at_end) return true;
    return (!(pair.first == other.pair.first) ||
            pair.second != other.pair.second);
  }
};

#endif

