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
 * print LMDB val fields for diagnostics.
 */

#ifndef PRINT_LMDB_VAL_HPP
#define PRINT_LMDB_VAL_HPP

#include <iostream>
#include <string>
#include "to_hex.hpp"

namespace hashdb {

  static void print_mdb_val(const std::string& name, const MDB_val& val) {
    std::cerr << name << ": " << hashdb::to_hex(
         std::string(static_cast<char*>(val.mv_data), val.mv_size)) << "\n";
  }
}

#endif

