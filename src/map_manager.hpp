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
#include "file_types.h"
#include "digest_types.h"
#include <boost/iterator/iterator_facade.hpp>

// provide this for the unordered hash map and multimap
inline std::size_t hash_value(const md5_t& md5) {
  return boost::hash_value<unsigned char,16>(md5.digest);
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
  }

  // get internal type
  static storage_type_t get_storage_type(map_type_t map_type, digest_type_t digest_type) {
    if (map_type == MAP_BTREE && digest_type == DIGEST_MD5) return BTREE_MD5;
    if (map_type == MAP_FLAT_SORTED_VECTOR && digest_type == DIGEST_MD5) return FLAT_SORTED_VECTOR_MD5;
    if (map_type == MAP_RED_BLACK_TREE && digest_type == DIGEST_MD5) return RED_BLACK_TREE_MD5;
    if (map_type == MAP_UNORDERED_HASH && digest_type == DIGEST_MD5) return UNORDERED_HASH_MD5;

    if (map_type == MAP_BTREE && digest_type == DIGEST_SHA1) return BTREE_SHA1;
    if (map_type == MAP_FLAT_SORTED_VECTOR && digest_type == DIGEST_SHA1) return FLAT_SORTED_VECTOR_SHA1;
    if (map_type == MAP_RED_BLACK_TREE && digest_type == DIGEST_SHA1) return RED_BLACK_TREE_SHA1;
    if (map_type == MAP_UNORDERED_HASH && digest_type == DIGEST_SHA1) return UNORDERED_HASH_SHA1;

    if (map_type == MAP_BTREE && digest_type == DIGEST_SHA256) return BTREE_SHA256;
    if (map_type == MAP_FLAT_SORTED_VECTOR && digest_type == DIGEST_SHA256) return FLAT_SORTED_VECTOR_SHA256;
    if (map_type == MAP_RED_BLACK_TREE && digest_type == DIGEST_SHA256) return RED_BLACK_TREE_SHA256;
    if (map_type == MAP_UNORDERED_HASH && digest_type == DIGEST_SHA256) return UNORDERED_HASH_SHA256;

    assert(0);
  }

  // hash_map_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;
  const storage_type_t storage_type;

  // md5 map models
  map_btree_t<md5_t, uint64_t>*                 map_btree_md5;
  map_red_black_tree_t<md5_t, uint64_t>*        map_red_black_tree_md5;
  map_flat_sorted_vector_t<md5_t, uint64_t>*    map_flat_sorted_vector_md5;
  map_unordered_hash_t<md5_t, uint64_t>*        map_unordered_hash_md5;

  // sha1 map models
  map_btree_t<sha1_t, uint64_t>*                map_btree_sha1;
  map_red_black_tree_t<sha1_t, uint64_t>*       map_red_black_tree_sha1;
  map_flat_sorted_vector_t<sha1_t, uint64_t>*   map_flat_sorted_vector_sha1;
  map_unordered_hash_t<sha1_t, uint64_t>*       map_unordered_hash_sha1;

  // sha256 map models
  map_btree_t<sha256_t, uint64_t>*              map_btree_sha256;
  map_flat_sorted_vector_t<sha256_t, uint64_t>* map_flat_sorted_vector_sha256;
  map_red_black_tree_t<sha256_t, uint64_t>*     map_red_black_tree_sha256;
  map_unordered_hash_t<sha256_t, uint64_t>*     map_unordered_hash_sha256;

  // disallow copy and assignment methods
  map_manager_t(const map_manager_t<KEY_T, PAY_T>&);
  map_manager_t& operator=(const map_manager_t<KEY_T, PAY_T>&);

  public:
  /**
   * Create a hash store of the given map type and file mode type.
   */
  map_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode,
                 map_type_t p_map_type,
                 digest_type_t p_digest_type) :
       filename(p_hashdb_dir + "/hash_store"),
       file_mode(p_file_mode),
       storage_type(p_map_type, p_digest_type) :

       map_btree_md5(0), map_red_black_tree_md5(0),
       map_flat_sorted_vector_md5(0), map_unordered_hash_md5(0),
       map_btree_sha1(0), map_red_black_tree_sha1(0),
       map_flat_sorted_vector_sha1(0), map_unordered_hash_sha1(0),
       map_btree_sha256(0), map_red_black_tree_sha256(0),
       map_flat_sorted_vector_sha256(0), map_unordered_hash_sha256(0) {

    switch(storage_type) {
      case BTREE_MD5: map_btree_md5 =
           new map_btree_t<md5_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_MD5: map_flat_sorted_vector_md5 =
           new map_flat_sorted_vector<md5_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_MD5: map_red_black_tree_md5 =
           new map_red_black_tree_t<md5_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_MD5: map_unordered_hash_md5 =
           new map_unordered_hash_t<md5_t, uint64_t>(filename, file_mode); return;

      case BTREE_SHA1: map_btree_sha1 =
           new map_btree_t<sha1_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_SHA1: flat_sorted_vector_sha1 =
           new map_flat_sorted_vector<sha1_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_SHA1: map_red_black_tree_sha1 =
           new map_red_black_tree_t<sha1_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_SHA1: map_unordered_hash_sha1 =
           new map_unordered_hash_t<sha1_t, uint64_t>(filename, file_mode); return;

      case BTREE_SHA256: map_btree_sha1 =
           new map_btree_t<sha256_t, uint64_t>(filename, file_mode); return;
      case FLAT_SORTED_VECTOR_SHA256: flat_sorted_vector_sha256 =
           new map_flat_sorted_vector<sha256_t, uint64_t>(filename, file_mode); return;
      case RED_BLACK_TREE_SHA256: map_red_black_tree_sha256 =
           new map_red_black_tree_t<sha1_t, uint64_t>(filename, file_mode); return;
      case UNORDERED_HASH_SHA1: map_unordered_hash_sha1 =
           new map_unordered_hash_t<sha1_t, uint64_t>(filename, file_mode); return;

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
      case FLAT_SORTED_VECTOR_SHA1: delete flat_sorted_vector_sha1; return;
      case RED_BLACK_TREE_SHA1: delete map_red_black_tree_sha1; return;
      case UNORDERED_HASH_SHA1: delete map_unordered_hash_sha1; return;

      case BTREE_SHA256: delete map_btree_sha256; return;
      case FLAT_SORTED_VECTOR_SHA256: delete flat_sorted_vector_sha256; return;
      case RED_BLACK_TREE_SHA256: delete map_red_black_tree_sha256; return;
      case UNORDERED_HASH_SHA256: delete map_unordered_hash_sha256; return;

      default:
        assert(0);
    }
  }

  // insert
  pair<map_manager_t::const_iterator, bool> insert(const& md5_t md5, uint64_t) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->insert(md5, uint64_t);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->insert(md5, uint64_t);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->insert(md5, uint64_t);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->insert(md5, uint64_t);

      default:
        assert(0);
    }
  }
  pair<map_manager_t::const_iterator, bool> insert(const& sha1_t sha1, uint64_t) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->insert(sha1, uint64_t);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->insert(sha1, uint64_t);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->insert(sha1, uint64_t);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->insert(sha1, uint64_t);

      default:
        assert(0);
    }
  }
  pair<map_manager_t::const_iterator, bool> insert(const& sha256_t sha256, uint64_t) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->insert(sha256, uint64_t);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->insert(sha256, uint64_t);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->insert(sha256, uint64_t);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->insert(sha256, uint64_t);

      default:
        assert(0);
    }
  }

  // erase
  size_t erase(const& md5_t) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->erase(md5);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->erase(md5);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->erase(md5);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->erase(md5);

      default:
        assert(0);
    }
  }
  size_t erase(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->erase(md5);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->erase(md5);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->erase(md5);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->erase(md5);

      default:
        assert(0);
    }
  }
  size_t erase(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->erase(md5);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->erase(md5);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->erase(md5);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->erase(md5);

      default:
        assert(0);
    }
  }

  // change
  pair<map_manager_t::const_iterator, bool> change(const& md5_t md5, uint64_t) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->change(md5, uint64_t);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->change(md5, uint64_t);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->change(md5, uint64_t);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->change(md5, uint64_t);

      default:
        assert(0);
    }
  }
  pair<map_manager_t::const_iterator, bool> change(const& sha1_t sha1, uint64_t) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->change(sha1, uint64_t);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->change(sha1, uint64_t);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->change(sha1, uint64_t);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->change(sha1, uint64_t);

      default:
        assert(0);
    }
  }
  pair<map_manager_t::const_iterator, bool> change(const& sha256_t sha256, uint64_t) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->change(sha256, uint64_t);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->change(sha256, uint64_t);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->change(sha256, uint64_t);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->change(sha256, uint64_t);

      default:
        assert(0);
    }
  }

  // find
  map_manager_t::const_iterator find(const& md5_t) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->find(md5);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->find(md5);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->find(md5);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->find(md5);

      default:
        assert(0);
    }
  }
  map_manager_t::const_iterator find(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->find(md5);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->find(md5);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->find(md5);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->find(md5);

      default:
        assert(0);
    }
  }
  map_manager_t::const_iterator find(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->find(md5);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->find(md5);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->find(md5);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->find(md5);

      default:
        assert(0);
    }
  }

  // has
  bool has(const& md5_t) {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->has(md5);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->has(md5);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->has(md5);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->has(md5);

      default:
        assert(0);
    }
  }
  bool has(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA1: return map_btree_sha1->has(md5);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->has(md5);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->has(md5);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->has(md5);

      default:
        assert(0);
    }
  }
  bool has(const& md5_t) {
    switch(storage_type) {
      case BTREE_SHA256: return map_btree_sha256->has(md5);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->has(md5);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->has(md5);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->has(md5);

      default:
        assert(0);
    }
  }

  // size
  size_t size() {
    switch(storage_type) {
      case BTREE_MD5: return map_btree_md5->size(md5);
      case FLAT_SORTED_VECTOR_MD5: return map_flat_sorted_vector_md5->size(md5);
      case RED_BLACK_TREE_MD5: return map_red_black_tree_md5->size(md5);
      case UNORDERED_HASH_MD5: return map_unordered_hash_md5->size(md5);

      case BTREE_SHA1: return map_btree_sha1->size(md5);
      case FLAT_SORTED_VECTOR_SHA1: return map_flat_sorted_vector_sha1->size(md5);
      case RED_BLACK_TREE_SHA1: return map_red_black_tree_sha1->size(md5);
      case UNORDERED_HASH_SHA1: return map_unordered_hash_sha1->size(md5);

      case BTREE_SHA256: return map_btree_sha256->size(md5);
      case FLAT_SORTED_VECTOR_SHA256: return map_flat_sorted_vector_sha256->size(md5);
      case RED_BLACK_TREE_SHA256: return map_red_black_tree_sha256->size(md5);
      case UNORDERED_HASH_SHA256: return map_unordered_hash_sha256->size(md5);

      default:
        assert(0);
    }
  }

//  const begin() const;
//  const end() const;

};
#endif
