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
 * Provide structure for storing source ID and sub_count pairs.
 */

#ifndef SOURCE_ID_SUB_COUNTS_HPP
#define SOURCE_ID_SUB_COUNTS_HPP

#include <string>
#include <set>
#include <cstdint>

namespace hashdb {

struct source_id_sub_count_t {
  uint64_t source_id;
  uint64_t sub_count;
  source_id_sub_count_t() : source_id(0), sub_count(0) {
  }
  source_id_sub_count_t(uint64_t p_source_id,
                     uint64_t p_sub_count) :
              source_id(p_source_id),
              sub_count(p_sub_count) {
  }
  bool operator<(const source_id_sub_count_t& that) const {
    if (source_id < that.source_id) return true;
    if (source_id > that.source_id) return false;

    // the above should be sufficient but lets be complete
    if (sub_count < that.sub_count) return true;
    return false;
  }
};

typedef std::set<source_id_sub_count_t> source_id_sub_counts_t;

} // end namespace hashdb

#endif

