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

// Based on GZIP file format specification v4.3,
// http://www.zlib.org/rfc-gzip.html

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

  // return new out_buf else error text
  std::string new_from_gzip(const uint8_t* const in_buf,
                            const size_t in_size,
                            const size_t in_offset,
                            uint8_t** out_buf,
                            size_t* out_size) {

    const size_t max_out_size = 256*1024*1024;

    // pointer to the output buffer that will be created using new
    *out_buf = NULL;
    *out_size = 0;

    // validate the buffer range
    if (in_size < in_offset + 18) {
      // nothing to do
      *out_buf = new uint8_t[0]();
      return "gzip region too small";
    }

    // create the destination uncompressed buffer
    *out_buf = new (std::nothrow) uint8_t[max_out_size]();
    if (*out_buf == NULL) {
      // comment that the buffer acquisition request failed
      hashdb::tprint(std::cout,
                     "# bad memory allocation in gzip uncompression");
      return "bad memory allocation in gzip uncompression";
    }

    // calculate maximum input size
    const size_t max_in_size = in_size - in_offset;

    // set up the decompression structure
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(
                                                        in_buf + in_offset));
    zs.avail_in = max_in_size;
    zs.next_out = *out_buf;
    zs.avail_out = max_out_size;

    int r = inflateInit2(&zs,16+MAX_WBITS);
    if(r==0){
      // ignore any error code and accept total_out
      r = inflate(&zs,Z_SYNC_FLUSH);
      *out_size = zs.total_out;
      r = inflateEnd(&zs);
      return "";
    } else {

      // inflate failed
      delete[] *out_buf;
      *out_buf = NULL;
      return "gzip zlib inflate failed";
    }
  }
} // end namespace hasher

