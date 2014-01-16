// Author:  Joel Young <jdyoung@nps.edu>
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
 * Manage map stats.
 */

#ifndef MAP_STATS_HPP
#define MAP_STATS_HPP

#include "file_modes.h"
#include <string>
#include <sstream>
#include <cstdio>

// allow query stats
struct map_stats_t {
  std::string filename;
  file_mode_type_t file_mode;
  std::string data_type_name;
  size_t segment_size;
  size_t count_size;

  map_stats_t() : filename(""), file_mode(READ_ONLY),
                  data_type_name(""), segment_size(0), count_size(0) {
  }

  map_stats_t(std::string p_filename,
              file_mode_type_t p_file_mode,
              std::string p_data_type_name,
              size_t p_segment_size,
              size_t p_count_size) :
                          filename(p_filename),
                          file_mode(p_file_mode),
                          data_type_name(p_data_type_name),
                          segment_size(p_segment_size),
                          count_size(p_count_size) {
  }
};

inline std::ostream& operator<<(std::ostream& os,
        const class map_stats_t& s) {
  os << "(filename=" << s.filename
     << ", file_mode=" << file_mode_type_to_string(s.file_mode)
     << ", data type name=" << s.data_type_name
     << ", segment size=" << s.segment_size
     << ", count size=" << s.count_size
     << ")";
  return os;
}

/*
  void report_stats(dfxml_writer& x) const {
    x.push("map_stats");
    x.xmlout("filename", filename);
    x.xmlout("file_mode", file_mode_type_to_string(file_mode));
    x.xmlout("data_type_name", data_type_name);
    x.xmlout("segment_size",segment_size);
    x.xmlout("count_size",count_size);
    x.pop();
  }
*/

#endif

