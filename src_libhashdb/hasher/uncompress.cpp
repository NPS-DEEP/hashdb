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

// Code adapted from bulk_extractor recursion scanners.

// NOTE: inline methods do not check for buffer overflow.  Do not call
//       them when near the end of data.

/**
 * \file
 * Provide uncompress routines for selected compression algorithms.
 */
#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include <cstring>
#include <sstream>
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <unistd.h>
#include <zlib.h>
#include "tprint.hpp" // threadsafe report unusual condition

namespace hasher {

  static const uint32_t zip_name_len_max = 1024;
  static const size_t uncompressed_size_min = 6;
  static const size_t uncompressed_size_max = 16777216; // 2^24 = 16MiB

  inline uint16_t u16(const uint8_t* const b) {
    return (uint16_t)(b[0]<<0) | (uint16_t)(b[1]<<8);
  }

  inline uint32_t u32(const uint8_t* const b) {
    return (uint32_t)(b[0]<<0) | (uint32_t)(b[1]<<8) |
           (uint32_t)(b[2]<<16) | (uint32_t)(b[3]<<24);
  }

  // always returns new out_buf, even if empty
  std::string new_from_zip(const uint8_t* const in_buf,
                           const size_t in_size,
                           const size_t in_offset,
                           uint8_t** out_buf,
                           size_t* out_size) {

    // pointer to the output buffer that will be created using new
    *out_buf = NULL;
    *out_size = 0;

    // validate the buffer range
    if (in_size < in_offset + 30) {
      // nothing to do
      *out_buf = new uint8_t[0]();
      return "zip region too small";
    }

    const uint8_t* const b = in_buf + in_offset;

    const uint32_t compr_size=u32(b+18);
    const uint32_t uncompr_size=u32(b+22);
    const uint16_t name_len=u16(b+26);
    const uint16_t extra_field_len=u16(b+28);

    // validate name length
    if (name_len == 0 || name_len > zip_name_len_max) {
      *out_buf = new uint8_t[0]();
      return "invalid zip metadata";
    }

    // calculate offset to compressed data
    uint32_t compressed_offset = in_offset + 30 + name_len + extra_field_len;

    // offset must be inside the buffer
    if (compressed_offset >= in_size) {
      *out_buf = new uint8_t[0]();
      return "zip read request outside data range";
    }

    // size of compressed data
    const uint32_t compressed_size = (compr_size == 0 ||
               compressed_offset + compr_size > in_size) 
                          ? in_size - compressed_offset : compr_size;

    // size of uncompressed data
    const uint32_t potential_uncompressed_size =
               (compr_size == 0 || compr_size > uncompressed_size_max)
                                  ? uncompressed_size_max : uncompr_size;
    
    // skip if uncompressed size is too small
    if (potential_uncompressed_size < uncompressed_size_min) {
      *out_buf = new uint8_t[0]();
      return "zip uncompress size too small";
    }

    // create the uncompressed buffer
    *out_buf = new (std::nothrow) uint8_t[potential_uncompressed_size]();
    if (*out_buf == NULL) {
      // comment that the buffer acquisition request failed
      hashdb::tprint("# bad memory allocation in zip uncompression");
      return "bad memory allocation in zip uncompression";
    }

    // set up zlib data
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(
                                         in_buf + compressed_offset));
    zs.avail_in = compressed_size;
    zs.next_out = *out_buf;
    zs.avail_out = potential_uncompressed_size;

    // initialize zlib for this decompression
    int r = inflateInit2(&zs, -15);
    if (r == 0) {

      // inflate
      inflate(&zs, Z_SYNC_FLUSH);

      // set out_size
      *out_size = zs.total_out;

      // close zlib
      inflateEnd(&zs);
      return "";

    } else {

      // comment that zlib inflate failed
      return "zlib inflate failed";
    }
  }
} // end namespace hasher

