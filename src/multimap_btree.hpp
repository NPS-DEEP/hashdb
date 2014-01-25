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
 * Glue to btree multimap.
 */

#ifndef MULTIMAP_BTREE_HPP
#define MULTIMAP_BTREE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>
#include "file_modes.h"

// Boost includes
//#include <boost/functional/hash.hpp>

#include <boost/btree/btree_map.hpp>

//#include "hashdb_types.h"
//#include "map_stats.hpp"

// KEY_T must be something that is a lot like md5_t (nothing with pointers)
// both KEY_T and PAY_T must not use dynamic memory
template<typename KEY_T, typename PAY_T>
class multimap_btree_t {
  private:
    // btree map
    typedef boost::btree::btree_multimap<
              KEY_T, PAY_T>                            map_t;

  public:
    typedef class map_t::const_iterator map_const_iterator_t;
    typedef class std::pair<map_const_iterator_t, map_const_iterator_t> map_const_iterator_range_t;

  private:
    const std::string filename;
    const file_mode_type_t file_mode;
    const std::string data_type_name;
    map_t* map;
    
    // do not allow copy or assignment
    multimap_btree_t(const multimap_btree_t&);
    multimap_btree_t& operator=(const multimap_btree_t&);

  public:

    // access to new store based on file_mode_type_t in hashdb_types.h,
    // specifically: READ_ONLY, RW_NEW, RW_MODIFY
    multimap_btree_t(const std::string& p_filename,
                         file_mode_type_t p_file_mode) : 
          filename(p_filename)
         ,file_mode(p_file_mode)
         ,data_type_name("map_btree")
         ,map(0) {

      if (file_mode == READ_ONLY) {
        map = new map_t(filename, boost::btree::flags::read_only);
        // note, old code used to "map->max_cache_size(65536);" here.
        // lets not limit this now.
      } else if (file_mode == RW_NEW) {
        map = new map_t(filename, boost::btree::flags::truncate);
      } else if (file_mode == RW_MODIFY) {
        map = new map_t(filename, boost::btree::flags::read_write);
      } else {
        assert(0);
      }
    }

    ~multimap_btree_t() {
      // pack btree to .scratch
      // zz lets not optimize this at this time, see manager_modified.h

      // close
      delete map;
    }

  public:
    // range for key
    map_const_iterator_range_t equal_range(const KEY_T& key) const {
      map_const_iterator_range_t it = map->equal_range(key);
      return it;
    }

    // count for key
    size_t count(const KEY_T& key) const {
      return map->count(key);
    }

    // emplace
    bool emplace(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: emplace called in RO mode");
      }

      // see if element already exists
      typename map_t::const_iterator it = find(key, pay);
      if (it != map->end()) {
        return false;
      }

      // insert the element
//      typename map_t::const_iterator it2 = map->emplace(key, pay);
      map->emplace(key, pay);
//      if (it == map->end()) {
//        assert(0);
//      }
      return true;
    }

    // erase
    bool erase(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: erase called in RO mode");
      }

      // find the uniquely identified element
      map_const_iterator_range_t it = map->equal_range(key);
      typename map_t::const_iterator lower = it.first;
      if (lower == map->end()) {
        return false;
      }
      const typename map_t::const_iterator upper = it.second;
      for (; lower != upper; ++lower) {
        if (lower->second == pay) {
          // found it so erase it
          map->erase(lower);
          return true;
        }
      }
      // pay is not a member in range of key
      return false;
    }

    // find
    typename map_t::const_iterator find(const KEY_T& key, const PAY_T& pay) const {
      // find the uniquely identified element
      map_const_iterator_range_t it = map->equal_range(key);
      typename map_t::const_iterator lower = it.first;
      if (lower == map->end()) {
        return map->end();
      }
      const typename map_t::const_iterator upper = it.second;
      for (; lower != upper; ++lower) {
        if (lower->second == pay) {
          // found it
          return lower;
        }
      }
      // if here, pay was not found
      return map->end();
    }

    // has
    bool has(const KEY_T& key, const PAY_T& pay) const {
      // find the uniquely identified element
      map_const_iterator_range_t it = map->equal_range(key);
      typename map_t::const_iterator lower = it.first;
      if (lower == map->end()) {
        return false;
      }
      const typename map_t::const_iterator upper = it.second;
      for (; lower != upper; ++lower) {
        if (lower->second == pay) {
          // found it
          return true;
        }
      }
      // if here, pay was not found
      return false;
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
      return map_stats_t(filename, file_mode, "multimap_btree", 0, map->size());
    }
*/
};

#endif

/*
    // change
    std::pair<class map_t::const_iterator, bool>
    change(const KEY_T& key, const PAY_T& pay) {
      if (file_mode == READ_ONLY) {
        throw std::runtime_error("Error: change called in RO mode");
      }

      // erase the old element
      size_t num_erased = erase(key);
      if (num_erased != 1) {
        // erase failed
        return std::pair<class map_t::const_iterator, bool>(map->end(), false);
      } else {
        // put in new
        return map->emplace(key, pay);
      }
    }
*/

