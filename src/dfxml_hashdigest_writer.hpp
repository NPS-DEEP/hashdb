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
 * Writes hashdb elements in DFXML format.
 */

#ifndef DFXML_HASHDIGEST_WRITER_HPP
#define DFXML_HASHDIGEST_WRITER_HPP
#include <dfxml_writer.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include "globals.hpp"

/**
 * Provides the service of exporting the hashdb in DFXML format.
 */
class dfxml_hashdigest_writer_t {

  private:
  const std::string dfxml_file;
  const uint64_t hash_block_size;
  dfxml_writer x;

  public:
  /**
   * Open a DFXML file for writing.
   */
  dfxml_hashdigest_writer_t (const std::string& p_dfxml_file,
                             uint64_t p_hash_block_size) :
                    dfxml_file(p_dfxml_file),
                    hash_block_size(p_hash_block_size),
                    x(dfxml_file, false) {

    // start with dfxml tag
    x.push("dfxml");

    // add creator information
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "", globals_t::command_line_string);
  }

  ~dfxml_hashdigest_writer_t() {
    // close dfxml_writer
    x.pop();
    x.close();
  }

  // add a hashdb element
  void add_hashdb_element(const std::string& binary_hash,
                          lmdb_source_data_t source_data,
                          uint64_t file_offset) {

    // start the fileobject tag
    x.push("fileobject");

    // write the repository name tag
    x.xmlout("repository_name", source_data.repository_name);

    // write the filename tag
    x.xmlout("filename", source_data.filename);

    // if available, write filesize
    if (source_data.filesize != 0) {
      x.xmlout("filesize", source_data.filesize);
    }

    // if available, write file hashdigest
    if (source_data.binary_hash != "") {
      std::string file_hashdigest =
                   lmdb_helper::binary_hash_to_hex(source_data.binary_hash);
      x.xmlout("hashdigest", file_hashdigest, "type='MD5'", false);
    }

    // start the byte_run tag with its file_offset attribute
    std::stringstream ss;
    ss << "file_offset='" << file_offset
       << "' len='" << hash_block_size << "'";
    x.push("byte_run", ss.str());

    // write the block hashdigest
    std::string block_hashdigest =
                   lmdb_helper::binary_hash_to_hex(binary_hash);
    x.xmlout("hashdigest", block_hashdigest, "type='MD5'", false);

    // close the byte_run tag
    x.pop();

    // close the fileobject tag
    x.pop();
  }
};

#endif

