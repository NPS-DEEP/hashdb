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

#ifndef ROCKS_DB_MERGE_HPP
#define ROCKS_DB_MERGE_HPP

#include <string>
#include <stdint.h>

namespace hashdb {

class source_id_merge_t : public AssociativeMergeOperator {

  private:
  changes_t* changes;

  // do not allow copy or assignment
  source_id_merge_t(const source_id_merge_t&) = delete;
  source_id_merge_t& operator=(const source_id_merge_t&) = delete;

  public:
  source_id_merge_t(changes_t p_changes);
  virtual bool Merge(
    const Slice& key,
    const Slice* existing_value,
    const Slice& value,
    std::string* new_value,
    Logger* logger) const override {
  }
};


#endif

