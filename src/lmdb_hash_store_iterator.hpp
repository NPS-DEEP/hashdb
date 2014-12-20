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
 */

// Beware use of the class-specific allocated cursor resource

#ifndef LMDB_HASH_STORE_ITERATOR_HPP
#define LMDB_HASH_STORE_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "lmdb.h"
#include "lmdb_resources.h"
#include "lmdb_resource_manager.hpp"

class lmdb_hash_store_iterator_t {
  private:
  const lmdb_resource_manager_t* lmdb_resource_manager;
  pair_t pair;
  bool at_end;

  MDB_cursor* open_cursor() {
    // get the stored thread-specific resources
    pthread_resources_t* resources = lmdb_resource_manager->get_pthread_resources();

    MDB_cursor* cursor;
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }
    return cursor;
  }

  void increment() {
    if (at_end) {
      std::cerr << "lmdb_hash_store_iterator: increment requested when at end\n";
      assert(0);
    }
    MDB_val key;
    MDB_val data;
    pair_to_mdb(pair.first, pair.second, key, data);

    // set cursor onto current key, data pair
    MDB_cursor* cursor = open_cursor();
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
    if (rc != 0) {
      // invalid usage
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    if (rc == 0) {
      pair = mdb_to_pair(key, data);
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
      pair = pair_t();
    } else if (rc != 0) {
      // program error
      assert(0);
    }

    // close cursor
    mdb_cursor_close(cursor);
  }

  public:
  // set to key, data pair
  lmdb_hash_store_iterator_t(
                      const lmdb_resource_manager_t& p_lmdb_resource_manager,
                      hash_t p_hash, uint64_t p_value):
               lmdb_resource_manager(&p_lmdb_resource_manager),
               pair(),
               at_end(false) {

    // set the cursor to this key, data position
    MDB_val key;
    MDB_val data;
    pair_to_mdb(p_hash, p_value, key, data);
    MDB_cursor* cursor = open_cursor();
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
    if (rc == 0) {
      pair = mdb_to_pair(key, data);
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else {
      // program error
      assert(0);
    }

    // close cursor
    mdb_cursor_close(cursor);
  }

  // set to lower bound or upper bound for key
  lmdb_hash_store_iterator_t(
                      const lmdb_resource_manager_t& p_lmdb_resource_manager,
                      hash_t p_hash, bool is_lower_bound) :
               lmdb_resource_manager(&p_lmdb_resource_manager),
               pair(),
               at_end(false) {

    // set the cursor to this key position
    MDB_val key;
    MDB_val data;
    pair_to_mdb(p_hash, 0, key, data);
    MDB_cursor* cursor = open_cursor();
    int rc;

    if (is_lower_bound) {

      // first record with key
      rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH_RANGE);
      if (rc == 0) {
        pair = mdb_to_pair(key, data);
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
        assert(0);
      }
    } else {

      // first record after key
      // make sure a record with key exists
      rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH_RANGE);
      if (rc == 0) {
        // good, key exists, move past it
        rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT_NODUP);
        if (rc == 0) {
          pair = mdb_to_pair(key, data);
        } else if (rc == MDB_NOTFOUND) {
          // no key
          at_end = true;
          return;
        } else {
          // program error
          std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
          assert(0);
        }
      } else if (rc == MDB_NOTFOUND) {
        // no key
        at_end = true;
        return;
      } else {
        // program error
        assert(0);
      }
    }

    // close cursor
    mdb_cursor_close(cursor);
  }

  // set to begin or end of DB
  lmdb_hash_store_iterator_t(
                      const lmdb_resource_manager_t& p_lmdb_resource_manager,
                      bool is_begin) :
               lmdb_resource_manager(&p_lmdb_resource_manager),
               pair(),
               at_end(true) {

    // set the cursor
    if (is_begin) {
      MDB_val key;
      MDB_val data;
      int rc;
      MDB_cursor* cursor = open_cursor();
      rc = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);
      if (rc == 0) {
        pair = mdb_to_pair(key, data);
        at_end = false;
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
        assert(0);
      }

      // close cursor
      mdb_cursor_close(cursor);

    } else {
      at_end = true;
    }
  }

  // this useless default constructor is required by std::pair
  lmdb_hash_store_iterator_t() : lmdb_resource_manager(0),
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

  const pair_t& operator*() const {
    if (at_end) {
      assert(0);
    }
    return pair;
  }

  const pair_t* operator->() const {
    if (at_end) {
      assert(0);
    }
    return &pair;
  }

  bool operator==(const lmdb_hash_store_iterator_t& other) const {
    if (at_end == other.at_end) return true;
    if (at_end || other.at_end) return false;
    return (pair.first == other.pair.first &&
            pair.second == other.pair.second);
  }

  bool operator!=(const lmdb_hash_store_iterator_t& other) const {
    if (at_end == other.at_end) return false;
    if (at_end || other.at_end) return true;
    return (!(pair.first == other.pair.first) ||
            pair.second != other.pair.second);
  }
};

#endif

