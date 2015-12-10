/* 
 * File:   file_mapper.hpp
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

#ifndef FILE_MAPPER_HPP
#define FILE_MAPPER_HPP

#include <stddef.h>
#include <stdint.h>

//struct map_impl_t;
typedef struct map_impl_ map_impl_t;

typedef enum map_permissions_ {MAP_READ_ONLY,
                               MAP_READ_AND_WRITE} map_permissions_t;

#ifdef __cplusplus
extern "C"
#endif
int map_file_region(const char* filePath,
                    map_permissions_t curMode,
                    int file_offset,
                    size_t region_size,
                    map_impl_t** p_impl,
                    uint8_t** address);




#ifdef __cplusplus
extern "C"
#endif
int unmap_file_region(map_impl_t* p_impl);

#endif

