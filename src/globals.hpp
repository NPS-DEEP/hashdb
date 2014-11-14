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
 * Hold global variables in this globals namespace.
 */

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "boost/btree/helpers.hpp" // for btree flags bitmask

/**
 * Hold global variables in this globals namespace.
 */
class globals_t {
  public:
  static const uint32_t default_scan_expanded_max = 200;
  static const uint32_t default_explain_identified_blocks_max = 20;
  static const uint32_t default_import_tab_sector_size = 512;
  static bool quiet_mode;
  static boost::btree::flags::bitmask btree_flags;
};

#endif

