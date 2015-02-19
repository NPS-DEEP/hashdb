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
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp>
#include "file_mapper.hpp"

namespace BIP = boost::interprocess;
using namespace std;

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

    BIP::mapped_region* getRegion() {
      return(region);
    }
} map_impl_t;

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
    uint8_t* temp = static_cast<uint8_t*>((*p_impl)->getRegion()->get_address());
    *address = temp;

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

