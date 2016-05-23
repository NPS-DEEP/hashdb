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
 * Calculate entropy from data.
 * This algorithm is loosely adapted from sdhash/sdbf/entr64.cc.
 *
 * The entropy is calculated over one block of size block_size.
 *
 * The entropy is calculated for 16-bit alphabet elements.
 */

#ifndef ENTROPY_CALCULATOR_HPP
#define ENTROPY_CALCULATOR_HPP
#include <tgmath.h>
#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <map>

namespace hasher {

class entropy_calculator_t {

  private:
  typedef std::map<size_t, size_t>::const_iterator bucket_it_t;
  uint64_t* const lookup_table;

  // do not allow copy or assignment
  entropy_calculator_t(const entropy_calculator_t&);
  entropy_calculator_t& operator=(const entropy_calculator_t&);

  uint64_t calculate_private(const uint8_t* const buffer, const size_t count) {

    // calculate entropy buckets
    std::map<size_t, size_t> buckets;
    for (size_t i=0; i<count-1; i+=2) {
      uint16_t element = (uint16_t)(buffer[i+0]<<0 | buffer[i+1]<<8);
      buckets[element] += 1;
    }

    // sum the entropy from the buckets
    uint64_t entropy = 0;
    for (bucket_it_t it = buckets.begin(); it != buckets.end(); ++it) {
      entropy += lookup_table[it->second - 1];
    }

    return entropy;
  }

  public:
  entropy_calculator_t(const size_t block_size) :
                   lookup_table(new uint64_t[block_size]) {

    // compute entropy values for each slot
    for (size_t i=0; i< block_size; ++i) {
      double p = (i+1.0)/block_size;
      lookup_table[i] = ((-p * (log(p)/log(2.0))/6) *(1<<10));
    }
  }

  ~entropy_calculator_t(){
    delete[] lookup_table;
  }

  // safely calculate block entropy by padding with zeros on overflow.
  uint64_t calculate(const uint8_t* const buffer,
                     const size_t buffer_size,
                     const size_t offset,
                     const size_t count) {

    if (offset + count <= buffer_size) {
      // calculate when not a buffer overrun
      return calculate_private(buffer + offset, count);
    } else {
      // make new buffer from old but zero-extended
      uint8_t* b = new uint8_t[count]();
      ::memcpy (b, buffer+offset, offset + count - buffer_size);
      size_t entropy = calculate_private(b, count);
      delete[] b;
      return entropy;
    }
  }
};

} // end namespace hasher

#endif
