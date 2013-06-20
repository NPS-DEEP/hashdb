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
 * Provides a Bloom filter object to which hashes may be added
 * or checked for a possible match.
 * Note that Bloom filters may have false postives,
 * Bloom filters have no false negatives,
 * and Bloom filters are faster to check than databases.
 */

#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP
#include "hashdb_types.h"
#include "bloom.hpp"
#include "dfxml/src/hash_t.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>

class bloom_filter_t {
  public:
    const bool is_used;
  private:
    const std::string filename;
    const file_mode_type_t file_mode_type;
    const bloom_settings_t settings;
    NSRLBloom bloom;
    bloom_filter_t();

  public:
    bloom_filter_t (const std::string& _filename,
                    file_mode_type_t _file_mode_type,
                    bloom_settings_t _settings) :
                          is_used(_settings.is_used),
                          filename(_filename),
                          file_mode_type(_file_mode_type),
                          settings(_settings),
                          bloom() {
      // bloom filter is not fully instantiated unless it is used
      if (is_used) {
        int success = 0;
        // open based on file mode
        switch(file_mode_type) {
        case READ_ONLY:
          success = bloom.open(filename.c_str(), O_RDONLY);

          // validate
          if (success != 0) {
            std::cerr << "Unable to open Bloom filter file '" << filename
                      << "' for reading.\n";
            std::cerr << strerror(errno) << "\n";
            std::cerr << "Cannot continue.\n";
            exit(1);
          }
          break;
        case RW_NEW:
          success = bloom.create(filename.c_str(),
                       128,
                       settings.M_hash_size,
                       settings.k_hash_functions,
                       "no message");

          // validate
          if (success != 0) {
            std::cerr << "Unable to open new Bloom filter file '" << filename
                      << "'.\n";
            std::cerr << strerror(errno) << "\n";
            std::cerr << "Cannot continue.\n";
            exit(1);
          }
          break;
        case RW_MODIFY:
          success = bloom.open(filename.c_str(), O_RDWR);

          // validate
          if (success != 0) {
            std::cerr << "Unable to open Bloom filter file '" << filename
                      << "' for modification.\n";
            std::cerr << strerror(errno) << "\n";
            std::cerr << "Cannot continue.\n";
            exit(1);
          }
          break;
        }
      }
    }

    void add_hash_value(const md5_t& md5) {
//std::cout << "bloom_filter add_hash_value " << md5 << " " << filename << "\n";
      if (is_used) {
        // program error
        bloom.add(md5.digest);
      } else {
        assert(0);
      }
    }

    bool is_positive(const md5_t& md5) const {
//std::cout << "bloom_filter is_positive " << md5 << " " << bloom.query(md5.digest) << " " << filename << "\n";
      if (is_used) {
        return bloom.query(md5.digest);
      } else {
        // program error
        assert(0);
        return false;
      }
    }

    void report_status(std::ostream& os, size_t index) const {
      if (is_used) {
        os << "bloom filter " << index << " status: ";
        os << "status=" << bloom_state_to_string(settings.is_used);
        os << ", added items=" << bloom.added_items;
        os << ", unique added items=" << bloom.unique_added_items;
        os << ", aliased adds=" << bloom.aliased_adds;
        os << ", hits=" << bloom.hits << "\n";
      } else {
        os << "Bloom filter " << index << " not used\n";
      }
    }

    void report_status(dfxml_writer& x, size_t index) const {
      x.push("bloom_filter_status");
      x.xmlout("index", index);
      x.xmlout("status", bloom_state_to_string(settings.is_used));
      if (is_used) {
        x.xmlout("added_items", bloom.added_items);
        x.xmlout("unique_added_items", bloom.unique_added_items);
        x.xmlout("aliased_adds", bloom.aliased_adds);
        x.xmlout("hits", bloom.hits);
      }
      x.pop();
    }

    std::ostream& diagnostics_state(std::ostream& os) const {
//      os << "(bloom_filter is_used=" << is_used
      os << "(is_used=" << is_used
         << ",filename=" << filename
         << ",M_hash_size=" << settings.M_hash_size
         << ",k_hash_functions=" << settings.k_hash_functions << ")";
      return os;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class bloom_filter_t& filter) {
  filter.diagnostics_state(os);
  return os;
}

#endif
