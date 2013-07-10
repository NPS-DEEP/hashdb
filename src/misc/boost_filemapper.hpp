/* 
 * File:   boost_filemapper.hpp
 * Author: jschmid
 * 
 * Created on July 2, 2013, 4:56 PM
 */

#ifndef BOOST_FILEMAPPER_HPP
#define BOOST_FILEMAPPER_HPP

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
                    int region_offset,
                    size_t region_size,
                    map_impl_t** p_impl,
                    uint8_t** address);




#ifdef __cplusplus
extern "C"
#endif
int unmap_file_region(map_impl_t* p_impl);

#endif

