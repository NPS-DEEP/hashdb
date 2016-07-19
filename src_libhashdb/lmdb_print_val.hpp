// Author:  Bruce Allen
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
 * print LMDB val fields for diagnostics.
 */

#ifndef LMDB_PRINT_VAL_HPP
#define LMDB_PRINT_VAL_HPP

#include <iostream>
#include <string>
#include <cassert>
#include "lmdb.h"
#include "hashdb.hpp" // for bin_to_hex

namespace hashdb {

  // print MDB_val size and bytes in hex
  void print_mdb_val(const std::string& name, const MDB_val& val) {
    std::cerr << name << ", " << val.mv_data << ": " << hashdb::bin_to_hex(
         std::string(static_cast<char*>(val.mv_data), val.mv_size)) << "\n";
  }

  // Print LMDB entries.  Leave the cursor at end.
  void print_whole_mdb(const std::string& name, MDB_cursor* cursor) {
    std::cerr << "DB walk: " << name << "\n";
    MDB_val key;
    MDB_val data;
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);

    while (rc == 0) {
      print_mdb_val("DB walk key", key);
      print_mdb_val("DB walk data", data);
      rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    }
    if (rc != MDB_NOTFOUND) {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
    std::cerr << "DB walk: done.\n";
  }
}

#endif

