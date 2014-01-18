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
 * Provides interfaces to the hash map store using glue to the actual
 * storage maps used.
 */

#ifndef MAP_MANAGER_HPP
#define MAP_MANAGER_HPP
#include "map_btree.hpp"
#include "map_flat_sorted_vector.hpp"
#include "map_red_black_tree.hpp"
#include "map_unordered_hash.hpp"
#include "multimap_btree.hpp"
#include "multimap_flat_sorted_vector.hpp"
#include "multimap_red_black_tree.hpp"
#include "multimap_unordered_hash.hpp"
#include "file_modes.h"
#include "map_types.h"
#include "dfxml/src/hash_t.h"
#include "hash_algorithm_types.h"
//#include "map_manager_iterator.hpp"
#include <boost/functional/hash.hpp>

// provide these for the unordered hash map and multimap
inline std::size_t hash_value(const md5_t& key) {
  return boost::hash_value<unsigned char,16>(key.digest);
}
inline std::size_t hash_value(const sha1_t& key) {
  return boost::hash_value<unsigned char,20>(key.digest);
}
inline std::size_t hash_value(const sha256_t& key) {
  return boost::hash_value<unsigned char,32>(key.digest);
}

/**
 * Provides interfaces to the hash map store that use glue to the actual
 * storage maps used.
 */
class map_manager_t {

  private:
  enum storage_type_t {
    BTREE_MD5,
    FLAT_SORTED_VECTOR_MD5,
    RED_BLACK_TREE_MD5,
    UNORDERED_HASH_MD5,
    BTREE_SHA1,
    FLAT_SORTED_VECTOR_SHA1,
    RED_BLACK_TREE_SHA1,
    UNORDERED_HASH_SHA1,
    BTREE_SHA256,
    FLAT_SORTED_VECTOR_SHA256,
    RED_BLACK_TREE_SHA256,
    UNORDERED_HASH_SHA256
  };

  // get storage type for internal usage
  static storage_type_t get_storage_type(map_type_t map_type, hash_algorithm_type_t hash_algorithm_type) {
    if (map_type == MAP_BTREE && hash_algorithm_type == HASH_ALGORITHM_MD5) return BTREE_MD5;
    if (map_type == MAP_FLAT_SORTED_VECTOR && hash_algorithm_type == HASH_ALGORITHM_MD5) return FLAT_SORTED_VECTOR_MD5;
    if (map_type == MAP_RED_BLACK_TREE && hash_algorithm_type == HASH_ALGORITHM_MD5) return RED_BLACK_TREE_MD5;
    if (map_type == MAP_UNORDERED_HASH && hash_algorithm_type == HASH_ALGORITHM_MD5) return UNORDERED_HASH_MD5;

    if (map_type == MAP_BTREE && hash_algorithm_type == HASH_ALGORITHM_SHA1) return BTREE_SHA1;
    if (map_type == MAP_FLAT_SORTED_VECTOR && hash_algorithm_type == HASH_ALGORITHM_SHA1) return FLAT_SORTED_VECTOR_SHA1;
    if (map_type == MAP_RED_BLACK_TREE && hash_algorithm_type == HASH_ALGORITHM_SHA1) return RED_BLACK_TREE_SHA1;
    if (map_type == MAP_UNORDERED_HASH && hash_algorithm_type == HASH_ALGORITHM_SHA1) return UNORDERED_HASH_SHA1;

    if (map_type == MAP_BTREE && hash_algorithm_type == HASH_ALGORITHM_SHA256) return BTREE_SHA256;
    if (map_type == MAP_FLAT_SORTED_VECTOR && hash_algorithm_type == HASH_ALGORITHM_SHA256) return FLAT_SORTED_VECTOR_SHA256;
    if (map_type == MAP_RED_BLACK_TREE && hash_algorithm_type == HASH_ALGORITHM_SHA256) return RED_BLACK_TREE_SHA256;
    if (map_type == MAP_UNORDERED_HASH && hash_algorithm_type == HASH_ALGORITHM_SHA256) return UNORDERED_HASH_SHA256;

    assert(0);
  }

  // hash_map_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;
  const storage_type_t storage_type;

public:
  // md5 map models
  map_btree_t<md5_t, uint64_t>*                 map_btree_md5;
  map_flat_sorted_vector_t<md5_t, uint64_t>*    map_flat_sorted_vector_md5;
  map_red_black_tree_t<md5_t, uint64_t>*        map_red_black_tree_md5;
  map_unordered_hash_t<md5_t, uint64_t>*        map_unordered_hash_md5;

  // sha1 map models
  map_btree_t<sha1_t, uint64_t>*                map_btree_sha1;
  map_flat_sorted_vector_t<sha1_t, uint64_t>*   map_flat_sorted_vector_sha1;
  map_red_black_tree_t<sha1_t, uint64_t>*       map_red_black_tree_sha1;
  map_unordered_hash_t<sha1_t, uint64_t>*       map_unordered_hash_sha1;

  // sha256 map models
  map_btree_t<sha256_t, uint64_t>*              map_btree_sha256;
  map_flat_sorted_vector_t<sha256_t, uint64_t>* map_flat_sorted_vector_sha256;
  map_red_black_tree_t<sha256_t, uint64_t>*     map_red_black_tree_sha256;
  map_unordered_hash_t<sha256_t, uint64_t>*     map_unordered_hash_sha256;
private:

  // disallow copy and assignment methods
  map_manager_t(const map_manager_t&);
  map_manager_t& operator=(const map_manager_t&);

  public:

  /**
   * Create a hash store of the given map type and file mode type.
   */
  map_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode,
                 map_type_t p_map_type,
                 hash_algorithm_type_t p_hash_algorithm_type) :
       filename(p_hashdb_dir + "/hash_store"),
       file_mode(p_file_mode),
       storage_type(get_storage_type(p_map_type, p_hash_algorithm_type)),

       map_btree_md5(0),
       map_flat_sorted_vector_md5(0),
       map_red_black_tree_md5(0),
       map_unordered_hash_md5(0),
       map_btree_sha1(0),
       map_flat_sorted_vector_sha1(0),
       map_red_black_tree_sha1(0),
       map_unordered_hash_sha1(0),
       map_btree_sha256(0),
       map_flat_sorted_vector_sha256(0),
       map_red_black_tree_sha256(0),
       map_unordered_hash_sha256(0) {

    switch(storage_type) {
      case BTREE_MD5:
           map_btree_md5 = new map_btree_t<md5_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_MD5:
           map_flat_sorted_vector_md5 = new map_flat_sorted_vector_t<md5_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_MD5:
           map_red_black_tree_md5 = new map_red_black_tree_t<md5_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_MD5:
           map_unordered_hash_md5 = new map_unordered_hash_t<md5_t, uint64_t>(filename, file_mode); return;

      case BTREE_SHA1:
           map_btree_sha1 = new map_btree_t<sha1_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_SHA1:
           map_flat_sorted_vector_sha1 = new map_flat_sorted_vector_t<sha1_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_SHA1:
           map_red_black_tree_sha1 = new map_red_black_tree_t<sha1_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_SHA1:
           map_unordered_hash_sha1 = new map_unordered_hash_t<sha1_t, uint64_t>(filename, file_mode); return;

      case BTREE_SHA256:
           map_btree_sha256 = new map_btree_t<sha256_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_SHA256:
           map_flat_sorted_vector_sha256 = new map_flat_sorted_vector_t<sha256_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_SHA256:
           map_red_black_tree_sha256 = new map_red_black_tree_t<sha256_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_SHA256:
           map_unordered_hash_sha256 = new map_unordered_hash_t<sha256_t, uint64_t>(filename, file_mode); return;

      default:
        assert(0);
    }
  }

  ~map_manager_t() {
    switch(storage_type) {
      case BTREE_MD5: delete map_btree_md5; return;
      case FLAT_SORTED_VECTOR_MD5: delete map_flat_sorted_vector_md5; return;
      case RED_BLACK_TREE_MD5: delete map_red_black_tree_md5; return;
      case UNORDERED_HASH_MD5: delete map_unordered_hash_md5; return;

      case BTREE_SHA1: delete map_btree_sha1; return;
      case FLAT_SORTED_VECTOR_SHA1: delete map_flat_sorted_vector_sha1; return;
      case RED_BLACK_TREE_SHA1: delete map_red_black_tree_sha1; return;
      case UNORDERED_HASH_SHA1: delete map_unordered_hash_sha1; return;

      case BTREE_SHA256: delete map_btree_sha256; return;
      case FLAT_SORTED_VECTOR_SHA256: delete map_flat_sorted_vector_sha256; return;
      case RED_BLACK_TREE_SHA256: delete map_red_black_tree_sha256; return;
      case UNORDERED_HASH_SHA256: delete map_unordered_hash_sha256; return;

      default:
        assert(0);
    }
  }

/*
  // insert
  std::pair<map_manager_iterator_md5, bool> insert(const& md5_t md5, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->insert(md5, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->insert(md5, source_lookup_encoding);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->insert(md5, source_lookup_encoding);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->insert(md5, source_lookup_encoding);

      default:
        assert(0);
    }
  }
  std::pair<map_manager_iterator_sha1, bool> insert(const& sha1_t sha1, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->insert(sha1, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->insert(sha1, source_lookup_encoding);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->insert(sha1, source_lookup_encoding);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->insert(sha1, source_lookup_encoding);

      default:
        assert(0);
    }
  }
  std::pair<map_manager_iterator_sha256, bool> insert(const& sha256_t sha256, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->insert(sha256, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->insert(sha256, source_lookup_encoding);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->insert(sha256, source_lookup_encoding);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->insert(sha256, source_lookup_encoding);

      default:
        assert(0);
    }
  }
*/

  // erase
  size_t erase(const md5_t& key) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->erase(key);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->erase(key);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->erase(key);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->erase(key);

      default:
        assert(0);
    }
  }
  size_t erase(const sha1_t& key) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->erase(key);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->erase(key);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->erase(key);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->erase(key);

      default:
        assert(0);
    }
  }
  size_t erase(const sha256_t& key) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->erase(key);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->erase(key);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->erase(key);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->erase(key);

      default:
        assert(0);
    }
  }

/*
  // change
  std::pair<map_manager_iterator_md5, bool> change(const& md5_t md5, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->change(md5, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->change(md5, source_lookup_encoding);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->change(md5, source_lookup_encoding);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->change(md5, source_lookup_encoding);

      default:
        assert(0);
    }
  }
  std::pair<map_manager_iterator_sha1, bool> change(const& sha1_t sha1, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->change(sha1, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->change(sha1, source_lookup_encoding);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->change(sha1, source_lookup_encoding);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->change(sha1, source_lookup_encoding);

      default:
        assert(0);
    }
  }
  std::pair<map_manager_iterator_sha256, bool> change(const& sha256_t sha256, uint64_t source_lookup_encoding) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->change(sha256, source_lookup_encoding);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->change(sha256, source_lookup_encoding);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->change(sha256, source_lookup_encoding);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->change(sha256, source_lookup_encoding);

      default:
        assert(0);
    }
  }
*/

/*
  // find
  map_manager_iterator_md5 find(const md5_t& pay) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->find(pay);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->find(pay);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->find(pay);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->find(pay);

      default:
        assert(0);
    }
  }
  map_manager_iterator_sha1 find(const sha1_t& pay) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->find(pay);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->find(pay);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->find(pay);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->find(pay);

      default:
        assert(0);
    }
  }
  map_manager_iterator_sha256 find(const sha256_t& pay) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->find(pay);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->find(pay);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->find(pay);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->find(pay);

      default:
        assert(0);
    }
  }
*/

  // has
  bool has(const md5_t& pay) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->has(pay);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->has(pay);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->has(pay);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->has(pay);

      default:
        assert(0);
    }
  }
  bool has(const sha1_t& pay) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->has(pay);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->has(pay);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->has(pay);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->has(pay);

      default:
        assert(0);
    }
  }
  bool has(const sha256_t& pay) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->has(pay);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->has(pay);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->has(pay);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->has(pay);

      default:
        assert(0);
    }
  }

  // size
  size_t size() {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->size();
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->size();
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->size();
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->size();

      case BTREE_SHA1: return map_btree_sha1->size();
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->size();
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->size();
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->size();

      case BTREE_SHA256: return map_btree_sha256->size();
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->size();
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->size();
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->size();

      default:
        assert(0);
    }
  }

/*
  // map stats
inline std::ostream& operator<<(std::ostream& os,
        const class map_stats_t& s) {
  os << "(filename=" << s.filename
     << ", file_mode=" << file_mode_type_to_string(s.file_mode)
     << ", data type name=" << s.data_type_name
     << ", segment size=" << s.segment_size
     << ", count size=" << s.count_size
     << ")";
  return os;
}

  void report_stats(dfxml_writer& x) const {
    x.push("map_stats");
    x.xmlout("filename", filename);
    x.xmlout("file_mode", file_mode_type_to_string(file_mode));
    x.xmlout("data_type_name", data_type_name);
    x.xmlout("segment_size",segment_size);
    x.xmlout("count_size",count_size);
    x.pop();
  }
*/

//  const begin() const;
//  const end() const;

};
#endif
