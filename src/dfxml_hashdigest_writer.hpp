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
#include "hash_t_selector.h"
#include "dfxml/src/dfxml_writer.h"
#include "command_line.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

/**
 * Provides the service of exporting the hashdb in DFXML format.
 */
class dfxml_hashdigest_writer_t {

  private:
  const std::string dfxml_file;
  dfxml_writer x;

  public:
  /**
   * Open a DFXML file for writing.
   */
  dfxml_hashdigest_writer_t (const std::string& p_dfxml_file) 
                  : dfxml_file(p_dfxml_file),
                    x(dfxml_file, false) {

    // start with dfxml tag
    x.push("dfxml");

    // add creator information
    x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "", command_line_t::command_line_string);
  }

  ~dfxml_hashdigest_writer_t() {
    // close dfxml_writer
    x.pop();
    x.close();
  }

  // add a hashdb element
  void add_hashdb_element(const hashdb_element_t& element) {

    // start the fileobject tag
    x.push("fileobject");

    // write the repository name tag
    x.xmlout("repository_name", element.repository_name);

    // write the filename tag
    x.xmlout("filename", element.filename);

    // start the byte_run tag with its file_offset attribute
    std::stringstream ss;
    ss << "file_offset='" << element.file_offset
       << "' len='" << element.hash_block_size << "'";
    x.push("byte_run", ss.str());

    // write the hashdigest
    std::stringstream ss2;
    ss2 << "type='" << digest_name<hash_t>() << "'";
    x.xmlout("hashdigest", element.key.hexdigest(), ss2.str(), false);

    // close the byte_run tag
    x.pop();

    // close the fileobject tag
    x.pop();
  }

  // add a source metadata record
  void add_source_metadata(
            const std::pair<bool, std::pair<std::string, std::string> >& lookup_pair,
            const std::pair<bool, source_metadata_t>& source_metadata_pair) {

    // error if invalid lookup pair
    if (lookup_pair.first == false) {
      assert(0);
    }

    // start the fileobject tag
    x.push("fileobject");

    // write the repository name tag
    x.xmlout("repository_name", lookup_pair.second.first);

    // write the filename tag
    x.xmlout("filename", lookup_pair.second.second);

    if (source_metadata_pair.first == true) {
      // write the file size
      x.xmlout("filesize", source_metadata_pair.second.file_size);

      // write the file hashdigest
      std::stringstream ss2;
      ss2 << "type='" << digest_name<hash_t>() << "'";
      x.xmlout("hashdigest", source_metadata_pair.second.file_hash.hexdigest(), ss2.str(), false);
    }

    // close the fileobject tag
    x.pop();
  }
};

#endif

