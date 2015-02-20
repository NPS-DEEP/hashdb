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
 * Provides a working context for accessing a LMDB DB.
 *
 * A context must be opened then closed exactly once.
 */

#ifndef LMDB_CONTEXT_HPP
#define LMDB_CONTEXT_HPP
#include "lmdb.h"

class lmdb_context_t {
  private:
  MDB_env* env;
  unsigned int txn_flags; // example MDB_RDONLY
  unsigned int dbi_flags; // example MDB_DUPSORT
  int state;

  // do not allow copy or assignment
  lmdb_context_t(const lmdb_context_t&);
  lmdb_context_t& operator=(const lmdb_context_t&);

  public:
  MDB_txn* txn;
  MDB_dbi dbi;
  MDB_cursor* cursor;
  MDB_val key;
  MDB_val data;

  lmdb_context_t(MDB_env* p_env, bool is_writable, bool is_duplicates) :
           env(p_env), txn_flags(0), dbi_flags(0),
           state(0), txn(0), dbi(0), cursor(0), key(), data() {

    // set flags based on bool inputs
    if (is_writable) {
      dbi_flags |= MDB_CREATE;  // DBI
    } else {
      txn_flags |= MDB_RDONLY;  // TXN
    }
    if (is_duplicates) {
      dbi_flags |= MDB_DUPSORT; // DBI
    }
  }

  ~lmdb_context_t() {
    if (state != 2) {
      assert(0);
    }
  }

  void open() {
    if (state++ != 0) {
      assert(0);
    }
 
    // create txn object
    int rc = mdb_txn_begin(env, NULL, txn_flags, &txn);

    // create the database handle integer
    rc = mdb_dbi_open(txn, NULL, dbi_flags, &dbi);
    if (rc != 0) {
      std::cerr << "LMDB dbi error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // create a cursor object to use with this txn
    rc = mdb_cursor_open(txn, dbi, &cursor);
    if (rc != 0) {
      std::cerr << "LMDB cursor error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  void close() {
    if (state++ != 1) {
      assert(0);
    }

    // free cursor
    mdb_cursor_close(cursor);

    // do not close dbi handle, why not close it?

    // free txn object
    if ((txn_flags & READ_ONLY) == 0) {
      // RW
      mdb_txn_commit(txn);
    } else {
      // RO
      mdb_txn_abort(txn);
    }
  }
};

#endif

