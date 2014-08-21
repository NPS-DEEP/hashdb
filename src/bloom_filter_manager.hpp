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

#ifndef BLOOM_FILTER_MANAGER_HPP
#define BLOOM_FILTER_MANAGER_HPP
#include "bloom.h"
#include "file_modes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>

template<class T>
class bloom_filter_manager_t {
  public:
  const std::string filename1;
  const std::string filename2;
  const file_mode_type_t file_mode;
  const bool bloom1_is_used;
  const uint32_t bloom1_M_hash_size; // number of bloom function bits, e.g. 28
  const uint32_t bloom1_k_hash_functions; // number of hash functions, e.g. 2
  const bool bloom2_is_used;
  const uint32_t bloom2_M_hash_size;
  const uint32_t bloom2_k_hash_functions;

  private:
  NSRLBloom bloom1;
  NSRLBloom bloom2;

  // disallow these
  bloom_filter_manager_t(const bloom_filter_manager_t&);
  bloom_filter_manager_t& operator=(const bloom_filter_manager_t&);

  void open_bloom(NSRLBloom& bloom,
                  std::string filename,
                  bool is_used,
                  uint32_t M_hash_size,
                  uint32_t k_hash_functions) {
    if (is_used) {
      int success = 0;
      // open based on file mode
      switch(file_mode) {
      case READ_ONLY:
        success = bloom.open(filename.c_str(), MAP_READ_ONLY);

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
                     sizeof(T) * 8,
                     M_hash_size,
                     k_hash_functions,
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
        success = bloom.open(filename.c_str(), MAP_READ_AND_WRITE);

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

  public:
  bloom_filter_manager_t (const std::string& p_hashdb_dir,
                          file_mode_type_t p_file_mode,
                          bool p_bloom1_is_used,
                          uint32_t p_bloom1_M_hash_size,
                          uint32_t p_bloom1_k_hash_functions,
                          bool p_bloom2_is_used,
                          uint32_t p_bloom2_M_hash_size,
                          uint32_t p_bloom2_k_hash_functions) :
          filename1(p_hashdb_dir + "/bloom_filter_1"),
          filename2(p_hashdb_dir + "/bloom_filter_2"),
          file_mode(p_file_mode),

          bloom1_is_used(p_bloom1_is_used),
          bloom1_M_hash_size(p_bloom1_M_hash_size),
          bloom1_k_hash_functions(p_bloom1_k_hash_functions),
          bloom2_is_used(p_bloom2_is_used),
          bloom2_M_hash_size(p_bloom2_M_hash_size),
          bloom2_k_hash_functions(p_bloom2_k_hash_functions),
          bloom1(),
          bloom2() {

    open_bloom(bloom1, filename1, bloom1_is_used,
               bloom1_M_hash_size, bloom1_k_hash_functions);
    open_bloom(bloom2, filename2, bloom2_is_used,
               bloom2_M_hash_size, bloom2_k_hash_functions);
  }

  void add_hash_value(const T& key) {
//std::cerr << "bloom_filter add_hash_value.a " << key << " " << filename << std::endl;
    if (bloom1_is_used) {
      bloom1.add(key.digest);
    }
    if (bloom2_is_used) {
      bloom2.add(key.digest);
    }
//std::cerr << "bloom_filter add_hash_value.b" << std::endl;
  }

  bool is_positive(const T& key) const {
//std::cerr << "bloom_filter is_positive.a " << key << " " << bloom.query(key.digest) << " " << filename << std::endl;
    if (bloom1_is_used && !bloom1.query(key.digest)) {
      // not in bloom1
//std::cerr << "bloom_filter is_positive.b" << std::endl;
      return false;
    }
    if (bloom2_is_used && !bloom2.query(key.digest)) {
      // not in bloom2
//std::cerr << "bloom_filter is_positive.b" << std::endl;
      return false;
    }

    // At this point, either it is present in both or neither filter are used.
    // Either way, we must indicate the potential positive.
    return true;
  }

  /**
   * Approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
   */
  static uint64_t approximate_M_to_n(uint32_t M) {
    uint64_t m = (uint64_t)1<<M;
    uint64_t n = m * 0.17;
  //std::cout << "Bloom filter conversion: for M=" << M << " use n=" << n << "\n";
    return n;
  }

  /**
   * approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
   */
  static uint32_t approximate_n_to_M(uint64_t n) {
    uint64_t m = n / 0.17;
    uint32_t M = 1;
    // fix with actual math formula, but this works
    while ((m = m/2) > 0) {
      M++;
    }
  //std::cout << "Bloom filter conversion: for n=" << n << " use M=" << M << "\n";
    return M;
  }

  /**
   * throw std::runtime_error if invalid.
   */
  static void validate_bloom_settings(hashdb_settings_t hashdb_settings) {
    std::ostringstream ss;

    // check that bloom hash size is not too loarge for the running system
    uint32_t max_M_hash_size = (sizeof(size_t) * 8) -1;
    if (hashdb_settings.bloom1_M_hash_size > max_M_hash_size) {
      ss << "bloom1 bits per hash, "
         << hashdb_settings.bloom1_M_hash_size
         << ", exceeds " << max_M_hash_size
         << ", which is the limit on this system";
      throw std::runtime_error(ss.str());
    }
    if (hashdb_settings.bloom2_M_hash_size > max_M_hash_size) {
      ss << "bloom1 bits per hash, "
         << hashdb_settings.bloom2_M_hash_size
         << ", exceeds " << max_M_hash_size
         << ", which is the limit on this system";
      throw std::runtime_error(ss.str());
    }

    // check that bloom hash size is not too small
    uint32_t min_M_hash_size = 3;
    if (hashdb_settings.bloom1_M_hash_size < min_M_hash_size) {
      ss << "bloom1 bits per hash, "
         << hashdb_settings.bloom1_M_hash_size
         << ", must not be less than " << min_M_hash_size;
      throw std::runtime_error(ss.str());
    }
    if (hashdb_settings.bloom2_M_hash_size < min_M_hash_size) {
      ss << "bloom2 bits per hash, "
         << hashdb_settings.bloom2_M_hash_size
         << ", must not be less than " << min_M_hash_size;
      throw std::runtime_error(ss.str());
    }

    // check that the number of hash functions, k hash functions, is reasonable
    if (hashdb_settings.bloom1_k_hash_functions < 1
     || hashdb_settings.bloom1_k_hash_functions > 5) {
      std::cerr << "bloom1 k hash functions, "
                << hashdb_settings.bloom1_k_hash_functions
                << ", must be between 1 and 5\n";
      throw std::runtime_error(ss.str());
    }
    if (hashdb_settings.bloom2_k_hash_functions < 1
     || hashdb_settings.bloom2_k_hash_functions > 5) {
      std::cerr << "bloom2 k hash functions, "
                << hashdb_settings.bloom2_k_hash_functions
                << ", must be between 2 and 5\n";
      throw std::runtime_error(ss.str());
    }
  }
};

#endif

