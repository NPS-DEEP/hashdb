// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
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
 * Manager for boost::multi_index_container.
 */

#ifndef    MANAGER_MULTI_INDEX_CONTAINER_H
#define    MANAGER_MULTI_INDEX_CONTAINER_H

// source location record and fixed size source location record
#include "source_location_record.hpp"

// Local includes
#include "manager_modified_includes.h"
#include "fcntl.h"
#include "dfxml/src/dfxml_writer.h"

// Standard includes
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>

// TR1 includes:
#include <tr1/cmath>     // log2

// Boost includes
#include<utility>
#include<functional>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

// template usage fails without these "using" directives
using boost::multi_index_container;
using namespace boost::multi_index;

// prefer within class private
struct key_tag{};
struct pay_tag{};

template<typename KEY_T, typename PAY_T>

class manager_multi_index_container_t {
  public:

    typedef KEY_T                                    key_t;
    typedef PAY_T                                    pay_t;

  private:
    typedef boost::interprocess::managed_mapped_file segment_t;
    typedef segment_t::segment_manager               segment_manager_t;

    // do not allow these
    manager_multi_index_container_t(const manager_multi_index_container_t&);
    manager_multi_index_container_t& operator=(const manager_multi_index_container_t&);

//    struct key_tag{};
//    struct pay_tag{};

  public:
    typedef std::pair<key_t, pay_t>                  val_t;

    typedef boost::interprocess::allocator<
              val_t, segment_manager_t>              allocator_t;
      
    // multi_index_container
    typedef boost::multi_index_container<
      val_t,
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
          boost::multi_index::tag<key_tag>,
          boost::multi_index::member<val_t, key_t, &val_t::first>
        >,
        boost::multi_index::ordered_unique<
          boost::multi_index::tag<pay_tag>,
          boost::multi_index::member<val_t, pay_t, &val_t::second>
        >
      >,
      allocator_t
    > map_t;

  protected:
    std::string  name;
    std::string  data_type_name;
    file_mode_type_t file_mode;

    segment_t*   segment;
    allocator_t* allocator;
    map_t*       map;
    size_t       size;

  public:

    // Write access to new store
    manager_multi_index_container_t(
        const std::string& p_ds_name
       ,const std::string& p_name
       ,std::size_t        p_size // only useful on file mode of RW_NEW
       ,file_mode_type_t   p_file_mode
      ) : 
          name(p_name)
         ,data_type_name(p_ds_name)
         ,file_mode(p_file_mode)
         ,segment(0)
         ,allocator(0)
         ,map(0)
         ,size(p_size)
    {
      if (file_mode == READ_ONLY) {
        openone_read_only(name,segment,allocator,map,size);
      } else {
        openone(name,segment,allocator,map,size);
      }
    }


    ~manager_multi_index_container_t() {
      // Don't delete the map as it needs to live inside the mapped file
      delete allocator;
      delete segment;
    }

    void pack() {
      // validate mode
      if (file_mode == READ_ONLY) {
        assert(0);
      }
    }
 
  private:

  
    void openone_read_only(
        const std::string& _name
       ,segment_t*&        _segment
       ,allocator_t*&      _allocator
       ,map_t*&            _map
       ,std::size_t&       _size
      ) {
      _segment   = new segment_t(
                        boost::interprocess::open_read_only, _name.c_str()
                      );
      _size      = _segment->get_size();
      _allocator = new allocator_t(_segment->get_segment_manager());
      _map       = _segment->find<map_t>(data_type_name.c_str()).first; 
    }

    void openone(
        const std::string& _name
       ,segment_t*&        _segment
       ,allocator_t*&      _allocator
       ,map_t*&            _map
       ,std::size_t&       _size
      ) {
      _segment   = new segment_t(
                    boost::interprocess::open_or_create, _name.c_str(), _size
                  );
      _size      = _segment->get_size();
      _allocator = new allocator_t(_segment->get_segment_manager());

      // if file isn't big enough, we need to grow the file and 
      // (recursively through grow) retry.
      try {
        _map = _segment->find_or_construct<map_t>(data_type_name.c_str())(*_allocator);
      } catch (...) {
        grow(_name,_segment,_allocator,_map,_size); 
      }
    }

    void grow(
        const std::string& _name
       ,segment_t*&        _segment
       ,allocator_t*&      _allocator
       ,map_t*&            _map
       ,std::size_t&       _size
      ) {
      delete _allocator;
      delete _segment;

      segment_t::grow(_name.c_str(),_size/2);
      openone(_name,_segment,_allocator,_map,_size); 
    }
  
  public:

    void report_status(std::ostream& os) const {
      os << "source lookup store status: ";
      os << "multi-index container type=multi-index-container";
      os << ", element count=" << map->size();
      os << ", bytes=" << size;
      os << "\n";
    }

    void report_status(dfxml_writer& x) const {
      x.push("source_lookup_store_status");
      x.xmlout("multi_index_container_type", "multi-index-container");
      x.xmlout("element_count", map->size());
      x.xmlout("bytes", size);
      x.pop();
    }

    // ************************************************************
    // accessors as required by source_lookup_store_t.
    // ************************************************************
    public:
    /**
     * Determine if key exists.
     */
    bool has_key(const key_t& key) const {
      typename map_t::iterator it = map->get<key_tag>().find(key);
      // this requires "using namespace boost::multi_index;"
      return (it != map->get<key_tag>().end());
return false;
    }

    /**
     * Determine if pay exists.
     */
    bool has_pay(const pay_t& pay) const {
      typename boost::multi_index::index<map_t, pay_tag>::type::const_iterator it = map->get<pay_tag>().find(pay);
      return (it != map->get<pay_tag>().end());
    }

    /**
     * Insert else fail if left or right already exist.
     */
    void insert_element(const key_t& key, const pay_t& pay) {

      // it is a program error if the key or value are already there
      if (has_key(key) || has_pay(pay)) {
        assert(0);
      }
  
      // perform the insert
      if (file_mode == READ_ONLY) {
        assert(0);
      }
      bool    done = true;
      do { // if item doesn't exist, may need to grow map
        done = true;
        try {
          map->insert(typename map_t::value_type(key, pay));

        } catch(boost::interprocess::interprocess_exception& ex) {
          grow(name,segment,allocator,map,size);
          done = false;
        }
      } while (not done);
    }

    /**
     * Get key else fail.
     */
    void get_key(const pay_t& pay, key_t& key) {

      // it is a program error if pay is not there
      if (!has_pay(pay)) {
        assert(0);
      }
  
      typename boost::multi_index::index<map_t, pay_tag>::type::const_iterator it = map->get<pay_tag>().find(pay);
      key = it->first;
    }

    /**
     * Get pay else fail.
     */
    void get_pay(const key_t& key, pay_t& pay) {
 
      // it is a program error if key is not there
      if (!has_key(key)) {
        assert(0);
      }
  
      typename map_t::iterator it = map->get<key_tag>().find(key);
      pay = it->second;
    }

    /**
     * Get highest payload value used.
     */
    key_t get_highest_key() {
      key_t highest = 0L; // this requires that key_t must be uint64_t
      typename map_t::iterator end = map->get<key_tag>().end();
      for (typename map_t::iterator it=map->get<key_tag>().begin(); it!=end; ++it) {
        if (it->first > highest) {
          highest = it->first;
        }
      }
      return highest;
    }
};

#endif // Main include guard

