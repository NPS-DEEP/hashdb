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
 * Provides hashdb filenames.
 */

#include <string>
#include "hashdb_filenames.hpp"

namespace hashdb_filenames_t {

    std::string SETTINGS_FILENAME              = "settings.xml";
    std::string LOG_FILENAME                   = "log.xml";
    std::string LOG_HISTORY_FILENAME           = "log_history.xml";
    std::string HASH_STORE_FILENAME            = "hash_store";
    std::string HASH_DUPLICATES_STORE_FILENAME = "hash_duplicates_store";
    std::string SOURCE_LOOKUP_FILENAME         = "source_lookup_store";
    std::string BLOOM1_FILENAME                = "bloom_filter_1";
    std::string BLOOM2_FILENAME                = "bloom_filter_2";

    std::string settings_filename(const std::string& hashdb_dir) {
      return (hashdb_dir + "/" + SETTINGS_FILENAME);
    }
 
    std::string log_filename(const std::string& hashdb_dir) {
      return (hashdb_dir + "/" + LOG_FILENAME);
    }
 
    std::string log_history_filename(const std::string& hashdb_dir) {
      return (hashdb_dir + "/" + LOG_HISTORY_FILENAME);
    }
 
    std::string hash_store_filename(const std::string& hashdb_dir) {
      return hashdb_dir + "/" + HASH_STORE_FILENAME;
    }

    std::string hash_duplicates_store_filename(const std::string& hashdb_dir) {
      return hashdb_dir + "/" + HASH_DUPLICATES_STORE_FILENAME;
    }

    std::string source_lookup_filename(const std::string& hashdb_dir) {
      return hashdb_dir + "/" + SOURCE_LOOKUP_FILENAME;
    }

    std::string bloom1_filename(const std::string& hashdb_dir) {
      return hashdb_dir + "/" + BLOOM1_FILENAME;
    }

    std::string bloom2_filename(const std::string& hashdb_dir) {
      return hashdb_dir + "/" + BLOOM2_FILENAME;
    }
}

