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
 * Provide support for LMDB operations.
 *
 * Note: it would be nice if MDB_val had a const type and a non-const type
 * to handle reading vs. writing.  Instead, we hope the callee works right.
 */

#include <config.h>
#include <cassert>
#include <stdint.h>
#include <cstring>

namespace hdb {

  // write value into encoding, return pointer past value written.
  // each write will add no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417
  uint8_t* encode_uint64_t(uint64_t value, uint8_t* target) {

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32_t part0 = static_cast<uint32_t>(value      );
    uint32_t part1 = static_cast<uint32_t>(value >> 28);
    uint32_t part2 = static_cast<uint32_t>(value >> 56);

    int size;

    // hardcoded binary search tree...
    if (part2 == 0) {
      if (part1 == 0) {
        if (part0 < (1 << 14)) {
          if (part0 < (1 << 7)) {
            size = 1; goto size1;
          } else {
            size = 2; goto size2;
          }
        } else {
          if (part0 < (1 << 21)) {
            size = 3; goto size3;
          } else {
            size = 4; goto size4;
          }
        }
      } else {
        if (part1 < (1 << 14)) {
          if (part1 < (1 << 7)) {
            size = 5; goto size5;
          } else {
            size = 6; goto size6;
          }
        } else {
          if (part1 < (1 << 21)) {
            size = 7; goto size7;
          } else {
            size = 8; goto size8;
          }
        }
      }
    } else {
      if (part2 < (1 << 7)) {
        size = 9; goto size9;
      } else {
        size = 10; goto size10;
      }
    }

    // bad if here
    assert(0);

    size10: target[9] = static_cast<uint8_t>((part2 >>  7) | 0x80);
    size9 : target[8] = static_cast<uint8_t>((part2      ) | 0x80);
    size8 : target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
    size7 : target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
    size6 : target[5] = static_cast<uint8_t>((part1 >>  7) | 0x80);
    size5 : target[4] = static_cast<uint8_t>((part1      ) | 0x80);
    size4 : target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
    size3 : target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
    size2 : target[1] = static_cast<uint8_t>((part0 >>  7) | 0x80);
    size1 : target[0] = static_cast<uint8_t>((part0      ) | 0x80);

    target[size-1] &= 0x7F;

    return target + size;
  }

  // read pointer into value, return pointer past value read.
  // each read will consume no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417
  const uint8_t* decode_uint64_t(const uint8_t* p_ptr, uint64_t& value) {

    const uint8_t* ptr = p_ptr;
    uint32_t b;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32_t part0 = 0, part1 = 0, part2 = 0;

    b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

    // We have overrun the maximum size of a varint (10 bytes).  The data
    // must be corrupt.
    std::cerr << "corrupted uint64 protocol buffer\n";
    assert(0);

   done:
    value = (static_cast<uint64_t>(part0)      ) |
            (static_cast<uint64_t>(part1) << 28) |
            (static_cast<uint64_t>(part2) << 56);
    return ptr;
  }
}

