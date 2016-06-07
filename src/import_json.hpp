// Author:  Bruce Allen
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
 * Import from data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
 */

#ifndef IMPORT_JSON_HPP
#define IMPORT_JSON_HPP

#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"
#include <fstream>

void import_json(hashdb::import_manager_t& manager,
                 progress_tracker_t& progress_tracker,
                 std::istream& in);

#endif

