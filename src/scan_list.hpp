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
 * Scan for hashes in file where lines are "<forensic path><tab><hex hash>".
 * Comment lines are forwarded to output.
 */

#ifndef SCAN_LIST_HPP
#define SCAN_LIST_HPP

#include <iostream>
#include "../src_libhashdb/hashdb.hpp"

void scan_list(hashdb::scan_manager_t& manager, std::istream& in,
               const hashdb::scan_mode_t scan_mode);

#endif
