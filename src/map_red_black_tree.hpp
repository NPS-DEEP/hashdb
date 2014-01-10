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
 * Glue to red-black-tree map.
 */

#ifndef MAP_RED_BLACK_TREE_HPP
#define MAP_RED_BLACK_TREE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>

// TR1 includes:
#include <tr1/cmath>     // log2

// Boost includes
#include <boost/functional/hash.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>

// managed the mapped file during creation.  Allows for growing the 
// mapped file.
//
// KEY_T must be something that is a lot like md5_t (nothing with pointers)
// both KEY_T and PAY_T must not use dynamic memory
template<typename KEY_T, typename PAY_T>

class map_red_black_tree_t {
  private:
    typedef boost::interprocess::managed_mapped_file segment_t;
    typedef segment_t::segment_manager               segment_manager_t;

  public:
    // Entry stored by map
    typedef std::pair<const key_t, pay_t>            val_t;

    // Allocator for pairs to be stored in map
    typedef boost::interprocess::allocator<
              val_t, segment_manager_t>              allocator_t;
      
    // Map to be stored in memory mapped file
    typedef boost::interprocess::map<
              key_t, pay_t, std::less<key_t>, 
              allocator_t>                           map_t;

  protected:
    std::string  name;
    std::size_t  expected_size;              // expected size of container
    std::string  data_type_name;
    file_mode_type_t file_mode;

    typedef std::vector<segment_t*>                  segments_t;
    typedef std::vector<allocator_t*>                allocators_t;

    segments_t   segments;
    allocators_t allocators;

  public:

    // access to new store based on file_mode_type_t in hashdb_types.h,
    // specifically: READ_ONLY, RW_NEW, RW_MODIFY
    map_red_black_tree_t(
        const std::string& p_ds_name
       ,const std::string& p_name
       ,std::size_t        p_size
       ,std::size_t        p_expected_size
       ,const std::size_t  p_shard_count
       ,file_mode_type_t   p_file_mode
      ) : 
          name(p_name)
         ,expected_size(p_expected_size)
         ,data_type_name(p_ds_name)
         ,file_mode(p_file_mode)
         ,segment(0)
         ,allocator(0)
         ,map(0)
         ,size(p_size) 
    {
      if (file_mode == READ_ONLY) {
        openone_read_only(name, segment, allocator, map, size);
      } else {
        openone(name, segment, allocator, map, size);
      }
    }

    ~map_red_black_tree_t() {
      for (uint8_t map=0; map < shard_count; ++map) {
        // Don't delete the map as it needs to live inside the mapped file
        delete allocator;
        delete segment;
        if (file_mode != READ_ONLY) {
          segment_t::shrink_to_fit(name.c_str());
        }
      }
    }

 
  
    void openone_read_only(
        const std::string& _name
       ,segment_t*&        segment
       ,allocator_t*&      allocator
       ,map_t*&            map
       ,std::size_t&       _size
      ) {
      segment   = new segment_t(
                        boost::interprocess::open_read_only, _name.c_str()
                      );
      _size      = segment->get_size();
      allocator = new allocator_t(segment->get_segment_manager());
      map       = segment->find<map_t>(data_type_name.c_str()).first; 
    }

    void openone(
        const std::string& _name
       ,segment_t*&        segment
       ,allocator_t*&      allocator
       ,map_t*&            map
       ,std::size_t&       _size) {

      segment   = new segment_t(
                    boost::interprocess::open_or_create, _name.c_str(), _size
                  );
      _size      = segment->get_size();
      allocator = new allocator_t(segment->get_segment_manager());

      // if file isn't big enough, we need to grow the file and 
      // (recursively through grow) retry.
      try {
        map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (std::less<key_t>(), *allocator);
      } catch (...) {
        grow(_name,segment,allocator,map,_size); 
      }
    }

    void grow(
        const std::string& _name
       ,segment_t*&        segment
       ,allocator_t*&      allocator
       ,map_t*&            map
       ,std::size_t&       _size
      ) {
      delete allocator;
      delete segment;

      segment_t::grow(_name.c_str(),_size/2);
      openone(_name,segment,allocator,map,_size); 
    }
  
  public:

    // insert
    std::pair<map_t::const_iterator, bool>
    emplace(const key_t& key, const pay_t& pay) {
      if (file_mode == READ_ONLY) {
        assert(0);
      }
      bool    done = true;
      do { // if item doesn't exist, may need to grow map
        done = true;
        try {
          (*map)[key] = pay;
        } catch(boost::interprocess::interprocess_exception& ex) {
          grow(name, segment, allocator, map, size);
          done = false;
        }
      } while (not done);
    }

    // erase
    size_t erase(const key_t& key) {
      size_t num_erased = map->erase(key);
    }

    // change
    std::pair<map_t::const_iterator, bool>
    change(const key_t& key, const pay_t& pay) {

      // erase the old element
      size_t num_erased = erase(key);
      if (num_erased != 1) {
        // erase failed
        return pair<map_t::const_iterator, bool>(map->end(), false);
      } else {
        // put in new
        return emplace(key, pay);
      }
    }

    // find
    map_t::const_iterator find(const key_t& key) const {
        map_t::const_iterator itr = map->find(key);
        return itr;
    }

    // has
    bool has(const key_t& key) const {
      if (find(key) != map->end()) {
        return true;
      } else {
        return false;
      }
    }

    // begin
    map_t::const_iterator begin() const {
      return map->begin();
    }

    // end
    map_t::const_iterator end() const {
      return map->end();
    }

    // size
    std::size_t size() const {
      std::size_t _size = 0;
      for (uint8_t i = 0; i < shard_count; ++i) {
        _size += maps[i]->size();
      }
      return _size;
    }


    void report_status(std::ostream& os) const {
      os << "hash store status: ";
      os << "map type=red-black-tree";
      os << ", element count=" << size() << "\n";
      for (uint32_t i = 0; i < shard_count; ++i) {
        os << "shard " << i << ": ";
        os << "elements=" << maps[i]->size();
        os << ", bytes=" << sizes[i];
        os << "\n";
      }
    }

    void report_status(dfxml_writer& x) const {
      x.push("hash_store_status");
      x.xmlout("map_type", "red-black-tree");
      x.xmlout("element_count",size());
      for (uint32_t i = 0; i < shard_count; ++i) {
        std::stringstream s;
        s << "index='" << i << "'";
        x.push("shard", s.str());
        x.xmlout("elements", maps[i]->size());
        x.xmlout("bytes",    sizes[i]);
        x.pop();
      }
      x.pop();
    }
}

#endif

