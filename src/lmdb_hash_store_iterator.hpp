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
 * Provides a lmdb_hash_store iterator which dereferences into
 * key and value fields.
 */

#ifndef LMDB_HASH_STORE_ITERATOR_HPP
#define LMDB_HASH_STORE_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "liblmdb/lmdb.h"

//// macros for use with LMDB
//#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
//#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
//#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
//	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))

// key and data types for hash_store
// zz may pass as template instead using T_K, T_D
typedef hash_t key_t;
typedef uint64_t data_t;
typedef std::pair<key_t, data_t> pair_t;

class lmdb_hash_store_iterator_t {
  private:
  MDB_txn* txn;        // transaction handle pointer
  MDB_dbi dbi;         // database handle integer
  MDB_val key;         // generic structure for passing data
  MDB_val data;        // generic structure for passing data
  MDB_cursor* cursor;  // cursor handle
  pair_t cached_pair;
  bool at_end;

  public:
  lmdb_hash_store_iterator_t(
                  MDB_txn* txn,
                  MDB_dbi dbi,
                  MDB_val key,
                  MDB_val data,
                  MDB_cursor_op get_op // cursor GET operation
                  ) : txn(0), dbi(), key(), data(), cursor(0), cached_pair(), at_end(false) {

    // create a cursor object
    E(mdb_cursor_open(txn, dbi, &cursor));

    // set the cursor
    int rc = mdb_cursor_get(cursor, &key, &data, get_op);
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  // this useless default constructor is required by std::pair
  lmdb_hash_store_iterator_t() : txn(0), dbi(), key(), data(), cursor(0), cached_pair(), at_end(false) {
  }

  // override the copy constructor and the assignment operator to quiet the
  // compiler about using a pointer data member
  lmdb_hash_store_iterator_t(const lmdb_hash_store_iterator_t& other) :
               txn(other.txn),
               dbi(other.dbi),
               key(),  // do not forward key
               data(), // do not forward data
               cursor(other.cursor),
               cached_pair(), // do not forward cached pair
               at_end(other.at_end) {
  }
  lmdb_hash_store_iterator_t& operator=(const lmdb_hash_store_iterator_t& other) {
    txn = other.txn;
    dbi = other.dbi;
    // do not forward key
    // do not forward data
    cursor = other.cursor;
    // do not forward cached pair
    at_end = other.at_end;
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  lmdb_hash_store_iterator_t& operator++() {
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
    return *this;
  }

  lmdb_hash_store_iterator_t operator++(int) {  // c++11 delete would be better.
    lmdb_hash_store_iterator_t temp(*this);
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
    return temp;
  }

  std::pair<key_t, data_t>& operator*() {
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_CURRENT);
    if (rc == MDB_NOTFOUND) {
      // program error
      assert(0);
    } else if (rc != 0) {
      // possible invalid parameter
      assert(0);
    }
    pair = pair_to_hash_store(key, data);
    return pair;
  }

  hashdb_element_t* operator->() {
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_CURRENT);
    if (rc == MDB_NOTFOUND) {
      // program error
      assert(0);
    } else if (rc != 0) {
      // possible invalid parameter
      assert(0);
    }
    pair = pair_to_hash_store(key, data);
    return &pair;
  }

  bool operator==(const hashdb_iterator_t& other) const {
    if (at_end == other.at_end) return true;
    if (at_end || other.at_end) return false;
    pair_t this_pair = *this;
    pair_t other_pair = *other;
    return (this_pair.first == other_pair.first &&
            this_pair.second == other_pair.second);
  }

  }
  bool operator!=(const hashdb_iterator_t& other) const {
    if (at_end == other.at_end) return false;
    if (at_end || other.at_end) return true;
    pair_t this_pair = *this;
    pair_t other_pair = *other;
    return (this_pair.first != other_pair.first ||
            this_pair.second != other_pair.second);
  }
};

#endif

