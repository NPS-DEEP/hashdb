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

#ifndef MAP_UNORDERED_HASH_HPP
#define MAP_UNORDERED_HASH_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>
#include "file_modes.h"

// Boost includes
#include <boost/functional/hash.hpp>
#include "hash_values.hpp"

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/unordered/unordered_map.hpp>

//#include "hashdb_types.h"
//#include "map_stats.hpp"

// NOTE: you must provide a boost_hash_value function
// for every KEY_T, PAY_T expected.

// managed the mapped file during creation.  Allows for growing the 
// mapped file.
//
// KEY_T must be something that is a lot like md5_t (nothing with pointers)
// both KEY_T and PAY_T must not use dynamic memory
template<typename KEY_T, typename PAY_T>

class map_unordered_hash_t {
  private:
    typedef boost::interprocess::managed_mapped_file   segment_t;

    // Allocator for pairs to be stored in map
    typedef boost::interprocess::allocator<
              std::pair<const KEY_T, PAY_T>,
              segment_t::segment_manager>              allocator_t;
      
    // Map to be stored in memory mapped file
    typedef boost::unordered_map<
              KEY_T, PAY_T,
              boost::hash<KEY_T>,
              std::equal_to<KEY_T>,
              allocator_t>                             map_t;

  public:
    typedef typename map_t::const_iterator map_const_iterator_t;

    // pair returned by emplace
    typedef typename std::pair<map_const_iterator_t, bool> map_pair_it_bool_t;

  private:
    const std::string filename;
    const file_mode_type_t file_mode;
    const std::string data_type_name;
    size_t segment_size;
    size_t expected_size;
    segment_t* segment;
    allocator_t* allocator;
    map_t* map;
    
    // grow the map
    void grow() {
      // delete old allocator and segment
      delete allocator;
      delete segment;

      // increase the file size
      segment_t::grow(filename.c_str(), segment_size/2);

      // open the new segment and allocator
      segment = new segment_t(boost::interprocess::open_only,
                              filename.c_str());
      segment_size = segment->get_size();
      allocator = new allocator_t(segment->get_segment_manager());

      // if file isn't big enough, we need to grow the file and 
      // (recursively through grow) retry.
      try {
        map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (expected_size, boost::hash<KEY_T>(), std::equal_to<KEY_T>(),
                *allocator);
//        map->reserve(expected_size);
      } catch (...) {
        grow();
      }
    }

    // do not allow copy or assignment
    map_unordered_hash_t(const map_unordered_hash_t&);
    map_unordered_hash_t& operator=(const map_unordered_hash_t&);

  public:

    // access to new store based on file_mode_type_t in hashdb_types.h,
    // specifically: READ_ONLY, RW_NEW, RW_MODIFY
    map_unordered_hash_t(const std::string& p_filename,
                         file_mode_type_t p_file_mode) : 
          filename(p_filename)
         ,file_mode(p_file_mode)
         ,data_type_name("map_unordered_hash")
         ,segment_size(100000) 
         ,expected_size(100000) 
         ,segment(0)
         ,allocator(0)
         ,map(0) {

      if (file_mode == READ_ONLY) {
        // open RO
        segment = new segment_t(boost::interprocess::open_read_only,
                                filename.c_str());
        segment_size = segment->get_size();
        allocator = new allocator_t(segment->get_segment_manager());
        map = segment->find<map_t>(data_type_name.c_str()).first; 

      } else {
        // open RW
        if (file_mode == RW_NEW) {
          segment = new segment_t(boost::interprocess::create_only,
                                  filename.c_str(),
                                  segment_size);
        } else if (file_mode == RW_MODIFY) {
          segment = new segment_t(boost::interprocess::open_only,
                                  filename.c_str());
        } else {
          assert(0);
        }
        segment_size = segment->get_size();
        allocator = new allocator_t(segment->get_segment_manager());

        // if file isn't big enough, we need to grow the file and 
        // (recursively through grow) retry.
        try {
          map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (expected_size, boost::hash<KEY_T>(), std::equal_to<KEY_T>(),
                *allocator);
//        map->reserve(expected_size);
        } catch (...) {
          grow();
        }
      }
    }

    ~map_unordered_hash_t() {
      // Don't delete the map as it needs to live inside the mapped file
      delete allocator;
      delete segment;
      if (file_mode != READ_ONLY) {
        segment_t::shrink_to_fit(filename.c_str());
      }
    }

  public:
    // insert
    std::pair<typename map_t::const_iterator, bool>
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
    std::pair<typename map_t::const_iterator, bool>
    change(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: change called in RO mode");
      }

      // get original key
      typename map_t::const_iterator itr = map->find(key);
      if (itr == map->end()) {
        // the old element did not exist
        return std::pair<typename map_t::const_iterator, bool>(map->end(), false);
      }
      if (itr->second == pay) {
        // the payload value is the same
        return std::pair<typename map_t::const_iterator, bool>(itr, false);
      }

      // erase the old element
      size_t num_erased = erase(key);
      if (num_erased != 1) {
        assert(0);
//        // erase failed
//        return std::pair<typename map_t::const_iterator, bool>(map->end(), false);
      } else {
        // put in new
        return map->emplace(key, pay);
      }
    }

    // find
    typename map_t::const_iterator find(const KEY_T& key) const {
        typename map_t::const_iterator itr = map->find(key);
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

    // number of elements
    size_t size() {
      return map->size();
    }

/*
    // stats
    map_stats_t get_map_stats() {
      return map_stats_t(filename, file_mode, data_type_name,
                         segment->get_size(), map->size());
    }
*/
};

#endif

