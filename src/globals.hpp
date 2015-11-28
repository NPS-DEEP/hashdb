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

#include <stdint.h>
#include <string>

/**
 * Hold global variables in this globals namespace.
 */
class globals_t {
  public:
  static const uint32_t hashdb_settings_version = 2;
  static const uint32_t default_sector_size = 512;
  static const uint32_t default_hash_truncation = 0;
  static const uint32_t default_hash_block_size = 512;
  static const uint32_t default_scan_expanded_max = 200;
  static const uint32_t default_maximum_hash_duplicates = 0;
  static const bool default_bloom_is_used = true;
  static const uint32_t default_bloom_M_hash_size = 28;
  static const uint32_t default_bloom_k_hash_functions = 3;
  static const uint32_t default_expand_identified_blocks_max = 200;
  static const uint32_t default_explain_identified_blocks_max = 20;
  static const uint32_t default_import_tab_sector_size = 512;
  static bool quiet_mode;
  static std::string command_line_string;
};

#endif

