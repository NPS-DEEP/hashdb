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
 * Calculate entropy from data.
 *
 * Adapted from bulk_extractor/scan_hashdb.cpp and bulk_extractor sbuf.
 */

#ifndef CALCULATE_BLOCK_LABEL_HPP
#define CALCULATE_BLOCK_LABEL_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <cctype> // isspace
#include <map> // isspace

namespace hasher {


// rules for determining if a block should be ignored
static bool ramp_trait(const uint8_t* const buffer, const size_t size) {
    uint32_t count = 0;
    for(size_t i=0; i+8 < size; i+= 4) {
        // note that little endian is detected and big endian is not detected
        uint32_t a = (uint32_t)(buffer[i+0]<<0) 
                   | (uint32_t)(buffer[i+1]<<8) 
                   | (uint32_t)(buffer[i+2]<<16) 
                   | (uint32_t)(buffer[i+3]<<24);
        uint32_t b = (uint32_t)(buffer[i+4]<<0) 
                   | (uint32_t)(buffer[i+5]<<8) 
                   | (uint32_t)(buffer[i+6]<<16) 
                   | (uint32_t)(buffer[i+7]<<24);
        if (a+1 == b) {
            count += 1;
        }
    }
    return count > size/8;
}

static bool hist_trait(const uint8_t* const buffer, const size_t size) {
    std::map<uint32_t,uint32_t> hist;
    for(size_t i=0; i+4 < size; i+= 4) {
        uint32_t a = (uint32_t)(buffer[i+3]<<0) 
                   | (uint32_t)(buffer[i+2]<<8) 
                   | (uint32_t)(buffer[i+1]<<16) 
                   | (uint32_t)(buffer[i+0]<<24);
        hist[a] += 1;
    }
    if (hist.size() < 3) return true;

    for (std::map<uint32_t,uint32_t>::const_iterator it = hist.begin();it != hist.end(); it++){
        if ((it->second) > size/16){
            return true;
        }
    }
    return false;
}

static bool whitespace_trait(const uint8_t* const buffer, const size_t size) {
    size_t count = 0;
    for(size_t i=0; i<size; i++){
        if (::isspace(buffer[i])) count+=1;
    }
    return count >= (size * 3)/4;
}

static bool monotonic_trait(const uint8_t* const buffer, const size_t size) {
    const double total = size / 4.0;
    int increasing = 0, decreasing = 0, same = 0;
    for (size_t i=0; i+8 < size; i += 4) {

        // note that little endian is detected and big endian is not detected
        uint32_t a = (uint32_t)(buffer[i+0]<<0) 
                   | (uint32_t)(buffer[i+1]<<8) 
                   | (uint32_t)(buffer[i+2]<<16) 
                   | (uint32_t)(buffer[i+3]<<24);
        uint32_t b = (uint32_t)(buffer[i+4]<<0) 
                   | (uint32_t)(buffer[i+5]<<8) 
                   | (uint32_t)(buffer[i+6]<<16) 
                   | (uint32_t)(buffer[i+7]<<24);


        if (b > a) {
            increasing++;
        } else if (b < a) {
            decreasing++;
        } else {
            same++;
        }
    }
    if (increasing / total >= 0.75) return true;
    if (decreasing / total >= 0.75) return true;
    if (same / total >= 0.75) return true;
    return false;
}


  static std::string calculate_block_label_private(
                     const uint8_t* const buffer, const size_t count) {
    // zzzzzzzzz
    std::stringstream ss_flags;
    if (ramp_trait(buffer, count))       ss_flags << "R";
    if (hist_trait(buffer, count))       ss_flags << "H";
    if (whitespace_trait(buffer, count)) ss_flags << "W";
    if (monotonic_trait(buffer, count))  ss_flags << "M";
    return ss_flags.str();
  }

  std::string calculate_block_label(uint8_t* const buffer,
                                    const size_t buffer_size,
                                    const size_t offset,
                                    const size_t count) {

    if (offset + count <= buffer_size) {
      // calculate when not a buffer overrun
      return calculate_block_label_private(buffer + offset, count);
    } else {
      // make new buffer from old but zero-extended
      uint8_t* b = new uint8_t[count];
      ::memcpy (b, buffer+offset, offset + count - buffer_size);
      std::string block_label = calculate_block_label_private(b, count);
      delete[] b;
      return block_label;
    }
  }
} // end namespace hasher

#endif
