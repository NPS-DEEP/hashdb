// Author:  Joel Young <jdyoung@nps.edu>
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
 * Glue to map.
 */

#ifndef MAP_RED_BLACK_TREE_HPP
#define MAP_RED_BLACK_TREE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>
#include "file_modes.h"

// TR1 includes:
//#include <tr1/cmath>     // log2

// Boost includes
#include <boost/functional/hash.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>

#include "hashdb_types.h"
#include "map_stats.hpp"

// managed the mapped file during creation.  Allows for growing the 
// mapped file.
//
// KEY_T must be something that is a lot like md5_t (nothing with pointers)
// both KEY_T and PAY_T must not use dynamic memory
template<typename KEY_T, typename PAY_T>
class map_red_black_tree_t {
  private:
    typedef boost::interprocess::managed_mapped_file   segment_t;

    // Allocator for pairs to be stored in map
    typedef boost::interprocess::allocator<
              std::pair<const KEY_T, PAY_T>,
              segment_t::segment_manager>              allocator_t;
      
    // Map to be stored in memory mapped file
    typedef boost::interprocess::map<
              KEY_T, PAY_T, std::less<KEY_T>, 
              allocator_t>                             map_t;

  public:
    typedef class map_t::const_iterator map_const_iterator;

  private:
    const std::string filename;
    const file_mode_type_t file_mode;
    const std::string data_type_name;
    size_t size;
    segment_t* segment;
    allocator_t* allocator;
    map_t* map;
    
    // grow the map
    void grow() {
      // delete old allocator and segment
      delete allocator;
      delete segment;

      // increase the file size
      segment_t::grow(filename.c_str(), size/2);

      // open the new segment and allocator
      segment = new segment_t(boost::interprocess::open_only,
                              filename.c_str());
      size = segment->get_size();
      allocator = new allocator_t(segment->get_segment_manager());

      // if file isn't big enough, we need to grow the file and 
      // (recursively through grow) retry.
      try {
        map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (std::less<KEY_T>(), *allocator);
      } catch (...) {
        grow();
      }
    }

    // do not allow copy or assignment
    map_red_black_tree_t(const map_red_black_tree_t&);
    map_red_black_tree_t& operator=(const map_red_black_tree_t&);

  public:

    // access to new store based on file_mode_type_t in hashdb_types.h,
    // specifically: READ_ONLY, RW_NEW, RW_MODIFY
    map_red_black_tree_t(const std::string& p_filename,
                         file_mode_type_t p_file_mode) : 
          filename(p_filename)
         ,file_mode(p_file_mode)
         ,data_type_name("map_red_black_tree")
         ,size(100000) 
         ,segment(0)
         ,allocator(0)
         ,map(0) {

      if (file_mode == READ_ONLY) {
        // open RO
        segment = new segment_t(boost::interprocess::open_read_only,
                                filename.c_str());
        size = segment->get_size();
        allocator = new allocator_t(segment->get_segment_manager());
        map = segment->find<map_t>(data_type_name.c_str()).first; 

      } else {
        // open RW
        if (file_mode == RW_NEW) {
          segment = new segment_t(boost::interprocess::create_only,
                                  filename.c_str(),
                                  size);
        } else if (file_mode == RW_MODIFY) {
          segment = new segment_t(boost::interprocess::open_only,
                                  filename.c_str());
        } else {
          assert(0);
        }
        size = segment->get_size();
        allocator = new allocator_t(segment->get_segment_manager());

        // if file isn't big enough, we need to grow the file and 
        // (recursively through grow) retry.
        try {
          map = segment->find_or_construct<map_t>(data_type_name.c_str())
                  (std::less<KEY_T>(), *allocator);
        } catch (...) {
          grow();
        }
      }
    }

    ~map_red_black_tree_t() {
      // Don't delete the map as it needs to live inside the mapped file
      delete allocator;
      delete segment;
      if (file_mode != READ_ONLY) {
        segment_t::shrink_to_fit(filename.c_str());
      }
    }

  public:
    // insert
    std::pair<class map_t::const_iterator, bool>
    emplace(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: emplace called in RO mode");
      }

      bool    done = true;
      do { // if item doesn't exist, may need to grow map
        done = true;
        try {
          return map->emplace(key, pay);
        } catch(boost::interprocess::interprocess_exception& ex) {
          grow();
          done = false;
        }
      } while (not done);
      // the compiler doesn't know we can't get here, so appease it
      assert(0);
    }

    // erase
    size_t erase(const KEY_T& key) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: erase called in RO mode");
      }

      size_t num_erased = map->erase(key);
      return num_erased;
    }

    // change
    std::pair<class map_t::const_iterator, bool>
    change(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: change called in RO mode");
      }

      // get original key
      class map_t::const_iterator itr = map->find(key);
      if (itr == map->end()) {
        // the old element did not exist
        return std::pair<class map_t::const_iterator, bool>(map->end(), false);
      }
      if (itr->second == pay) {
        // the payload value is the same
        return std::pair<class map_t::const_iterator, bool>(itr, false);
      }

      // erase the old element
      size_t num_erased = erase(key);
      if (num_erased != 1) {
        assert(0);
//        // erase failed
//        return std::pair<class map_t::const_iterator, bool>(map->end(), false);
      } else {
        // put in new
        return map->emplace(key, pay);
      }
    }

    // find
    typename map_t::const_iterator find(const KEY_T& key) const {
        class map_t::const_iterator itr = map->find(key);
        return itr;
    }

    // has
    bool has(const KEY_T& key) const {
      if (find(key) != map->end()) {
        return true;
      } else {
        return false;
      }
    }

    // begin
    typename map_t::const_iterator begin() const {
      return map->begin();
    }

    // end
    typename map_t::const_iterator end() const {
      return map->end();
    }

    // stats
    map_stats_t get_map_stats() {
      return map_stats_t(filename, file_mode, data_type_name,
                         segment->get_size(), map->size());
    }
};

#endif

