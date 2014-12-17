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

#ifndef LMDB_HASH_STORE_ITERATOR_HPP
#define LMDB_HASH_STORE_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "lmdb.h"
#include "lmdb_resources.h"

class lmdb_hash_store_iterator_t {
  private:
  pthread_resources_t* resources;
  pair_t pair;
  MDB_cursor* cursor;  // cursor just for this iterator
  bool at_end;

  void dereference() {
    MDB_val key;
    MDB_val data;
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_CURRENT);
    if (rc == MDB_NOTFOUND) {
      // program error
      assert(0);
    } else if (rc != 0) {
      // possible invalid parameter
      assert(0);
    }

    pair = mdb_to_pair(key, data);
  }

  void increment() {
    if (at_end) {
      assert(0);
    }
    MDB_val key;
    MDB_val data;
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    if (rc == 0) {
      pair = mdb_to_pair(key, data);
std::cout << "lmdb_hash_store_iterator increment pair " << pair.first.hexdigest() << ", " << pair.second << "\n";
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
      pair = pair_t();
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  public:
  // set to key, data pair
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  hash_t p_hash,
                  uint64_t p_value):
                        resources(p_resources),
                        pair(),
                        cursor(),
                        at_end(false) {

    // create a cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor to this key, data position
    MDB_val key;
    MDB_val data;
    pair_to_mdb(p_hash, p_value, key, data);
    rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
    if (rc == 0) {
      pair = mdb_to_pair(key, data);
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else {
      // program error
      assert(0);
    }
  }

  // set to lower bound or upper bound for key
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  hash_t p_hash,
                  bool is_lower_bound) :
                        resources(p_resources),
                        pair(),
                        cursor(),
                        at_end(false) {

    // set the cursor to this key
    MDB_val key;
    MDB_val data;
    pair_to_mdb(p_hash, 0, key, data);

std::cout << "lmdb_hash_store_iterator new cursor.a " << cursor << "\n";
    // create the cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor
    if (is_lower_bound) {
      // first with key
      rc = mdb_cursor_get(cursor, &key, &data, MDB_SET_RANGE);
    } else {
      // first after key
      rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT_NODUP);
    }
    if (rc == 0) {
      pair = mdb_to_pair(key, data);
std::cout << "lmdb_hash_store_iterator ctor pair.a " << pair.first.hexdigest() << ", " << pair.second << "\n";
    } else if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else {
      // program error
      std::cerr << "rc: " << rc << ", lb: " << is_lower_bound << "\n";
      assert(0);
    }
  }

  // set to begin or end
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  bool is_begin) :
                        resources(p_resources),
                        pair(),
                        cursor(),
                        at_end(true) {

    // create a cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor
    MDB_val key;
    MDB_val data;
    if (is_begin) {
      rc = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);
      if (rc == 0) {
        pair = mdb_to_pair(key, data);
      } else if (rc == MDB_NOTFOUND) {
        at_end = true;
      } else {
        // program error
        std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
        assert(0);
      }
    } else {
      rc = MDB_NOTFOUND;
    }
  }

  // this useless default constructor is required by std::pair
  lmdb_hash_store_iterator_t() : resources(), pair(), cursor(0), at_end(false) {
  }

  ~lmdb_hash_store_iterator_t() {
    mdb_cursor_close(cursor);
  }

  // override the copy constructor and the assignment operator to quiet the
  // compiler about using a pointer data member
  lmdb_hash_store_iterator_t(const lmdb_hash_store_iterator_t& other) :
               resources(other.resources),
               pair(other.pair),
               cursor(other.cursor),
               at_end(other.at_end) {
  }
  lmdb_hash_store_iterator_t& operator=(const lmdb_hash_store_iterator_t& other) {
    resources = other.resources;
    pair = other.pair;
    cursor = other.cursor;
    at_end = other.at_end;
    return *this;
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
    return pair;
  }

  const pair_t* operator->() const {
    return &pair;
  }
/*
  pair_t& operator*() {
    return pair;
  }

  pair_t* operator->() {
    return &pair;
  }
*/

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

