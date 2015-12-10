/* 
 * File:   file_mapper.cpp
 * Author: jschmid
 * 
 * Created on July 2, 2013, 4:56 PM
 */

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
// Released into the public domain on July 11, 2013 by Bruce Allen.

/**
 * \file
 * Provides a system-specific memory map service for Bloom: POSIX or WIN.
 * Legacy BOOST code is retained.
 */

#include <config.h>
#include <string.h>
#include <iostream>
#include "file_mapper.hpp"

//#define USE_BOOST_FOR_BLOOM
#ifdef USE_BOOST_FOR_BLOOM

// file_mapper implemented using Boost
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp>
namespace BIP = boost::interprocess;

typedef struct map_impl_ {

  private:
  BIP::mapped_region* region;

  public:
    map_impl_(const char* file_path,
               map_permissions_t curMode,
               size_t file_offset,
               size_t region_size): region(0) {

      // convert file mode to boost file mode
      BIP::mode_t boost_file_mode = (curMode == MAP_READ_ONLY) ?
                                    BIP::read_only : BIP::read_write;

      // create file mapping
      BIP::file_mapping file_map(file_path, boost_file_mode);

      // map bloom region of file to memory
      region = new BIP::mapped_region(file_map,
                                      boost_file_mode,
                                      file_offset,
                                      region_size);
    }

    uint8_t* get_address() const {
      return static_cast<uint8_t*>(region->get_address());
    }

    // do not allow these
    map_impl_(const map_impl_t& implInstance);
    map_impl_& operator=(const map_impl_&);

    ~map_impl_() {
      if (region) {
        delete region;
      } else {
        // program error
        assert(0);
      }
    }
} map_impl_t;

#else

#ifdef _WIN32
// file map approach adapted from liblmdb/mdb.c
#include <windows.h>

typedef struct map_impl_ {

  private:
  size_t length;
  HANDLE fd;
  HANDLE file_mapping;
  void* addr;

  public:
  map_impl_(const char* file_path,
            const map_permissions_t curMode,
            const size_t file_offset,
            const size_t region_size): length(0), fd(0), addr(NULL) {

    // open file and get File Descriptor
    const int desired_file_access = (curMode == MAP_READ_ONLY)
                             ? GENERIC_READ : GENERIC_READ|GENERIC_WRITE;
    const int share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE;
    const int creation_disposition = (curMode == MAP_READ_ONLY)
                             ? OPEN_EXISTING : OPEN_ALWAYS;
    const int flags_and_attributes = FILE_ATTRIBUTE_NORMAL;
    fd = CreateFile(file_path,
                    desired_file_access,
                    share_mode,
                    NULL,                           // security attributes
                    creation_disposition,
                    flags_and_attributes,
                    NULL);                          // template file
    if (fd == INVALID_HANDLE_VALUE) {
      std::cerr << "Error " << GetLastError() << "\n";
      std::cerr << "Failure opening Bloom Filter file " << file_path
                << "\nAborting.\n";
      exit(1);
    }

    // open file mapping
    const int page_protection = (curMode == MAP_READ_ONLY)
                             ? PAGE_READONLY : PAGE_READWRITE;
    const size_t combined_size = region_size + file_offset;
    const int maximum_size_high = combined_size >> 32;
    const int maximum_size_low = combined_size & 0xffffffff;
    file_mapping = CreateFileMapping(fd,
                                     NULL,          // security attributes
                                     page_protection,
                                     maximum_size_high,
                                     maximum_size_low,
                                     NULL);         // lp name
    if (file_mapping == 0) {
      std::cerr << "Error " << GetLastError() << "\n";
      std::cerr << "Failure Mapping Bloom Filter file " << file_path
                << "\nAborting.\n";
      exit(1);
    }

    // get addr
    const int desired_map_access = (curMode == MAP_READ_ONLY)
                             ? FILE_MAP_READ : FILE_MAP_WRITE;
    const int file_offset_high = file_offset >> 32;
    const int file_offset_low = file_offset & 0xffffffff;
    const size_t number_of_bytes = region_size;
    addr = MapViewOfFile(file_mapping,
                         desired_map_access,
                         file_offset_high,
                         file_offset_low,
                         number_of_bytes);
    if (addr == 0) {
      std::cerr << "Failure obtaining address of Bloom Filter map."
                << "\nAborting.\n";
      std::cerr << "Error " << GetLastError() << "\n";
      exit(1);
    }

    // save length for munmap
    length = region_size;
  }

  uint8_t* get_address() const {
    return static_cast<uint8_t*>(addr);
  }

  // do not allow these
  map_impl_(const map_impl_t& implInstance);
  map_impl_& operator=(const map_impl_&);

  ~map_impl_() {
    // close the map
    bool status = UnmapViewOfFile(addr);
    if (status == false) {
      // fail if zero
      std::cerr << "Error closing Bloom Filter map.\n";
      std::cerr << "Error " << GetLastError() << "\n";
    }

    // close the file
    status = CloseHandle(fd);
    if (status == false) {
      // fail if zero
      std::cerr << "Error closing Bloom Filter file.\n";
      std::cerr << "Error " << GetLastError() << "\n";
    }
  }
} map_impl_t;


#else

// file_mapper implemented using mmap for POSIX systems

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct map_impl_ {

  private:
  size_t length;
  int fd;
  void* addr;

  public:
  map_impl_(const char* file_path,
            const map_permissions_t curMode,
            const size_t file_offset,
            const size_t region_size): length(0), fd(0), addr(NULL) {

    // get File Descriptor
    const int file_mode = (curMode == MAP_READ_ONLY) ?
                                         O_RDONLY|O_BINARY : O_RDWR|O_BINARY;
    fd = open(file_path,file_mode,0);
    if (fd < 0) {
      std::cerr << "cannot open Bloom Filter file\nAborting.\n";
      exit(1);
    }

    // open MMAP
    const int prot = (curMode == MAP_READ_ONLY) ? PROT_READ :
                                         PROT_READ|PROT_WRITE;
    addr = mmap(0, region_size, prot, MAP_SHARED, fd, file_offset);
    if (addr == NULL) {
      std::cerr << "cannot open Bloom Filter map\nAborting.\n";
    }

    // save length for munmap
    length = region_size;
  }

  uint8_t* get_address() const {
    return static_cast<uint8_t*>(addr);
  }

  // do not allow these
  map_impl_(const map_impl_t& implInstance);
  map_impl_& operator=(const map_impl_&);

  ~map_impl_() {
    int status = munmap(addr, length);
    if (status != 0) {
      std::cerr << "Error closing Bloom Filter map.\n";
    }
    status = close(fd);
    if (status != 0) {
      std::cerr << "Error closing Bloom Filter file.\n";
    }
  }
} map_impl_t;
#endif
#endif

extern "C"
int map_file_region(const char* filePath,
                    map_permissions_t curMode,
                    int file_offset,
                    size_t region_size,
                    map_impl_t** p_impl,
                    uint8_t** address) {

    // set p_impl
    *p_impl = new map_impl_t(filePath, curMode, file_offset, region_size);

    // set address of mapped region
    *address = (*p_impl)->get_address();

    // later, may return error in map_impl_t
    return 0;
}

extern "C"
int unmap_file_region(map_impl_t* p_impl) {
  if (p_impl == NULL) {
    // program error
    return -1;
  }

  delete p_impl;
  p_impl = NULL;
  return 0;
}

