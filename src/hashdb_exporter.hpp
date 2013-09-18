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
 * Provides the service of exporting the hashdb in DFXML format.
 */

#ifndef HASHDB_EXPORTER_HPP
#define HASHDB_EXPORTER_HPP
#include "hashdb_types.h"
#include "dfxml/src/dfxml_writer.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

extern std::string command_line;

/**
 * Provides the service of exporting the hashdb in DFXML format.
 */
class hashdb_exporter_t {

  private:
  const std::string dfxml_outfilename;
  dfxml_writer x;

  public:
    /**
     * Open a DFXML file and import its hash elements.
     */
    hashdb_exporter_t (const std::string& _dfxml_outfilename) 
                    : dfxml_outfilename(_dfxml_outfilename),
                      x(_dfxml_outfilename, false) {

      // start with dfxml tag
      x.push("dfxml");

      // add creator information
      x.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "svn not tracked", command_line);
    }

    void do_export(const hashdb_db_manager_t& hashdb_in) {
      const size_t hash_block_size = hashdb_in.hashdb_settings.hash_block_size;

      hashdb_db_manager_t::hashdb_iterator_t it = hashdb_in.begin();
      hashdb_db_manager_t::hashdb_iterator_t end = hashdb_in.end();
      while (it != end) {

        // get the hashdb element associated with the iterator
        hashdb_element_t hashdb_element = *it;

        // get md5 from first
        md5_t md5 = hashdb_element.first;

        // get offset, repository_name, and filename from second
        uint64_t file_offset = hashdb_element.second.file_offset;
        std::string repository_name = hashdb_element.second.repository_name;
        std::string filename = hashdb_element.second.filename;

        // start the fileobject tag
        x.push("fileobject");

        // write the repository name tag
        x.xmlout("repository_name", repository_name);

        // write the filename tag
        x.xmlout("filename", std::string(filename));

        // start the byte_run tag with its file_offset attribute
        std::stringstream ss;
        ss << "file_offset='" << file_offset << "' len='" << hash_block_size << "'";
        x.push("byte_run", ss.str());

        // write the hashdigest
        std::string hashdigest = md5.hexdigest();
        x.xmlout("hashdigest", hashdigest, "type='MD5'", false);

        // close the byte_run tag
        x.pop();

        // close the fileobject tag
        x.pop();

        // move to next iterator
        ++it;
      }
    }

    ~hashdb_exporter_t() {
      // close dfxml_writer
      x.add_rusage();
      x.pop();
      x.close();
    }

};
#endif

