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
 * Calculate optimum settings based on expected size.
 */

#ifndef BLOOM_FILTER_CALCULATOR_HPP
#define BLOOM_FILTER_CALCULATOR_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>

namespace bloom_filter_calculator {

// approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
uint64_t approximate_M_to_n(uint32_t M) {
  uint64_t m = (uint64_t)1<<M;
  uint64_t n = m * 0.17;
//std::cout << "Bloom filter conversion: for M=" << M << " use n=" << n << "\n";
  return n;
}

// approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
uint32_t approximate_n_to_M(uint64_t n) {
  uint64_t m = n / 0.17;
  uint32_t M = 1;
  // fix with actual math formula, but this works
  while ((m = m/2) > 0) {
    M++;
  }
//std::cout << "Bloom filter conversion: for n=" << n << " use M=" << M << "\n";
  return M;
}

} // namespace

#endif

