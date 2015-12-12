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
 *
 * To have consistent hashes, all hashes are forced to 16 bytes, zero-extended.
 */

#ifndef BLOOM_FILTER_MANAGER_HPP
#define BLOOM_FILTER_MANAGER_HPP
#include "bloom.h"
#include "file_modes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>

class bloom_filter_manager_t {
  private:
  const std::string filename;
  const file_mode_type_t file_mode;
  const bool bloom_is_used;
  const uint32_t bloom_M_hash_size; // number of bloom function bits, e.g. 28
  const uint32_t bloom_k_hash_functions; // number of hash functions, e.g. 2
  NSRLBloom bloom;

  // disallow these
  bloom_filter_manager_t(const bloom_filter_manager_t&);
  bloom_filter_manager_t& operator=(const bloom_filter_manager_t&);

  void open_bloom(NSRLBloom& bloom_filter,
                  std::string bloom_filename,
                  bool is_used,
                  uint32_t M_hash_size,
                  uint32_t k_hash_functions) {
    if (is_used) {
      int success = 0;
      // open based on file mode
      switch(file_mode) {
      case READ_ONLY:
        success = bloom_filter.open(bloom_filename.c_str(), MAP_READ_ONLY);

        // validate
        if (success != 0) {
          std::cerr << "Unable to open Bloom filter file '" << bloom_filename
                    << "' for reading.\n";
          std::cerr << strerror(errno) << "\n";
          std::cerr << "Cannot continue.\n";
          exit(1);
        }
        break;
      case RW_NEW:
        success = bloom_filter.create(bloom_filename.c_str(),
                     16 * 8,  // expected size of binary hash
                     M_hash_size,
                     k_hash_functions,
                     "no message");

        // validate
        if (success != 0) {
          std::cerr << "Unable to open new Bloom filter file '" << bloom_filename
                    << "'.\n";
          std::cerr << strerror(errno) << "\n";
          std::cerr << "Cannot continue.\n";
          exit(1);
        }
        break;
      case RW_MODIFY:
        success = bloom_filter.open(bloom_filename.c_str(), MAP_READ_AND_WRITE);

        // validate
        if (success != 0) {
          std::cerr << "Unable to open Bloom filter file '" << bloom_filename
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
                          bool p_bloom_is_used,
                          uint32_t p_bloom_M_hash_size,
                          uint32_t p_bloom_k_hash_functions) :
          filename(p_hashdb_dir + "/bloom_filter"),
          file_mode(p_file_mode),
          bloom_is_used(p_bloom_is_used),
          bloom_M_hash_size(p_bloom_M_hash_size),
          bloom_k_hash_functions(p_bloom_k_hash_functions),
          bloom() {

    open_bloom(bloom, filename, bloom_is_used,
               bloom_M_hash_size, bloom_k_hash_functions);
  }

  // a bloom hash is 16 bytes long with unused bytes zeroed out
  std::string to_bloom_hash(const std::string& binary_hash) const {
    size_t count = binary_hash.size();

    // force hash count to 16 for Bloom
    if (count > 16) {
      count = 16;
    }

    // zero-extend short hashes
    uint8_t extended[16];
    memset(extended, 0, 16);
    memcpy(extended, binary_hash.c_str(), count);
    return std::string((char*)extended, 16);
  }

  void add_hash_value(const std::string& binary_hash) {
    if (bloom_is_used) {
      std::string bloom_hash = to_bloom_hash(binary_hash);
//std::cout << "add_hash_value " << lmdb_helper::binary_hash_to_hex(bloom_hash) << "\n";
      bloom.add(reinterpret_cast<const uint8_t*>(bloom_hash.c_str()));
    }
  }

  /**
   * True if found or if filter is disabled.
   */
  bool is_positive(const std::string& binary_hash) const {
    if (bloom_is_used) {
      std::string bloom_hash = to_bloom_hash(binary_hash);
//std::cout << "is_positive " << lmdb_helper::binary_hash_to_hex(bloom_hash) << "\n";
      return (bloom.query(reinterpret_cast<const uint8_t*>(bloom_hash.c_str())));
    }

    // At this point, either it is present in both or filter is not used.
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
   * Check, abort if invalid.
   */
  static void validate_bloom_settings(
            const bool bloom_is_used, // not used
            const uint32_t bloom_M_hash_size,
            const uint32_t bloom_k_hash_functions) {

    std::ostringstream ss;

    // check that bloom hash size is not too loarge for the running system
    uint32_t max_M_hash_size = (sizeof(size_t) * 8) -1;
    if (bloom_M_hash_size > max_M_hash_size) {
      ss << "bloom bits per hash, "
         << bloom_M_hash_size
         << ", exceeds " << max_M_hash_size
         << ", which is the limit on this system.  Please retune."
         << "\nAborting.\n";
      exit(1);
    }

    // check that bloom hash size is not too small
    uint32_t min_M_hash_size = 3;
    if (bloom_M_hash_size < min_M_hash_size) {
      ss << "bloom bits per hash, "
         << bloom_M_hash_size
         << ", must not be less than " << min_M_hash_size
         << ".  Please retune.\nAborting.";
      exit(1);
    }

    // check that the number of hash functions, k hash functions, is reasonable
    if (bloom_k_hash_functions < 1
     || bloom_k_hash_functions > 5) {
      std::cerr << "bloom k hash functions, "
                << bloom_k_hash_functions
                << ", must be between 1 and 5.  Please retune."
                << "\nAborting.";
      exit(1);
    }
  }
};

#endif

