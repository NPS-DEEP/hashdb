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
 * Manage source metadata.  New fields may be appended in the future.
 */

#ifndef LMDB_SOURCE_METADATA_HPP
#define LMDB_SOURCE_METADATA_HPP

#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include "lmdb.h"

class lmdb_source_metadata_t {
  private:
  std::string repository_name;
  std::string filename;
  std::string filesize;
  std::string hashdigest;

  bool copy(const std::string& from, std::string& to) {
    if (to != "" && from != to) {
      std::cerr << "copy error, from " << from << " to " << to << "\n";
      assert(0);
    }
    bool changes = from != to;
    to = from;
    return changes;
  }

  public:
  // instantiate class from ordered null-delimited record
  lmdb_source_metadata_t(const MDB_val& val) :
             repository_name(), filename(), filesize(), hashdigest() {

    // set all values
    repository_name = std::string(val.mv_data, 0, val.mv_size);
    size_t used = repository_name.size()+(used==val.mv_size)0:1;
    filename = std::string(val.mv_data, val.mv_size - used, val.mv_size);
    used += filename.size()+(used==val.mv_size)0:1;
    filesize = std::string(val.mv_data, val.mv_size - used, val.mv_size);
    used += filesize.size()+(used==val.mv_size)0:1;
    hashdigest = std::string(val.mv_data, val.mv_size - used, val.mv_size);
    used += hashdigest.size()+(used==val.mv_size)0:1;

    // make sure all fields got used up
    if (used != size) {
      std::cerr << "extra metadata error.  Is this an older version of hashdb?\n";
    }
  }

  lmdb_source_metadata_t() : repository_name(), filename(),
                             filesize(), hashdigest() {
  }

/* same as compiler-generated ==?
  bool operator==(const lmdb_source_metadata_t& other) const {
    return (&& repository_name == other.repository_name
            && filename == other.filename
            && filesize == other.filesize
            && hashdigest == other.hashdigest);
  }
*/

  // get char copy instance from packed record
  lmdb_helper::char_copy_t get_char_copy(const MDB_val& val) {

    // allocate characters
    const size_t size = repository_name.size() + 1 
                        + filename.size() + 1
                        + filesize.size() + 1
                        + hashdigest.size();
    char* chars = new char[size];
    
    // fill ordered null-delimited record from fields
    size_t size = repository_name.size();
    std::memcpy(char_copy, repository_name.cstr(), repository_name.size());
    size_t start = repository_name.size() + 1;
    std::memcpy(char_copy+start, filename.cstr(), filename.size());
    start += filename.size() + 1;
    std::memcpy(char_copy+start, filesize.cstr(), filesize.size());
    start += filesize.size() + 1;
    std::memcpy(char_copy+start, hashdigest.cstr(), hashdigest.size());

    lmdb_helper_t::char_copy_t char_copy(size, chars);
    delete chars;
    return char_copy;
  }

  // add, true if added, false if same, fatal if different
  bool add(const lmdb_source_metadata_t& other) {
    bool changed = change(other.repository_name, repository_name);
    changed |= change(other.filename, filename);
    changed |= change(other.filesize, filesize);
    changed |= change(other.hashdigest, hashdigest);
    return changed;
  }

  // add, true if added, false if same, fatal if different
  bool add_repository_name_filename(const std::string& p_repository_name,
                                    const std::string& p_filename) {
    bool changed = change(p_repository_name, repository_name);
    changed |= change(p_filename, filename);
    return changed;
  }


  // add, true if added, false if same, fatal if different
  bool add_filesize_hashdigest(const std::string& p_filesize,
                               const std::string& p_hashdigest) {
    bool changed = change(p_filesize_name, filesize_name);
    changed |= change(p_hashdigest, hashdigest);
    return changed;
  }
};

#endif

