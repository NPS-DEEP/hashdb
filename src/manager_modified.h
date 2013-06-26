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

// NOTE: modified (bda) 2/25/2013:
//       removed embedded bloom filter
//       added glue interfaces at bottom
//       renamed function parameters that shadow member names
//       added support for append file mode
//       removed preload for btree
//
// Title:   manager
//  
// I don't like this header file...it is insane but...here goes...
//  
// This is a very funny header file.  It can be included multiple times,
// once for each persistant store to use. 
//  
// For example
// #define LOCAL_MAP_TYPE MAP_TYPE_BTREE_MAP
// #include "manager.h"
//
// will generate the burst_manager_btree_map< > template.
//
// If LOCAL_MAP_TYPE is not defined, burst_managers for all the supported
// back ends will be generated.
//
// Use manager_type(key_type, payload_type, MAP_TYPE_) to get the 
// type of map you want to use.  e.g. 
//
// typedef manager_type(key_t, md5_t, MAP_TYPE_MAP) map_t;
//
// is equivalent to:
//
// typedef burst_manager_map<key_t, md5_t>          map_t;
//
// Use manager_map_type_name( LOCAL_MAP_TYPE ) to get the quoted
// cannonical name. e.g. manager_map_type_name(MAP_TYPE_MAP) yields
// "map"
//

/**
 * \file
 * Manager for boost maps.
 */

#ifndef    MANAGER_JDY_20120529_FIRST_CALL
#define    MANAGER_JDY_20120529_FIRST_CALL

// Supported persistant back ends:
#define MAP_TYPE_MAP           0   // red-black tree
#define MAP_TYPE_FLAT_MAP      1   // sorted-vector
#define MAP_TYPE_UNORDERED_MAP 2   // hash-map
#define MAP_TYPE_BTREE_MAP     3   // btree

///////////////// DETAILS ////////////////////
//////////////////////////////////////////////
//////////DO/NOT/USE/IN/USER/CODE/////////////
//////////////////////////////////////////////
// Number to Name Table:
#define MANAGER_TYPE_0           map
#define MANAGER_TYPE_1           flat_map
#define MANAGER_TYPE_2           unordered_map
#define MANAGER_TYPE_3           btree_map

// Macro expansion for BURST_MANAGER and manager_type
#define MANAGER_CAT_DONE(a,b)    a##_##b
#define MANAGER_CAT_RELAY(a,b)   MANAGER_CAT_DONE(a,b)
#define MANAGER_CCAT(a,b)        MANAGER_CAT_RELAY(a, MANAGER_TYPE_ ## b)
#define MANAGER_CAT(a,b)         MANAGER_CCAT(a,b)

// Macro expansion for manager_map_type_name
#define MANAGER_STRINGIFY1(x)    #x
#define MANAGER_STRINGIFY(x)     MANAGER_STRINGIFY1(x)
#define MANAGER_TYPE_NAME1(x)    MANAGER_STRINGIFY(MANAGER_TYPE_ ## x)
//////////////////////////////////////////////
///////////////// DETAILS ////////////////////
//////////////////////////////////////////////

// Constructs the name of the manager class
#define BURST_MANAGER            MANAGER_CAT(burst_manager,LOCAL_MAP_TYPE)

// Constructs type given key_type, pay_type, MAP_TYPE
#define manager_type(k, p, m)    MANAGER_CAT(burst_manager,m)<k,p>

// Quoted name of the map type
#define manager_map_type_name(x) MANAGER_TYPE_NAME1(x)


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
#include <boost/functional/hash.hpp>

// If a particular MAP_TYPE wasn't selected, build them all
#ifndef LOCAL_MAP_TYPE

#define LOCAL_MAP_TYPE MAP_TYPE_MAP
#include "manager_modified.h"
#undef  LOCAL_MAP_TYPE
#define LOCAL_MAP_TYPE MAP_TYPE_FLAT_MAP
#include "manager_modified.h"
#undef  LOCAL_MAP_TYPE
#define LOCAL_MAP_TYPE MAP_TYPE_UNORDERED_MAP
#include "manager_modified.h"
#undef  LOCAL_MAP_TYPE
#define LOCAL_MAP_TYPE MAP_TYPE_BTREE_MAP
#include "manager_modified.h"
#undef  LOCAL_MAP_TYPE

#endif

#endif  // MANAGER_JDY_20120529_FIRST_CALL

// Main include guard:
#if   (LOCAL_MAP_TYPE == MAP_TYPE_MAP            && !MANAGER_MODIFIED_MAP_DONE)           || \
      (LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP       && !MANAGER_MODIFIED_FLAT_MAP_DONE)      || \
      (LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP  && !MANAGER_MODIFIED_UNORDERED_MAP_DONE) || \
      (LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP      && !MANAGER_MODIFIED_BTREE_MAP_DONE)

      #if   LOCAL_MAP_TYPE == MAP_TYPE_MAP && !MANAGER_MODIFIED_MAP_DONE
      #define MANAGER_MODIFIED_MAP_DONE           true
      #endif
      #if   LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP && !MANAGER_MODIFIED_FLAT_MAP_DONE
      #define MANAGER_MODIFIED_FLAT_MAP_DONE      true
      #endif
      #if   LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP && !MANAGER_MODIFIED_UNORDERED_MAP_DONE
      #define MANAGER_MODIFIED_UNORDERED_MAP_DONE true
      #endif
      #if   LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP && !MANAGER_MODIFIED_BTREE_MAP_DONE
      #define MANAGER_MODIFIED_BTREE_MAP_DONE     true
      #endif

#if   LOCAL_MAP_TYPE != MAP_TYPE_BTREE_MAP
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#endif

#if   LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
#include <map>
#include <boost/interprocess/containers/flat_map.hpp>
#elif LOCAL_MAP_TYPE == MAP_TYPE_MAP
#include <boost/interprocess/containers/map.hpp>
#elif LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
#include <boost/unordered/unordered_map.hpp>
#elif LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
#include <boost/btree/header.hpp>
#include <boost/btree/map.hpp>
#include <boost/btree/support/strbuf.hpp>
#endif

// managed the mapped file during creation.  Allows for growing the 
// mapped file.
//
// KEY_T must be something that is a lot like md5_t (nothing with pointers)
// both KEY_T and PAY_T must not use dynamic memory
template<typename KEY_T, typename PAY_T>

class BURST_MANAGER {
  public:
    typedef BURST_MANAGER<KEY_T, PAY_T>              this_t;

    typedef KEY_T                                    key_t;
    typedef PAY_T                                    pay_t;

  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
    typedef boost::btree::btree_map<
              key_t, pay_t>                          map_t;
    typedef typename map_t::value_type               val_t;
  #else
  private:
    typedef boost::interprocess::managed_mapped_file segment_t;
    typedef segment_t::segment_manager               segment_manager_t;

  public:
    // Entry stored by map
    #if     LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
    typedef std::pair<      key_t, pay_t>            val_t;
    #elif   LOCAL_MAP_TYPE == MAP_TYPE_MAP || \
            LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
    typedef std::pair<const key_t, pay_t>            val_t;
    #endif

    // Allocator for pairs to be stored in map
    typedef boost::interprocess::allocator<
              val_t, segment_manager_t>              allocator_t;
      
    // Map to be stored in memory mapped file
    #if     LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
    typedef boost::interprocess::flat_map<
              key_t, pay_t, std::less<key_t>, 
              allocator_t>                           map_t;
    #elif   LOCAL_MAP_TYPE == MAP_TYPE_MAP
    typedef boost::interprocess::map<
              key_t, pay_t, std::less<key_t>, 
              allocator_t>                           map_t;
    #elif   LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
    typedef boost::unordered_map<
              key_t, pay_t, 
              boost::hash<key_t>, std::equal_to<key_t>, 
              allocator_t>                           map_t;
    #endif
  #endif

    typedef typename map_t::iterator                 map_iterator;
    typedef typename map_t::const_iterator           map_const_iterator;

  protected:
    std::string  name;
    std::size_t  expected_size;              // expected size of container
    std::string  data_type_name;
    file_mode_type_t file_mode;
    uint8_t      shard_count;
    std::size_t  shift_bits;

  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
  #else
    typedef std::vector<segment_t*>                  segments_t;
    typedef std::vector<allocator_t*>                allocators_t;

    segments_t   segments;
    allocators_t allocators;
  #endif

    typedef std::vector<map_t*>                      maps_t;
    typedef std::vector<std::size_t>                 sizes_t;

    maps_t       maps;
    sizes_t      sizes;

  public:

    // access to new store based on file_mode_type_t in hashdb_types.h,
    // specifically: READ_ONLY, RW_NEW, RW_MODIFY
    BURST_MANAGER(
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
         ,shard_count(p_shard_count)
         ,shift_bits(calc_bits(shard_count))
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
  #else
         ,segments(segments_t(shard_count,(segment_t*)(0)))
         ,allocators(allocators_t(shard_count,(allocator_t*)(0)))
  #endif
         ,maps(maps_t(shard_count,(map_t*)(0)))
         ,sizes(shard_count,p_size) 
    {
      for (uint8_t i = 0; i < shard_count; ++i) {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
        if (file_mode == READ_ONLY) {
          openone_read_only(namer(name,i),maps[i],sizes[i]);
        } else {
          openone(namer(name,i),maps[i],sizes[i]);
        }
  #else
        if (file_mode == READ_ONLY) {
          openone_read_only(namer(name,i),segments[i],allocators[i],maps[i],sizes[i]);
        } else {
          openone(namer(name,i),segments[i],allocators[i],maps[i],sizes[i]);
        }
  #endif
      }
    }

    ~BURST_MANAGER() {
      for (uint8_t map=0; map < shard_count; ++map) {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
        if (file_mode != READ_ONLY) {
          // alert user start
          std::cout << "Packing btree shard " << (uint32_t)map << " ...\n";

          // pack btree to .scratch
          map_t db_final(namer(name,map)+".scratch", boost::btree::flags::truncate);
          for (map_const_iterator i = maps[map]->begin(); i != maps[map]->end(); ++i) {
            db_final.emplace(i->key(),i->mapped_value());
          }
          delete maps[map];
          // rename .scratch back to existing btree, replacing existing btree
          std::rename((namer(name,map)+".scratch").c_str(), namer(name,map).c_str());

          // alert user done
          std::cout << "Packing btree shard " << (uint32_t)map << " completed.\n";
        }
 
  #else
        #if       LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
        if (file_mode != READ_ONLY) {
          maps[map]->shrink_to_fit();
        }
        #endif
        // Don't delete the map as it needs to live inside the mapped file
        delete allocators[map];
        delete segments[map];
        if (file_mode != READ_ONLY) {
          segment_t::shrink_to_fit(namer(name,map).c_str());
        }
  #endif
      }
    }

//    /*
//     * Pack when the map is expected to only be used for reading.
//     */
//    void pack() {
//      // validate mode
//      if (file_mode == READ_ONLY) {
//        assert(0);
//      }
//
//      for (uint8_t map=0; map < shard_count; ++map) {
//  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
//        // pack btree to .scratch
//        map_t db_final(namer(name,map)+".scratch", boost::btree::flags::truncate);
//        for (map_const_iterator i = maps[map]->begin(); i != maps[map]->end(); ++i) {
//          db_final.emplace(i->key(),i->mapped_value());
//        }
//        delete maps[map];
//        // rename .scratch back to existing btree, replacing existing btree
//        std::rename((namer(name,map)+".scratch").c_str(), namer(name,map).c_str());
//      }
//  #else
//    #if       LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
//      maps[map]->shrink_to_fit();
//    #endif
//      segment_t::shrink_to_fit(namer(name,map).c_str());
//  #endif
//    }
 

  private:
    std::size_t calc_bits(std::size_t bits) {
      return 8 - std::tr1::log2(bits);
    }

    // generate filename for segment
    std::string namer(const std::string& base, uint8_t val) {
      std::ostringstream ss;
      ss << base << "." << (int)val;
      return ss.str();
    }
  
    void openone_read_only(
        const std::string& _name
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
  #else
       ,segment_t*&        segment
       ,allocator_t*&      allocator
  #endif
       ,map_t*&            map
       ,std::size_t&       _size
      ) {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
    map       = new map_t(_name, boost::btree::flags::read_only);
      map->max_cache_size(65536);
  #else
      segment   = new segment_t(
                        boost::interprocess::open_read_only, _name.c_str()
                      );
      _size      = segment->get_size();
      allocator = new allocator_t(segment->get_segment_manager());
      map       = segment->find<map_t>(data_type_name.c_str()).first; 
  #endif

    }

    void openone(
        const std::string& _name
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
  #else
       ,segment_t*&        segment
       ,allocator_t*&      allocator
  #endif
       ,map_t*&            map
       ,std::size_t&       _size
      ) {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      if (file_mode == RW_NEW) {
        map       = new map_t(_name, boost::btree::flags::truncate);
      } else if (file_mode == RW_MODIFY) {
        
        map       = new map_t(_name, boost::btree::flags::read_write);
      } else {
        assert(0);
      }
  #else
      segment   = new segment_t(
                    boost::interprocess::open_or_create, _name.c_str(), _size
                  );
//      try {
      _size      = segment->get_size();
//      } catch (std::exception e) {
//        std::cout << "manager_modified openone exception\n";
//        throw e;
//      }
      allocator = new allocator_t(segment->get_segment_manager());

      // if file isn't big enough, we need to grow the file and 
      // (recursively through grow) retry.
      try {
        #if    LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
        map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (expected_size/shard_count, boost::hash<key_t>(), std::equal_to<key_t>(), *allocator);
        #elif  LOCAL_MAP_TYPE == MAP_TYPE_MAP || \
               LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
        map = segment->find_or_construct<map_t>(data_type_name.c_str())
                (std::less<key_t>(), *allocator);
        #endif
        #if       LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
        map->reserve(expected_size/shard_count);
        #endif
      } catch (...) {
        grow(_name,segment,allocator,map,_size); 
      }
  #endif
    }

  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
    
  #else
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
  #endif
  
  public:

    void emplace(const key_t& key, const pay_t& pay) {
      if (file_mode == READ_ONLY) {
        assert(0);
      }
      uint8_t i    = key.digest[0] >> shift_bits;
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      maps[i]->emplace(key,pay);
  #else
      bool    done = true;
      do { // if item doesn't exist, may need to grow map
        done = true;
        try {
          (*maps[i])[key] = pay;
        } catch(boost::interprocess::interprocess_exception& ex) {
          grow(namer(name,i),segments[i],allocators[i],maps[i],sizes[i]);
          done = false;
        }
      } while (not done);
  #endif

    }

    class manager_iterator {
      private:
        map_const_iterator m_itr;
        uint8_t            m_map;
        const this_t*      m_mgr;
 
      public:
        manager_iterator() : m_itr(),m_map(0),m_mgr(0) { }
        manager_iterator(const manager_iterator& other) :
                   m_itr(other.m_itr), m_map(other.m_map), m_mgr(other.m_mgr) {
        }

        explicit manager_iterator(map_const_iterator p, uint64_t map, const this_t* mgr) 
          : m_itr(p), m_map(map), m_mgr(mgr)
        {}

        key_t key() const {
          return m_mgr->get_key(*this); 
        }

        pay_t pay() const {
          return m_mgr->get_pay(*this);
        }

        manager_iterator& operator++() {
          ++m_itr; 
          if (m_itr == m_mgr->maps[m_map]->end()) {
            ++m_map;
            if (m_map < m_mgr->shard_count) {
              m_itr = m_mgr->maps[m_map]->begin();
            }
          }
          return *this;
        }

        manager_iterator& operator=(const manager_iterator& other) {
          m_itr = other.m_itr;
          m_map = other.m_map;
          m_mgr = other.m_mgr;
          return *this;
        }

        bool const operator==(const manager_iterator& other) const {
          return (m_itr == other.m_itr);
        }

        val_t const* operator->() const {
          return &(*m_itr);
        }
    };

    typedef manager_iterator citr_t;
    typedef manager_iterator const_iterator;

    const citr_t begin() const {
      return citr_t(maps[0]->begin(), 0, this);
    }

    const citr_t end() const {
      return citr_t(maps[shard_count-1]->end(), shard_count, this);
    }

    citr_t find(const key_t& key) const {
        uint8_t            map = key.digest[0]>>shift_bits;
        map_const_iterator itr = maps[map]->find(key);
        if (itr == maps[map]->end()) {
          return end();
        } else {
          return citr_t(itr,map,this);
        }
    }


    key_t get_key(const citr_t& i) const {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      return i->key();
  #else
      return i->first;
  #endif
    }

    pay_t get_pay(const citr_t& i) const {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      return i->mapped_value();
  #else
      return i->second;
  #endif
    }

    std::size_t size() const {
      std::size_t _size = 0;
      for (uint8_t i = 0; i < shard_count; ++i) {
        _size += maps[i]->size();
      }
      return _size;
    }

    // keys should be sorted before calling for best performance
    // except for if unordered_map then it won't matter
    std::vector<citr_t> check_list(const std::vector<key_t>& keys) const {
      std::vector<citr_t> results;

      for (typename std::vector<key_t>::const_iterator i = keys.begin();
           i != keys.end(); 
           ++i) {
        citr_t item = find(*i);
        if (item != end()) {
          results.push_back(item);
        }
      } 

      return results;
    }

    void report_status(std::ostream& os) const {
      os << "hash store status: ";
      #if   LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
      os << "map type=hash";
      os << ", element count=" << size() << "\n";
      for (uint32_t i = 0; i < shard_count; ++i) {
        os << "shard " << i << ": ";
        os << "elements=" << maps[i]->size();
        os << ", bytes=" << sizes[i];
        os << ", bucket count=" << maps[i]->bucket_count();
        os << ", max bucket count=" << maps[i]->max_bucket_count();
        os << ", load factor=" << maps[i]->load_factor();
        os << ", max load factor=" << maps[i]->max_load_factor();
        os << "\n";
      }
      #elif LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
      os << "map type=sorted-vector";
      os << ", element count=" << size() << "\n";
      for (uint32_t i = 0; i < shard_count; ++i) {
        os << "shard " << i << ": ";
        os << "elements=" << maps[i]->size();
        os << ", bytes=" << sizes[i];
        os << ", capacity=" << maps[i]->capacity();
        os << "\n";
      }
      #elif LOCAL_MAP_TYPE == MAP_TYPE_MAP
      os << "map type=red-black-tree";
      os << ", element count=" << size() << "\n";
      for (uint32_t i = 0; i < shard_count; ++i) {
        os << "shard " << i << ": ";
        os << "elements=" << maps[i]->size();
        os << ", bytes=" << sizes[i];
        os << "\n";
      }
      #elif LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      os << "map type=btree";
      os << ", element count=" << size() << "\n";
      for (uint32_t i = 0; i < shard_count; ++i) {
        os << "shard " << i << ": ";
        os << "elements=" << maps[i]->size();
        os << ", node size=" << maps[i]->node_size();
        os << ", max cache size=" << maps[i]->max_cache_size();
        os << "\n";
      }
      #endif
    }

    void report_status(dfxml_writer& x) const {
      x.push("hash_store_status");
      #if   LOCAL_MAP_TYPE == MAP_TYPE_UNORDERED_MAP
      x.xmlout("map_type", "hash");
      x.xmlout("element_count",size());
      for (uint32_t i = 0; i < shard_count; ++i) {
        std::stringstream s;
        s << "index='" << i << "'";
        x.push("shard", s.str());
        x.xmlout("elements",         maps[i]->size());
        x.xmlout("bytes",            sizes[i]);
        x.xmlout("bucket_count",     maps[i]->bucket_count());
        x.xmlout("max_bucket_count", maps[i]->max_bucket_count());
        x.xmlout("load_factor",      maps[i]->load_factor());
        x.xmlout("max_load_factor",  maps[i]->max_load_factor());
        x.pop();
      }
      x.pop();
      #elif LOCAL_MAP_TYPE == MAP_TYPE_FLAT_MAP
      x.xmlout("map_type", "sorted-vector");
      x.xmlout("element_count",size());
      for (uint32_t i = 0; i < shard_count; ++i) {
        std::stringstream s;
        s << "index='" << i << "'";
        x.push("shard", s.str());
        x.xmlout("elements", maps[i]->size());
        x.xmlout("bytes",    sizes[i]);
        x.xmlout("capacity", maps[i]->capacity());
        x.pop();
      }
      x.pop();
      #elif LOCAL_MAP_TYPE == MAP_TYPE_MAP
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
      #elif LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      x.xmlout("map_type", "btree");
      x.xmlout("element_count",size());
      for (uint32_t i = 0; i < shard_count; ++i) {
        std::stringstream s;
        s << "index='" << i << "'";
        x.push("shard", s.str());
        x.xmlout("elements",       maps[i]->size());
        x.xmlout("node_size",      maps[i]->node_size());
        x.xmlout("max_cache_size", maps[i]->max_cache_size());
        x.pop();
      }
      x.pop();
      #endif
    }

    // ************************************************************
    // glue for readers
    // ************************************************************
    /**
     * Determine if key is present.
     */
    bool has_key(const key_t& key, pay_t& pay) const {
      uint8_t            map = key.digest[0]>>shift_bits;
//      typename T::const_iterator itr = maps[map]->find(key);
      map_const_iterator itr = maps[map]->find(key);
      if (itr == maps[map]->end()) {
        return false;
      } else {
  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
        // BTree uses mapped_value
        pay = itr->mapped_value();
  #else
        pay = itr->second;
  #endif
        return true;
      }
    }

    /**
     * Insert {key, value} into map.
     */
    void insert_element(const key_t& key,
                          const pay_t& pay) {
//std::cout << "mm:Insert to hash store: key: " << key << " pay: " << pay << "\n";
      // it is a program error for the key to already be there
      pay_t existing_pay;
      if (has_key(key, existing_pay)) {
        assert(0);
      }
      emplace(key, pay);
    }

    /**
     * Remove element else fail.
     */
    void erase_key(const key_t& key) {
//std::cout << "mm:Remove from hash store: key: " << key << "\n";
      uint8_t            map = key.digest[0]>>shift_bits;
      size_t num_erased = maps[map]->erase(key);
      if (num_erased != 1) {
        assert(0);
      }
    }

    /**
     * Get the value associated with the key else fail.
     */
/*
    void get_pay(const key_t& key,
                             pay_t &pay) {
      uint8_t map = key.digest[0]>>shift_bits;
      map_const_iterator itr = maps[map]->find(key);

      // it is an error for the element to not exist
      if (itr == maps[map]->end()) {
        assert(0);
      }

  #if LOCAL_MAP_TYPE == MAP_TYPE_BTREE_MAP
      // BTree uses mapped_value
      pay = itr->mapped_value();
  #else
      pay = itr->second;
  #endif
    }
*/

    /**
     * Change the existing value to a new value in the map,
     * failing if the element to be changed does not exist.
     */
    void change_pay(const key_t& key,
                          const pay_t& pay) {

      // erase the old element, this makes sure it existed
      erase_key(key);

      // put in the new value
      insert_element(key, pay);
    }
};

#endif // Main include guard

