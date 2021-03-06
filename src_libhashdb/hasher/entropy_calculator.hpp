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
 *
 * The entropy returned is calculated entropy * 1,000 rounded into an integer.
 */

#ifndef ENTROPY_CALCULATOR_HPP
#define ENTROPY_CALCULATOR_HPP
#include <tgmath.h>
#include <stdint.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <map>

namespace hasher {

class entropy_calculator_t {

  private:
  typedef std::map<size_t, size_t>::const_iterator bucket_it_t;
  const size_t slots;
  float* const lookup_table; // index 0 is not used

  // do not allow copy or assignment
  entropy_calculator_t(const entropy_calculator_t&);
  entropy_calculator_t& operator=(const entropy_calculator_t&);

  uint64_t calculate_private(const uint8_t* const buffer) const {

    // calculate entropy buckets
    std::map<size_t, size_t> buckets;
    for (size_t i=0; i<slots; i++) {
      uint16_t element = (uint16_t)(buffer[i*2+0]<<0 | buffer[i*2+1]<<8);
      buckets[element] += 1;
    }

    // sum the entropy from the buckets
    float entropy = 0;
    for (bucket_it_t it = buckets.begin(); it != buckets.end(); ++it) {
      entropy += lookup_table[it->second];
    }

    return round(entropy * 1000);
  }

  public:
  entropy_calculator_t(const size_t block_size) :
                   slots(block_size / 2),
                   lookup_table(new float[slots+1]) {

    // compute entropy values for each slot
    for (size_t i=1; i<= slots; ++i) {
      float p = (float)i/slots;
      lookup_table[i] = -p * (log2f(p));
    }
  }

  ~entropy_calculator_t(){
    delete[] lookup_table;
  }

  // safely calculate block entropy by padding with zeros on overflow.
  // Returns entropy * 1,000 as an int for 3 decimal precision.
  uint64_t calculate(const uint8_t* const buffer,
                  const size_t buffer_size,
                  const size_t offset) const {

    if (offset + slots * 2 <= buffer_size) {
      // calculate when not a buffer overrun
      return calculate_private(buffer + offset);
    } else if (offset > buffer_size) {
      // program error
      assert(0);
      return 0; // for mingw
    } else {
      // make new buffer from old but zero-extended
      uint8_t* b = new uint8_t[slots*2]();
      ::memcpy (b, buffer+offset, buffer_size - offset);
      float entropy = calculate_private(b);
      delete[] b;
      return round(entropy * 1000);
    }
  }
};

} // end namespace hasher

#endif
