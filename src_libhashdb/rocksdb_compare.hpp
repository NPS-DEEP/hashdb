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

#ifndef ROCKS_DB_COMPARE_HPP
#define ROCKS_DB_COMPARE_HPP

#include <string>
#include <cstring>
#include <rocksdb/slice.h>

namespace hashdb {

class source_id_compare_t : public rocksdb::Comparator {

  public:
  // compare uint64_t protobuf values
  int Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const {
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return std::memcmp(a.data(), b.data(), a.size();
  }

  // comparator name in DB must match
  const char* Name() const { return "source_id_comparator"; }

  // unused
  void FindShortestSeparator(std::string*, const rocksdb::Slice&) const { }
  void FindShortSuccessor(std::string*) const { }
};

#endif

