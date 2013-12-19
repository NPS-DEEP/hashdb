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
 * Body for hashdb_settings_t
 */

#include <config.h>
#include "hashdb_settings.h"
#include "hashdb_types.h"
#include "source_lookup_record.h"
#include "dfxml/src/dfxml_writer.h"
#include "detail/settings_reader.hpp"
#include "detail/settings_writer.hpp"
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>


    /**
     * Open a clone settings object.
     */
    hashdb_settings_t::hashdb_settings_t(const hashdb_settings_t& settings) : 
        hashdb_version(settings.hashdb_version),
        hash_block_size(settings.hash_block_size),
        hashdigest_type(settings.hashdigest_type),
        maximum_hash_duplicates(settings.maximum_hash_duplicates),
        hash_store_settings(settings.hash_store_settings),
        hash_duplicates_store_settings(settings.hash_duplicates_store_settings),
        source_lookup_settings(settings.source_lookup_settings),
        bloom1_settings(settings.bloom1_settings),
        bloom2_settings(settings.bloom2_settings) {
    }

    /**
     * Open a default settings object.
     */
    hashdb_settings_t::hashdb_settings_t() : 
        hashdb_version(1),
        hash_block_size(4096),
        hashdigest_type(HASHDIGEST_MD5),
        maximum_hash_duplicates(0),
        hash_store_settings(),
        hash_duplicates_store_settings(),
        source_lookup_settings(),
        bloom1_settings(true, 3, 28),
        bloom2_settings(false, 3, 28) {
    }

    /**
     * Open a settings object using settings saved in hashdb_dir.
     */
    hashdb_settings_t::hashdb_settings_t(std::string hashdb_dir) :
        hashdb_version(1),
        hash_block_size(4096),
        hashdigest_type(HASHDIGEST_MD5),
        maximum_hash_duplicates(0),
        hash_store_settings(),
        hash_duplicates_store_settings(),
        source_lookup_settings(),
        bloom1_settings(true, 3, 28),
        bloom2_settings(false, 3, 28) {
    }
      try {
        hashdb_settings_t settings = hashdb_settings::read_settings(hashdb_dir, this);
      } catch {
        stderr << "Aborting.\n";
        exit(1);
      }

    /**
     * Save settings into hashdb_dir.
     */
    void hashdb_settings_t::save_settings(std::string hashdb_dir) const {
      hashdb_settings::write_settings(hashdb_dir, this);
    }

    void hashdb_settings_t::report_settings(std::ostream& os) const {
      os << "hashdb settings: ";
      os << "hashdb version=" << hashdb_version << ", ";
      os << "hash block size=" << hash_block_size << ", ";
      os << "hashdigest type=" << hashdigest_type_to_string(hashdigest_type) << ", ";
      os << "maximum hash duplicates=" << maximum_hash_duplicates << "\n";
      hash_store_settings.report_settings(os);
      hash_duplicates_store_settings.report_settings(os);
      source_lookup_settings.report_settings(os);
      bloom1_settings.report_settings(os, 1);
      bloom2_settings.report_settings(os, 2);
    }

    hashdb_settings_t& hashdb_settings_t::operator=(const hashdb_settings_t& other) {
      hashdb_version = other.hashdb_version;
      hash_block_size = other.hash_block_size;
      hashdigest_type = other.hashdigest_type;
      maximum_hash_duplicates = other.maximum_hash_duplicates;
      hash_store_settings = other.hash_store_settings;
      hash_duplicates_store_settings = other.hash_duplicates_store_settings;
      source_lookup_settings = other.source_lookup_settings;
      bloom1_settings = other.bloom1_settings;
      bloom2_settings = other.bloom2_settings;
      return *this;
    }

    void hashdb_settings_t::report_settings(dfxml_writer& x) const {
      x.xmlout("hashdb_version", hashdb_version);
      x.xmlout("hashdigest_type", hashdigest_type_to_string(hashdigest_type));
      x.xmlout("hash_block_size", hash_block_size);
      x.xmlout("maximum_hash_duplicates", (uint64_t)maximum_hash_duplicates);
      hash_store_settings.report_settings(x);
      hash_duplicates_store_settings.report_settings(x);
      source_lookup_settings.report_settings(x);
      bloom1_settings.report_settings(x, 1);
      bloom2_settings.report_settings(x, 2);
    }

