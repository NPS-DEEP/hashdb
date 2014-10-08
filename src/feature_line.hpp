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
 * Holds the fields of a feature line
 */

#ifndef FEATURE_LINE_HPP
#define FEATURE_LINE_HPP

#include <cstring>

struct feature_line_t {
  std::string forensic_path;
  std::string feature;
  std::string context;

  feature_line_t(std::string p_forensic_path, 
                 std::string p_feature, 
                 std::string p_context) :
                                forensic_path(p_forensic_path),
                                feature(p_feature), 
                                context(p_context) {
  }
/*
  feature_line_t& operator=(const feature_line_t& other) {
    forensic_path = other.forensic_path;
    feature = other.feature;
    context = other.context;
  }
*/
};

#endif

