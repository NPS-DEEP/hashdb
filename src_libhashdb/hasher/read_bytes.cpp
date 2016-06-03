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
 * Read raw bytes from a media image.
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

#include <string>
#include <cassert>
#include <iostream>
#include <unistd.h> // for F_OK
#include <sstream>
#include "num_cpus.hpp"
#include "hashdb.hpp"
#include "file_reader.hpp"
#include "hash_calculator.hpp"
#include "threadpool.hpp"
#include "job.hpp"
#include "job_queue.hpp"
#include "scan_tracker.hpp"
#include "uncompress.hpp" // for new_from_zip

namespace hashdb {

  // convenience function to
  // read bytes from image starting at offset.
  std::string read_bytes(const std::string& image_filename,
                         const uint64_t offset,
                         const uint64_t count,
                         std::string& bytes) {
    std::stringstream ss;
    ss << offset;
    return read_bytes(image_filename, ss.str(), count, bytes);
  }

  // read count bytes at forensic path in image.
  // Two example paths are 1000 and 1000-zip-0.
  // Return "" and reason on failure.
  std::string read_bytes(const std::string& image_filename,
                         const std::string& forensic_path,
                         const uint64_t count,
                         std::string& bytes) {

    // split forensic path into array of parts
    std::vector<std::string> parts;
    std::stringstream ss(forensic_path);
    std::string part;
    while (std::getline(ss, part, '-')) {
      parts.push_back(part);
    }

    // get file offset into in_offset
    std::vector<std::string>::const_iterator it = parts.begin();
    if (it == parts.end()) {
      return "invalid forensic path, image offset expected";
    }
    uint64_t image_offset = 0;
    std::istringstream iss(*it);
    iss >> image_offset;

    // open the file reader
    const hasher::file_reader_t file_reader(hasher::utf8_to_native(
                                                           image_filename));
    if (file_reader.error_message.size() > 0) {
      // the file failed to open
      return file_reader.error_message;
    }

    // create a buffer to read into, allow 1MB
    size_t from_size = 1048576; // 1MiB = 2^20
    uint8_t* from_buf = new (std::nothrow) uint8_t[from_size]();
    if (from_buf == NULL) {
      // abort
      return "bad memory allocation";
    }

    // read into from_buf
    const std::string read_error_message =
            file_reader.read(image_offset, from_buf, from_size, &from_size);
    if (read_error_message != "") {
      delete[] from_buf;
      return read_error_message;
    }

    // now recursively read down the forensic path
    size_t from_offset = 0;
    while (++it != parts.end()) {

      // get compression type
      std::string compression_type = *it;

      // read into new to_buf
      uint8_t* to_buf = NULL;
      size_t to_size = 0;
      if (compression_type == "zip") {
        std::string error_message = hasher::new_from_zip(
                                          from_buf, from_size, from_offset,
                                          &to_buf, &to_size);
        if (error_message != "") {
          // error in zip decompression
          delete[] from_buf;
          return error_message;
        }
      } else {
        // unrecognized compression type
        delete[] from_buf;
        return "invalid forensic path, compression type expected";
      }

      // get from_offset
      if (++it == parts.end()) {
        // missing offset
        delete[] from_buf;
        delete[] to_buf;
        return "invalid forensic path, compression offset expected";
      }
      from_offset = ::atol(it->c_str());

      // done with from_buf
      delete[] from_buf;

      // move new to_buf into working from_buf
      from_buf = to_buf;
      from_size = to_size;
    }

    // get bytes from range
    if (from_offset + count <= from_size) {
      // range is within buffer
      bytes = std::string(reinterpret_cast<const char*>(
                                    from_buf + from_offset), count);
    } else if (from_offset > from_size) {
      // range starts outside buffer
      bytes = "";
    } else {
      // range exceeds buffer limit
      bytes = std::string(reinterpret_cast<const char*>(
                        from_buf + from_offset), from_size - from_offset);
    }

    // done
    delete[] from_buf;
    return "";
  }
} // end namespace hashdb

