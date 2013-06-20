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
 * Manages the source location record data structure.
 *
 * This file also provides the fixed size source location record.
 * The fixed size source location record is an approach to allow
 * storage in a map.  It should be replaced a compatible variable-length
 * string store, see file [reference?]
 * or fixed length string store, see file boost/btree/detail/fixstr.hpp.
 */

#ifndef SOURCE_LOCATION_RECORD_HPP
#define SOURCE_LOCATION_RECORD_HPP
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include <boost/algorithm/string.hpp>

class source_location_record_t {
  private:
    std::string composite_value_string;

    std::string make_composite_value(std::string repository_name_string,
                                     std::string filename_string) {
      std::stringstream ss;
      ss << repository_name_string << "\t" << filename_string;
      return ss.str();
    }

  public:
    source_location_record_t():composite_value_string("none defined") {
    }

    source_location_record_t(std::string _composite_value_string):
                             composite_value_string(_composite_value_string) {
    }

    source_location_record_t(std::string repository_name_string,
                             std::string filename_string):
                             composite_value_string(make_composite_value(
                             repository_name_string, filename_string)) {
    }

    std::string composite_value() const {
      return composite_value_string;
    }

    std::string repository_name() const {
      size_t tab_index = composite_value_string.find('\t');
      if (tab_index == std::string::npos) {
        return "";
      } else {
        return composite_value_string.substr(0, tab_index);
      }
    }
    std::string filename() const {
      size_t tab_index = composite_value_string.find('\t');
      if (tab_index == std::string::npos) {
        return "";
      } else {

        size_t end_index = composite_value_string.find('\0', tab_index+1);
        return composite_value_string.substr(tab_index+1,
                                             end_index-(tab_index+1));
      }
    }

    source_location_record_t& operator=(const source_location_record_t& other) {
      composite_value_string = other.composite_value_string;
      return *this;
    }

    // support so that md5_t can be used as a key in a map
    bool const operator==(const source_location_record_t& source_location_record) const {
      return (composite_value_string.compare(source_location_record.composite_value_string) == 0);
    }
    bool const operator<(const source_location_record_t& source_location_record) const {
      return (composite_value_string.compare(source_location_record.composite_value_string) < 0);
    }
};

// glue for a fixed size record instead of string
struct fixed_size_source_location_record_t {
  char c[200];

  // copy
  fixed_size_source_location_record_t(const fixed_size_source_location_record_t& r) {
    memcpy(c, r.c, sizeof(fixed_size_source_location_record_t));
  }

  // blank
  fixed_size_source_location_record_t() {
    memset(c, '\0', sizeof(fixed_size_source_location_record_t));
  }

  // from source location record
  fixed_size_source_location_record_t(const source_location_record_t& source_location_record) {
    // start with clean slate
    memset(c, '\0', sizeof(fixed_size_source_location_record_t));

    // convert string to fixed-width byte[]
    std::string composite_value_string = source_location_record.composite_value();
    const char* record = (char*)composite_value_string.c_str();
    size_t count = composite_value_string.size();
    if (count > sizeof(fixed_size_source_location_record_t)) {
      count = sizeof(fixed_size_source_location_record_t);
    }
    memcpy(c, record, count);
  }

  // to string
  std::string to_string() const {
    return std::string(c, 200);
  }

  // comparison
  bool operator==(const class fixed_size_source_location_record_t r) const {
    return (strncmp(this->c, r.c, 200) == 0);
  }
  bool operator!=(const class fixed_size_source_location_record_t r) const {
    return (strncmp(this->c, r.c, 200) != 0);
  }
  bool operator<(const class fixed_size_source_location_record_t r) const {
    return (strncmp(this->c, r.c, 200) < 0);
  }
};

inline std::ostream& operator<<(std::ostream& os,
       const class source_location_record_t& source_location_record) {
  os << "(source_location_record composite_value='" << source_location_record.composite_value() << "')";
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
       const class fixed_size_source_location_record_t& fixed_size_source_location_record) {
  os << "(fixed_size_source_location_record composite_value='" << fixed_size_source_location_record.to_string() << "')";
  return os;
}

#endif

