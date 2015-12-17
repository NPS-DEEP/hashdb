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
 * Create a new hashdb.
 */

#ifndef BLOOM_HELPER_HPP
#define BLOOM_HELPER_HPP

#include <string>
#include <sstream>
#include <stdint.h>

/**
 * Approximate bloom conversions for k=3 and p false positive = ~ 1.1%
 * to 6.4%.
 */
uint64_t bloom_M_to_n(uint32_t M) {
  uint64_t m = (uint64_t)1<<M;
  uint64_t n = m * 0.17;
  return n;
}

/**
 * Approximate bloom conversions for k=3 and p false positive = ~ 1.1%
 * to 6.4%.
 */
uint32_t bloom_n_to_M(uint64_t n) {
  uint64_t m = n / 0.17;
  uint32_t M = 1;
  // fix with actual math formula, but this works
  while ((m = m/2) > 0) {
    M++;
  }
  return M;
}

/**
 * Check Bloom settings, return true and "" else false and reason.
 */
std::pair<bool, std::string> check_bloom_settings(
          const bool bloom_is_used,
          const uint32_t bloom_M_hash_size,
          const uint32_t bloom_k_hash_functions) {

  if (!bloom_is_used) {
    return std::pair<bool, std::string>(true, "");
  }

  std::ostringstream ss;

  // check that bloom hash size is not too loarge for the running system
  uint32_t max_M_hash_size = (sizeof(size_t) * 8) -1;
  if (bloom_M_hash_size > max_M_hash_size) {
    ss << "Invalid Bloom filter settings: bloom bits per hash, "
       << bloom_M_hash_size
       << ", exceeds " << max_M_hash_size
       << ", which is the limit on this system.";
    return std::pair<bool, std::string>(false, ss.str());
  }

  // check that bloom hash size is not too small
  uint32_t min_M_hash_size = 3;
  if (bloom_M_hash_size < min_M_hash_size) {
    ss << "Invalid Bloom filter settings: bloom bits per hash, "
       << bloom_M_hash_size
       << ", must not be less than " << min_M_hash_size << ".";
    return std::pair<bool, std::string>(false, ss.str());
  }

  // check that the number of hash functions, k hash functions, is reasonable
  if (bloom_k_hash_functions < 1
   || bloom_k_hash_functions > 5) {
    ss << "Invalid Bloom filter settings: bloom k hash functions, "
              << bloom_k_hash_functions
              << ", must be between 1 and 5.";
    return std::pair<bool, std::string>(false, ss.str());
  }
  return std::pair<bool, std::string>(true, "");
}

#endif

