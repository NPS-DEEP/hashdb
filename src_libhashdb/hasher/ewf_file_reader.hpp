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
 * Read E01 file
 *
 * Adapted heavily from bulk_extractor/src/image_process.cpp.
 */


#ifndef EWF_FILE_READER_HPP
#define EWF_FILE_READER_HPP

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <string.h> // strlen
#include <vector>
#include <set>
#include <cassert>
#include <libewf.h>
#ifdef WIN32
#include <cwchar>
#endif

namespace hasher {

class ewf_file_reader_t {

  public:
  const filename_t filename;
  std::string error_message;

  private:
  // E01 data
  libewf_handle_t* libewf_handle;

  public:
  const bool is_open;
  const uint64_t filesize;

  private:
  // do not allow copy or assignment
  ewf_file_reader_t(const ewf_file_reader_t&);
  ewf_file_reader_t& operator=(const ewf_file_reader_t&);

  // read EWF error and release EWF error resource
  std::string consume_libewf_error(libewf_error_t* error) const {
    char e[500];
    libewf_error_sprint(error, e, 500);
    libewf_error_free(&error);
    return std::string(e);
  }

  // open the file for reading
  bool open_reader() {

#ifdef WIN32
    const wchar_t *fname = filename.c_str();
    wchar_t **libewf_filenames = NULL;
    int amount_of_filenames = 0;
    libewf_error_t *error=0;

    if(libewf_glob_wide(fname,wcslen(fname),LIBEWF_FORMAT_UNKNOWN,
                       &libewf_filenames,&amount_of_filenames,&error)<0){
      std::stringstream ss;
      ss << "libewf_glob " << fname << ", " << consume_libewf_error(error);
      error_message = ss.str();
      return false;
    }
 
    libewf_handle = 0;
    if(libewf_handle_initialize(&libewf_handle,NULL)<0){
      error_message ="Cannot initialize EWF handle?";
      return false;
    }
    if(libewf_handle_open_wide(libewf_handle,libewf_filenames,
                      amount_of_filenames, LIBEWF_OPEN_READ,&error)<0){
      std::stringstream ss;
      ss << "cannot open " << fname << ", " << consume_libewf_error(error);
      error_message = ss.str();
      return false;
    }
      
    // Free the allocated filenames
    if(libewf_glob_wide_free(libewf_filenames,amount_of_filenames,&error)<0){
      std::stringstream ss;
      error_message = ss.str();
      ss << "libewf_glob_free failed, " << consume_libewf_error(error);
      return false;
    }
    return true;

#else
    const char *fname = filename.c_str();
    char **libewf_filenames = NULL;
    int amount_of_filenames = 0;

    libewf_error_t *error=0;

    if(libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,
                   &libewf_filenames,&amount_of_filenames,&error)<0){
      std::stringstream ss;
      ss << "libewf_glob " << fname << ", " << consume_libewf_error(error);
      error_message = ss.str();
      return false;
    }

    libewf_handle = 0;
    if(libewf_handle_initialize(&libewf_handle,NULL)<0){
      error_message ="Cannot initialize EWF handle?";
      return false;
    }
    if(libewf_handle_open(libewf_handle,libewf_filenames,
                      amount_of_filenames, LIBEWF_OPEN_READ,&error)<0){
      std::stringstream ss;
      ss << "cannot open " << fname << ", " << consume_libewf_error(error);
      error_message = ss.str();
      return false;
    }
      
    // Free the allocated filenames
    if(libewf_glob_free(libewf_filenames,amount_of_filenames,&error)<0){
      std::stringstream ss;
      error_message = ss.str();
      ss << "libewf_glob_free failed, " << consume_libewf_error(error);
      return false;
    }
    return true;
#endif
  }

  uint64_t get_filesize() {
    if (is_open) {
      uint64_t ewf_filesize;
      libewf_handle_get_media_size(libewf_handle,
                                   (size64_t *)&ewf_filesize,NULL);
      return ewf_filesize;
    } else {
      return 0;
    }
  }

  public:
  /**
   * Opens an EWF file reader.
   * Check is_open.  If false, print error_message.
   */
  ewf_file_reader_t(const filename_t& p_filename) :
          filename(p_filename),
          error_message(""),
          libewf_handle(NULL),
          is_open(open_reader()),
          filesize(get_filesize()) {
  }

  // destructor closes any open resources
  ~ewf_file_reader_t() {
    if(libewf_handle){
      libewf_handle_close(libewf_handle,NULL);
      libewf_handle_free(&libewf_handle,NULL);
    }
  }

  std::string read(const uint64_t offset,
                   uint8_t* const buffer,
                   const size_t buffer_size,
                   size_t* const bytes_read) const {

    libewf_error_t *error=0;
    ssize_t ret = libewf_handle_read_random(libewf_handle,buffer,buffer_size,
                                            offset,&error);
    if(ret<0){
      *bytes_read = 0;
      return consume_libewf_error(error);
    } else {
      *bytes_read = static_cast<size_t>(ret);
      return "";
    }
  }
};

} // end namespace hasher

#endif

