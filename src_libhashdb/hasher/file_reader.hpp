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
 * Read E01, serial 001, and single files.
 *
 * Adapted from bulk_extractor/src/image_process.cpp.
 */


#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include "ewf_file_reader.hpp"
#include "single_file_reader.hpp"

namespace hasher {

enum file_reader_type_t {E01, SERIAL, SINGLE};

class file_reader_t {

  public:
  const std::string filename;
  std::string error_message;
  const file_reader_type_t file_reader_type;
  ewf_file_reader_t* ewf_file_reader;
  single_file_reader_t* single_file_reader;

  public:
  const bool is_open;
  const uint64_t filesize;

  private:
  // last read
  mutable size_t last_offset;
  mutable uint8_t* last_buffer;
  mutable size_t last_buffer_size;
  mutable size_t last_bytes_read;

  // do not allow copy or assignment
  file_reader_t(const file_reader_t&);
  file_reader_t& operator=(const file_reader_t&);

  // get reader type from native_filename extension
  file_reader_type_t reader_type(const filename_t& native_filename) {
    if (native_filename.size() < 4) {
      // no special native_filename extension
      return file_reader_type_t::SINGLE;
    }
    const filename_t last4 = native_filename.substr(native_filename.size() - 4);
    if (last4 == ".E01" || last4 == ".E01") {
      // E01
      return file_reader_type_t::E01;
    }
    if (last4 == ".000" || last4 == ".001") {
      // 001
      return file_reader_type_t::SERIAL;
    }
    if (native_filename.size() >= 8 &&
        native_filename.substr(native_filename.size() - 8) == "001.vmdk") {
      // 001.vdmk
      return file_reader_type_t::SERIAL;
    }
    // no special native_filename extension
    return file_reader_type_t::SINGLE;
  }

  std::string utf8_filename(const filename_t& native_string) {
#ifdef WIN32
// from http://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
// Convert a wide Unicode string to an UTF8 string
    if( native_string.empty() ) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &native_string[0], (int)native_string.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &native_string[0], (int)native_string.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    return native_string;
  }
#endif

  // open the file for reading
  bool open_reader(const filename_t& native_filename) {
    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        ewf_file_reader = new ewf_file_reader_t(filename);
        error_message = ewf_file_reader->error_message;
        return ewf_file_reader->is_open;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        single_file_reader = new single_file_reader_t(filename);
        error_message = single_file_reader->error_message;
        return single_file_reader->is_open;
      }
      default: assert(0); std::exit(1);
    }
  }

  uint64_t get_filesize() {
    if (!is_open) {
      return 0;
    }

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        return ewf_file_reader->filesize;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        return single_file_reader->filesize;
      }

      default: assert(0); std::exit(1);
    }
  }

  public:
  /**
   * Opens a file reader.  The reader detects file types.
   * Provide the filename or device name to read from.
   * Check is_open.  If false, print error_message.
   * To read: read(offset, buffer, buffer_size).
   * Use these const globals as desired: filename, filesize, file_reader_type.
   */
  file_reader_t(const filename_t& p_filename) :
          filename(utf8_filename(p_filename)),
          error_message(""),
          file_reader_type(reader_type(filename)),
          ewf_file_reader(NULL),
          single_file_reader(NULL),
          is_open(open_reader(p_filename)),
          filesize(get_filesize()),
          last_offset(0),
          last_buffer(NULL),
          last_buffer_size(0),
          last_bytes_read(0) {
  }

  // destructor closes any open resources
  ~file_reader_t() {

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        delete ewf_file_reader;
        break;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        delete single_file_reader;
        break;
      }

      default: assert(0); std::exit(1);
    }
  }

  std::string read(int64_t offset,
                   uint8_t* const buffer,
                   const size_t buffer_size,
                   size_t* const bytes_read) const {

    // reader must be in good state
    if (error_message != "") {
      std::cerr << "read requested when in bad state";
    }
    if (is_open == false) {
      return "reader not open";
    }

    // do not re-read same
    if (offset == last_offset &&
        buffer == last_buffer &&
        buffer_size == last_buffer_size) {
      *bytes_read = last_bytes_read;
      return "";
    }
    last_offset = offset;
    last_buffer = buffer;
    last_buffer_size = buffer_size;

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        std::string read_error_message = ewf_file_reader->read(
                               offset, buffer, buffer_size, bytes_read);
        last_bytes_read = *bytes_read;
        return read_error_message;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        std::string read_error_message = single_file_reader->read(
                               offset, buffer, buffer_size, bytes_read);
        last_bytes_read = *bytes_read;
        return read_error_message;
      }

      default: assert(0); std::exit(1);
    }
  }
};

} // end namespace hasher

#endif

