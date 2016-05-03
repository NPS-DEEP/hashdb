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
 * Read E01,RAW, and general files, based on filename extension.
 *
 * Adapted heavily from bulk_extractor/src/image_process.cpp.
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
  // very simple iterator to walk offsets up to filesize.
  // dereferencing simply returns the itherator's current offset.
  class const_iterator {
    private:
    const uint64_t filesize;
    const size_t increment;
    uint64_t current_offset;
    public:
    const_iterator(const uint64_t p_filesize, const uint64_t p_increment, bool at_end) :
              filesize(p_filesize), increment(p_increment), current_offset(0) {
      if (at_end) {
        current_offset = filesize;
      }
    }
    bool operator==(const const_iterator& it) {
      return (filesize == it.filesize &&
              increment == it.increment &&
              current_offset == it.current_offset);
    }
    bool operator!=(const const_iterator& it) {
      return !((*this) != it);
    }
    const_iterator& operator++() {
      if (current_offset == filesize) {
        std::cerr << "Usage error: attempt to increment iterator after end\n";
      }
      current_offset += increment;
      if (current_offset > filesize) {
        current_offset = filesize;
      }
      return *this;
    }
    uint64_t operator*(const const_iterator& it) {
      return current_offset;
    }
  };

  const filename_t filename;
  std::string error_message;
  const file_reader_type_t file_reader_type;
  ewf_file_reader_t* ewf_file_reader;
  single_file_reader_t* single_file_reader;

  public:
  const bool is_open;
  const uint64_t filesize;
  uint8_t* const buffer;
  size_t const buffer_size;

  // set by read()
  bool has_buffer;
  uint64_t file_offset;
  size_t bytes_read;

  private:

  // do not allow copy or assignment
  file_reader_t(const file_reader_t&);
  file_reader_t& operator=(const file_reader_t&);

  // get reader type from fname extension
  file_reader_type_t reader_type(const filename_t& fname) {
    if (fname.size() < 4) {
      // no special fname extension
      return file_reader_type_t::SINGLE;
    }
    const filename_t last4 = fname.substr(fname.size() - 4);
    if (last4 == ".E01" || last4 == ".E01") {
      // E01
      return file_reader_type_t::E01;
    }
    if (last4 == ".000" || last4 == ".001") {
      // 001
      return file_reader_type_t::SERIAL;
    }
    if (fname.size() >= 8 && fname.substr(fname.size() - 8) ==
                                           "001.vmdk") {
      // 001.vdmk
      return file_reader_type_t::SERIAL;
    }
    // no special fname extension
    return file_reader_type_t::SINGLE;
  }

  // open the file for reading
  bool open_reader() {
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
   * Provide an allocated buffer of size hasher::buffer_size for this
   * reader to read into.
   * Check is_open.  If false, print error_message.
   * To read: read(offset).
   * To iterate through file: use const_iterator.
   * Use these const globals as desired: filename, filesize, file_reader_type.
   */
  file_reader_t(const filename_t& p_filename,
                uint8_t* const p_buffer,
                const size_t p_buffer_size) :
          filename(p_filename),
          error_message(""),
          file_reader_type(reader_type(filename)),
          ewf_file_reader(NULL),
          single_file_reader(NULL),
          is_open(open_reader()),
          filesize(get_filesize()),
          buffer(p_buffer),
          buffer_size(p_buffer_size),
          has_buffer(false),
          file_offset(0),
          bytes_read(0) {
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

  std::string read(int64_t offset) {
    if (is_open == false) {
      return "reader not open";
    }

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        error_message = ewf_file_reader->read(
                               offset, buffer, buffer_size, &bytes_read);
        return "";
        break;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        error_message = single_file_reader->read(
                               offset, buffer, buffer_size, &bytes_read);
        return "";
        break;
      }

      default: assert(0); std::exit(1);
    }
  }

  const_iterator begin() {
    return const_iterator(filesize, buffer_size, false);
  }
  const_iterator end() {
    return const_iterator(filesize, buffer_size, true);
  }

};

} // end namespace hasher

#endif

