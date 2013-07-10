/* 
 * File:   boostMappedReg.cpp
 * Author: jschmid
 * 
 * Created on July 2, 2013, 4:56 PM
 */

#include <string.h>
#include <boost/interprocess/file_mapping.hpp>  
#include <boost/interprocess/mapped_region.hpp>
#include "boost_filemapper.hpp"

// Note that region_offset can be 0 for whole file

namespace BIP = boost::interprocess;
using namespace std;

typedef struct map_impl_ {

  BIP::file_mapping*  file_map;
  BIP::mapped_region* region;
  size_t              region_offset;
  size_t              region_length;

  public:
    map_impl_(const char* file_path,
               map_permissions_t curMode,
               size_t region_offset_init,
               size_t region_length_init):
         file_map(0), region(0), region_offset(0), region_length(0) {

      assert(region_length_init > 0);  // Need some data in map
      BIP::mode_t boostFileMode;
      
      if(MAP_READ_ONLY == curMode) {
          boostFileMode = BIP::read_only;
      }
      else { // read & write mode
          boostFileMode = BIP::read_write;
      }

//printf("Creating map for file: %s\n", file_path);
      file_map      = new BIP::file_mapping(    file_path, 
                                                boostFileMode );

      // Store long-term parameters
      region_offset = region_offset_init;
      region_length = region_length_init;

//printf("Creating region from file-map, with offset: %lu, region length: %lu\n", region_offset, region_length);
      region        = new BIP::mapped_region(   *file_map,
                                                boostFileMode,
                                                region_offset,
                                                region_length );
    }

    // do not allow these
    map_impl_(const map_impl_t& implInstance);
    map_impl_& operator=(const map_impl_&);

/*
            file_map(0), region(0), region_offset(0), region_length(0) 

      printf("Error: construction of 'map_impl_t' class from a const instance not yet supported");
      exit(-1);
    }
*/

    ~map_impl_() {

//printf("DESTROYING MAP OBJ\n");
      if (region) {
        delete region;
      }
      if (file_map) {
        delete file_map;
      }
      
//printf("MAP DESTROYED\n");
    }

    BIP::mapped_region* getRegion() {
      return(region);
    }

  private:
    map_impl_(): file_map(0), region(0), region_offset(0), region_length(0) {}
} map_impl_t;

// A pointer to implementation of Boost Region black box
extern "C"
/**
 * parameters are ...
 */
int map_file_region(const char* filePath,
                    map_permissions_t curMode,
                    int region_offset,
                    size_t region_size,
                    map_impl_t** p_impl,
                    uint8_t** address) {

    int regionLength = (region_size - region_offset);

    // set p_impl
    *p_impl = new map_impl_t(filePath, curMode, region_offset, regionLength);

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

