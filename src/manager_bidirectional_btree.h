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

#ifndef    MANAGER_BIDIRECTIONAL_BTREE_H
#define    MANAGER_BIDIRECTIONAL_BTREE_H

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

#include "manager_modified.h"

// Boost includes
#include <boost/functional/hash.hpp>

#include <boost/btree/header.hpp>
#include <boost/btree/map.hpp>
#include <boost/btree/support/strbuf.hpp>
#

template<typename KEY_T, typename PAY_T>

class manager_bidirectional_btree_t {
  private:
    // glue for forward map
    typedef boost::btree::btree_map<KEY_T, PAY_T> map_forward_t;
    typedef boost::btree::btree_map<PAY_T, KEY_T> map_backward_t;

    std::string  name_forward;
    std::string  name_backward;
    file_mode_type_t file_mode;

    map_forward_t*  map_forward;
    map_backward_t* map_backward;

    // do not allow these
    manager_bidirectional_btree_t(const manager_bidirectional_btree_t&);
    manager_bidirectional_btree_t& operator=(const manager_bidirectional_btree_t&);

  public:

    // open both btree maps
    manager_bidirectional_btree_t(
       const std::string& p_name, file_mode_type_t p_file_mode) : 
          name_forward(p_name + "_forward")
         ,name_backward(p_name + "_backward")
         ,file_mode(p_file_mode)
         ,map_forward(0)
         ,map_backward(0)
    {
      if (file_mode == READ_ONLY) {
        map_forward = new map_forward_t(name_forward, boost::btree::flags::read_only);
        map_backward = new map_backward_t(name_backward, boost::btree::flags::read_only);
        map_forward->max_cache_size(65536);
        map_backward->max_cache_size(65536);
      } else if (file_mode == RW_NEW) {
        map_forward = new map_forward_t(name_forward, boost::btree::flags::truncate);
        map_backward = new map_backward_t(name_backward, boost::btree::flags::truncate);
      } else if (file_mode == RW_MODIFY) {
        map_forward = new map_forward_t(name_forward, boost::btree::flags::read_write);
        map_backward = new map_backward_t(name_backward, boost::btree::flags::read_write);
      }
    }

    ~manager_bidirectional_btree_t() {
      if (file_mode != READ_ONLY) {
        // alert user start
        std::cout << "Packing btree (bidirectional lookup) ...\n";

        // pack btree to .scratch
        map_forward_t map_forward_scratch(name_forward + ".scratch",
                                          boost::btree::flags::truncate);
        map_backward_t map_backward_scratch(name_backward + ".scratch",
                                          boost::btree::flags::truncate);
        for (typename map_forward_t::const_iterator it_forward = map_forward->begin(); it_forward != map_forward->end(); ++it_forward) {
          map_forward_scratch.emplace(it_forward->key(), it_forward->mapped_value());
        }
        for (typename map_backward_t::const_iterator it_backward = map_backward->begin(); it_backward != map_backward->end(); ++it_backward) {
          map_backward_scratch.emplace(it_backward->key(), it_backward->mapped_value());
        }

        // delete btree
        delete map_forward;
        delete map_backward;

        // rename .scratch back to existing btree, replacing existing btree
        std::rename((name_forward+".scratch").c_str(), name_forward.c_str());
        std::rename((name_backward+".scratch").c_str(), name_backward.c_str());

        // alert user done
        std::cout << "Packing btree (bidirectional lookup) completed.\n";
      }
    }

  public:

    void report_status(std::ostream& os) const {
      os << "source lookup store status: ";
      os << "multi-index container type=multi-index-container";
      os << ", element count=" << map_forward->size();
      os << ", node size=" << map_forward->node_size();
      os << ", max cache size=" << map_forward->max_cache_size();
      os << "\n";
    }

    void report_status(dfxml_writer& x) const {
      x.push("source_lookup_store_status");
      x.xmlout("multi_index_container_type", "multi-index-container");
      x.xmlout("element_count", map_forward->size());
      x.xmlout("node_size", map_forward->node_size());
      x.xmlout("max_cache_size", map_forward->max_cache_size());
      x.pop();
    }

    // ************************************************************
    // accessors required by source_lookup_store_t.
    // ************************************************************
    public:
    /**
     * Key is present.
     */
    bool has_key(const KEY_T& key) const {
      typename map_forward_t::const_iterator it = map_forward->find(key);
      return (it != map_forward->end());
    }

    /**
     * Payload is present.
     */
    bool has_pay(const PAY_T& pay) const {
      typename map_backward_t::const_iterator it = map_backward->find(pay);
      return (it != map_backward->end());
    }

    /**
     * Get payload from key.
     */
    bool get_pay(const KEY_T& key, PAY_T& pay) const {
      typename map_forward_t::const_iterator it = map_forward->find(key);
      if (it == map_forward->end()) {
        // no key, clear pay
        pay = PAY_T();
        return false;
      } else {
        // has key, set pay
        pay = it->mapped_value();
        return true;
      }
    }

    /**
     * Get key from payload.
     */
    bool get_key(const PAY_T& pay, KEY_T& key) const {
      typename map_backward_t::const_iterator it = map_backward->find(pay);
      if (it == map_backward->end()) {
        // no pay, clear key
        key = KEY_T();
        return false;
      } else {
        // has pay, set key
        key = it->mapped_value();
        return true;
      }
    }

    /**
     * Insert else fail if left or right already exist.
     */
    void insert_element(const KEY_T& key, const PAY_T& pay) {

      // it is a program error if the key or payload value are already there
      if (has_key(key) || has_pay(pay)) {
        assert(0);
      }
  
      // it is a program error if mode is read only
      if (file_mode == READ_ONLY) {
        assert(0);
      }

      // perform the insert
      map_forward->emplace(key, pay);
      map_backward->emplace(pay, key);
    }

    /**
     * Get highest payload value used.
     */
    KEY_T get_highest_key() {
      KEY_T highest = 0L; // this requires that KEY_T must be uint64_t
      typename map_forward_t::const_iterator end = map_forward->end();
      for (typename map_forward_t::const_iterator it=map_forward->begin(); it!=end; ++it) {
        if (it->key() > highest) {
          highest = it->key();
        }
      }
      return highest;
    }
};

#endif // Main include guard

