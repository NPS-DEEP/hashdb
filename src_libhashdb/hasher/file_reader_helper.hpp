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
 * Raw file accessors.  Mostly copied from bulk_extractor/src/image_process.cpp
 *
 * Provides:
 *   pread64() for Windows
 *   get_filesize()
 *   get_filesize_by_filename()
 */

#ifndef FILE_READER_HELPER_HPP
#define FILE_READER_HELPER_HPP

#include <sys/stat.h>
#include <fcntl.h>
#include "filename_t.hpp"

// if pread64 is not defined then define it in the global namespace
#ifdef WIN32
int pread64(HANDLE current_handle,char *buf,size_t bytes,uint64_t offset);
#else
  #if !defined(HAVE_PREAD64) && !defined(HAVE_PREAD) && defined(HAVE__LSEEKI64)
size_t pread64(int d,void *buf,size_t nbyte,int64_t offset);
  #endif
#endif

namespace hasher {

// return error_message or ""
std::string get_filesize_by_filename(const filename_t &fname, uint64_t* size);

} // end namespace hasher

#endif

