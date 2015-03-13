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
 * Provides a Boost file map service from C rather than from C++.
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
std::cout << "Boost file_mapper, offset " << file_offset << " size " << region_size << "\n";
    }

    uint8_t* get_address() const {
uint8_t* temp = static_cast<uint8_t*>(region->get_address());
std::cout << "Boost file_mapper, address: " << (void*)temp << "\n";

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

#ifdef WIN32
// usage example: https://groups.google.com/forum/#!topic/avian/Q8bwM4ksubw
// _get_ofshandle: https://msdn.microsoft.com/en-us/library/ks2530z6.aspx

#include <io.h>     // _get_osfhandle

typedef struct map_impl_ {

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

    // get handle to fd
    intptr_t fd_handle = _get_osfhandle(fd);

    // get handle to file map
    intptr_t file_map_handle = CreateFileMapping(
                 fd_handle,
                 NULL,
                 PAGE_READONLY,
                 0,
                 0,
                 NULL);

    // get addr
    addr = static_cast<uint8_t*>(MapViewOfFile(
                 file_map_handle,
                 FILE_MAP_READ,
                 0,
                 0,
                 s.st_size));



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
std::cout << "POSIX file_mapper, address: " << (void*)addr << "\n";

    return static_cast<uint8_t*>(addr);
  }

  // do not allow these
  map_impl_(const map_impl_t& implInstance);
  map_impl_& operator=(const map_impl_&);

  ~map_impl_() {
    UnmapViewOfFile(addr);
/*
    int status = munmap(addr, length);
    if (status != 0) {
      std::cerr << "Error closing Bloom Filter Map.\n";
    }
    status = close(fd);
    if (status != 0) {
      std::cerr << "Error closing Bloom Filter file.\n";
    }
*/
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

  size_t length;
  int fd;
  void* addr;

  public:
  map_impl_(const char* file_path,
            const map_permissions_t curMode,
            const size_t file_offset,
            const size_t region_size): length(0), fd(0), addr(NULL) {
std::cout << "POSIX file_mapper, offset " << file_offset << " size " << region_size << "\n";
std::cout << "POSIX file_mapper, cur_mode " << curMode << " MAP_READ_ONLY " << MAP_READ_ONLY << "\n";

    // get File Descriptor
    const int file_mode = (curMode == MAP_READ_ONLY) ?
                                         O_RDONLY|O_BINARY : O_RDWR|O_BINARY;
    fd = open(file_path,file_mode,0);
std::cout << "POSIX file_mapper, file mode " << file_mode << " fd " << fd << "\n";
std::cout << "POSIX file_mapper, filename " << file_path << "\n";
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
std::cout << "POSIX file_mapper, address: " << (void*)addr << "\n";

    return static_cast<uint8_t*>(addr);
  }

  // do not allow these
  map_impl_(const map_impl_t& implInstance);
  map_impl_& operator=(const map_impl_&);

  ~map_impl_() {
    int status = munmap(addr, length);
    if (status != 0) {
      std::cerr << "Error closing Bloom Filter Map.\n";
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

