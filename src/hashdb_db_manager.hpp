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
 * The hashdb manager provides access to the hashdb.
 */

#ifndef HASHDB_DB_MANAGER_HPP
#define HASHDB_DB_MANAGER_HPP
#include "hashdb_types.h"
#include "dfxml/src/hash_t.h"

#include "hashdb_filenames.hpp"
#include "settings_reader.hpp"
#include "hash_store.hpp"
#include "hash_duplicates_store.hpp"
#include "source_lookup_manager.hpp"
#include "bloom_filter.hpp"
#include <vector>
//#include <boost/iterator/iterator_facade.hpp>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cassert>

/**
 * The hashdb DB manager controls access to the hashdb
 * by providing services for accessing and updating the hashdb.
 *
 * Note that hashes are not added if count reaches maximum_hash_duplicates.
 */
class hashdb_db_manager_t {
  public:
    const std::string hashdb_dir;
    const file_mode_type_t file_mode_type;
    const hashdb_settings_t hashdb_settings;

    struct hash_changes_t {
      uint64_t hashes_inserted;
      uint64_t hashes_not_inserted_invalid_file_offset;
      uint64_t hashes_not_inserted_wrong_hash_block_size;
      uint64_t hashes_not_inserted_wrong_hashdigest_type;
      uint64_t hashes_not_inserted_exceeds_max_duplicates;
      uint64_t hashes_not_inserted_duplicate_source;

      uint64_t hashes_removed;
      uint64_t hashes_not_removed_invalid_file_offset;
      uint64_t hashes_not_removed_wrong_hash_block_size;
      uint64_t hashes_not_removed_wrong_hashdigest_type;
      uint64_t hashes_not_removed_no_hash;
      uint64_t hashes_not_removed_different_source;

      // create with all fields zero
      hash_changes_t() : hashes_inserted(0),
                         hashes_not_inserted_invalid_file_offset(0),
                         hashes_not_inserted_wrong_hash_block_size(0),
                         hashes_not_inserted_wrong_hashdigest_type(0),
                         hashes_not_inserted_exceeds_max_duplicates(0),
                         hashes_not_inserted_duplicate_source(0),

                         hashes_removed(0),
                         hashes_not_removed_invalid_file_offset(0),
                         hashes_not_removed_wrong_hash_block_size(0),
                         hashes_not_removed_wrong_hashdigest_type(0),
                         hashes_not_removed_no_hash(0),
                         hashes_not_removed_different_source(0) {
      }

      // copy constructor
      hash_changes_t(const hash_changes_t& hash_changes) :
         hashes_inserted(hash_changes.hashes_inserted),
         hashes_not_inserted_invalid_file_offset(hash_changes.hashes_not_inserted_invalid_file_offset),
         hashes_not_inserted_wrong_hash_block_size(hash_changes.hashes_not_inserted_wrong_hash_block_size),
         hashes_not_inserted_wrong_hashdigest_type(hash_changes.hashes_not_inserted_wrong_hashdigest_type),
         hashes_not_inserted_exceeds_max_duplicates(hash_changes.hashes_not_inserted_exceeds_max_duplicates),
         hashes_not_inserted_duplicate_source(hash_changes.hashes_not_inserted_duplicate_source),

         hashes_removed(hash_changes.hashes_removed),
         hashes_not_removed_invalid_file_offset(hash_changes.hashes_not_removed_invalid_file_offset),
         hashes_not_removed_wrong_hash_block_size(hash_changes.hashes_not_removed_wrong_hash_block_size),
         hashes_not_removed_wrong_hashdigest_type(hash_changes.hashes_not_removed_wrong_hashdigest_type),
         hashes_not_removed_no_hash(hash_changes.hashes_not_removed_no_hash),
         hashes_not_removed_different_source(hash_changes.hashes_not_removed_different_source) {
      }

      // generate reports to stream
      void report_insert_changes(std::ostream& os) const {
        os << "hashdb changes:\n"
           << "    hashes inserted=" << hashes_inserted << "\n"
           << "    hashes not inserted, invalid file offset=" << hashes_not_inserted_invalid_file_offset << "\n"
           << "    hashes not inserted, wrong hash block size=" << hashes_not_inserted_wrong_hash_block_size << "\n"
           << "    hashes not inserted, wrong hashdigest type=" << hashes_not_inserted_wrong_hashdigest_type << "\n"
           << "    hashes not inserted, exceeds max duplicates=" << hashes_not_inserted_exceeds_max_duplicates << "\n"
           << "    hashes not inserted, duplicate source=" << hashes_not_inserted_duplicate_source << "\n"
        ;
      }

      void report_remove_changes(std::ostream& os) const {
        os << "hashdb changes:\n"
           << "    hashes removed=" << hashes_removed << "\n"
           << "    hashes not removed, invalid file offset=" << hashes_not_removed_invalid_file_offset << "\n"
           << "    hashes not removed, wrong hash block size=" << hashes_not_removed_wrong_hash_block_size << "\n"
           << "    hashes not removed, wrong hashdigest type=" << hashes_not_removed_wrong_hashdigest_type << "\n"
           << "    hashes not removed, no hash=" << hashes_not_removed_no_hash << "\n"
           << "    hashes not removed, different source=" << hashes_not_removed_different_source << "\n"
        ;
      }

      // generate reports to dfxml
      void report_insert_changes(dfxml_writer& x) const {
        x.push("hashdb_changes");
        x.xmlout("hashes_inserted", hashes_inserted);
        x.xmlout("hashes_not_inserted_invalid_file_offset", hashes_not_inserted_invalid_file_offset);
        x.xmlout("hashes_not_inserted_wrong_hash_block_size", hashes_not_inserted_wrong_hash_block_size);
        x.xmlout("hashes_not_inserted_wrong_hashdigest_type", hashes_not_inserted_wrong_hashdigest_type);
        x.xmlout("hashes_not_inserted_exceeds_max_duplicates", hashes_not_inserted_exceeds_max_duplicates);
        x.xmlout("hashes_not_inserted_duplicate_source", hashes_not_inserted_duplicate_source);
        x.pop();
      }

      void report_remove_changes(dfxml_writer& x) const {
        x.push("hashdb_changes");
        x.xmlout("hashes_removed", hashes_removed);
        x.xmlout("hashes_not_removed_invalid_file_offset", hashes_not_removed_invalid_file_offset);
        x.xmlout("hashes_not_removed_wrong_hash_block_size", hashes_not_removed_wrong_hash_block_size);
        x.xmlout("hashes_not_removed_wrong_hashdigest_type", hashes_not_removed_wrong_hashdigest_type);
        x.xmlout("hashes_not_removed_no_hash", hashes_not_removed_no_hash);
        x.xmlout("hashes_not_removed_different_source", hashes_not_removed_different_source);
        x.pop();
      }
    };

    /**
     * Return runtime statistics about adds and deletes.
     */
    hash_changes_t get_hash_changes() {
      return hash_changes;
    }

  private:
    // runtime statistics about added and deleted hashes
    hash_changes_t hash_changes;

    // the hashdb settings that need retained
    bool use_bloom1;
    bool use_bloom2;

    // convenience variable
    number_of_index_bits_type_t number_of_index_bits_type;

    // hashdb components
    hash_store_t *hash_store;
    hash_duplicates_store_t *hash_duplicates_store;
    source_lookup_manager_t *source_lookup_manager;
    bloom_filter_t *bloom1;
    bloom_filter_t *bloom2;

    // do not allow these
    hashdb_db_manager_t(const hashdb_db_manager_t&);
    hashdb_db_manager_t& operator=(const hashdb_db_manager_t& that);

    /**
     * Obtain the hash source record corresponding to the source lookup record
     * else fail.
     */
    void source_lookup_record_to_hash_source_record(
             const source_lookup_record_t& source_lookup_record,
             hash_source_record_t& hash_source_record) const {

      // get the file offset
      uint64_t hash_block_offset_value =
                    source_lookup_record.hash_block_offset_value(
                         number_of_index_bits_type);
      uint64_t file_offset = hash_block_offset_value *
                         hashdb_settings.hash_block_size;

      // get the source lookup index
      uint64_t source_lookup_index = source_lookup_record.source_lookup_index(
                         number_of_index_bits_type);

      // get the repository name and the filename
      std::string repository_name;
      std::string filename;
      bool status = source_lookup_manager->get_source_location(
                        source_lookup_index, repository_name, filename);
      if (status != true) {
        // program error
        assert(0);
      }

      // now compose the hash source record
      hash_source_record = hash_source_record_t(
                                  file_offset,
                                  hashdb_settings.hash_block_size,
                                  hashdigest_type_to_string(hashdb_settings.hashdigest_type),
                                  repository_name,
                                  filename);
    }

    /**
     * Obtain the source lookup record corresponding to the repository name
     * and filename in the hash source record else false.
     */
    void hash_source_record_to_source_lookup_record(
             const hash_source_record_t& hash_source_record,
             source_lookup_record_t& source_lookup_record) const {

      uint64_t source_lookup_index;
      bool status = source_lookup_manager->get_source_lookup_index(
                                    hash_source_record.repository_name,
                                    hash_source_record.filename,
                                    source_lookup_index);

      if (status == false) {
        // program error
        assert(0);
      }

      // validate the file offset % hash block size
      if (hash_source_record.file_offset % hashdb_settings.hash_block_size != 0) {
        // program error
        assert(0);
      }

      // calculate hash block offset
      uint64_t hash_block_offset =
                 hash_source_record.file_offset / hashdb_settings.hash_block_size;

      // now compose the source lookup record
      source_lookup_record_t composed_source_lookup_record(
               number_of_index_bits_type,
               source_lookup_index,
               hash_block_offset);

      source_lookup_record = composed_source_lookup_record;
    }

  public:
    hashdb_db_manager_t(const std::string& _hashdb_dir,
                        file_mode_type_t _file_mode_type) :
                            hashdb_dir(_hashdb_dir),
                            file_mode_type(_file_mode_type),
                            hashdb_settings(settings_reader_t::read_settings(hashdb_dir)),
                            hash_changes(),
                            use_bloom1(hashdb_settings.bloom1_settings.is_used),
                            use_bloom2(hashdb_settings.bloom2_settings.is_used),
                            number_of_index_bits_type(hashdb_settings.source_lookup_settings.number_of_index_bits_type),
                            hash_store(0),
                            hash_duplicates_store(0),
                            source_lookup_manager(0),
                            bloom1(0),
                            bloom2(0) {

      // calculate the filename for each hashdb resource
      std::string hash_store_path =
               hashdb_filenames_t::hash_store_filename(hashdb_dir);
      std::string hash_duplicates_store_path =
               hashdb_filenames_t::hash_duplicates_store_filename(hashdb_dir);
      std::string bloom1_path =
               hashdb_filenames_t::bloom1_filename(hashdb_dir);
      std::string bloom2_path =
               hashdb_filenames_t::bloom2_filename(hashdb_dir);

      // open the hashdb resources
      hash_store = new hash_store_t(
               hash_store_path,
               file_mode_type,
               hashdb_settings.hash_store_settings);
      hash_duplicates_store = new hash_duplicates_store_t(
               hash_duplicates_store_path,
               file_mode_type,
               hashdb_settings.hash_duplicates_store_settings);
      source_lookup_manager = new source_lookup_manager_t(
               hashdb_dir,
               file_mode_type);
      bloom1 = new bloom_filter_t(
               bloom1_path,
               file_mode_type,
               hashdb_settings.bloom1_settings);
      bloom2 = new bloom_filter_t(
               bloom2_path,
               file_mode_type,
               hashdb_settings.bloom2_settings);
    }

    ~hashdb_db_manager_t() {
      delete hash_store;
      delete hash_duplicates_store;
      delete source_lookup_manager;
      delete bloom1;
      delete bloom2;
    }

    bool has_source_lookup_record(const md5_t& md5,
                      source_lookup_record_t& source_lookup_record) const {

      // first check for negative in active bloom filters
      if ((use_bloom1) && !bloom1->is_positive(md5)) {
        return false;
      }
      if ((use_bloom2) && !bloom2->is_positive(md5)) {
        return false;
      }

      // check hash store
      return hash_store->has_source_lookup_record(md5, source_lookup_record);
    }

    // has hash and also the same source
    bool has_hash_element(const hashdb_element_t& hashdb_element) const {
      const md5_t md5(hashdb_element.first);
      const hash_source_record_t hash_source_record(hashdb_element.second);

      // stop if the hash block size is wrong for this database
      if (hash_source_record.hash_block_size != hashdb_settings.hash_block_size) {
        return false;
      }

      // stop if the hash digest type is wrong for this database
      hashdigest_type_t type;
      bool has = string_to_hashdigest_type(hash_source_record.hashdigest_type_string, type);
      if (!(has && type == hashdb_settings.hashdigest_type)) {
        return false;
      }

      // false if no md5
      source_lookup_record_t source_lookup_record;
      if (!has_source_lookup_record(md5, source_lookup_record)) {
        // no md5, so no element
        return false;
      }

      // false if the source lookup store has no source location record
      uint64_t source_lookup_index;
      bool status = source_lookup_manager->get_source_lookup_index(
                    hash_source_record.repository_name,
                    hash_source_record.filename,
                    source_lookup_index);

      if (status == false) {
        // no hash_source_record, so no element
        return false;
      }

      // look up the expected source lookup record
      source_lookup_record_t expected_source_lookup_record;
      hash_source_record_to_source_lookup_record(
                       hash_source_record, expected_source_lookup_record);

      // check the source lookup record against the expected value
      if (source_lookup_record.get_count() == 1) {
        // the hash store has the only source lookup record
        if (source_lookup_record == expected_source_lookup_record) {
          // the source lookup record portion matches
          return true;
        } else {
          // the source lookup record portion is different
          return false;
        }
      } else {
        // the source lookup record is in the hash duplicates store
        return hash_duplicates_store->has_hash_element(
                                         md5, expected_source_lookup_record);
      }
    }

    void insert_hash_element(const hashdb_element_t& hashdb_element) {

      // get the hash source record from the pair
      const hash_source_record_t hash_source_record(hashdb_element.second);

      // validate the file offset
      if (hash_source_record.file_offset % hashdb_settings.hash_block_size != 0) {
        // invalid offset value
        ++hash_changes.hashes_not_inserted_invalid_file_offset;
        return;
      }

      // validate the hash block size
      if (hash_source_record.hash_block_size != hashdb_settings.hash_block_size) {
        // wrong hash block size
        ++hash_changes.hashes_not_inserted_wrong_hash_block_size;
        return;
      }

      // validate the hashdigest type
      hashdigest_type_t type;
      bool has = string_to_hashdigest_type(hash_source_record.hashdigest_type_string, type);
      if (!(has && type == hashdb_settings.hashdigest_type)) {
        // wrong hashdigest type
        ++hash_changes.hashes_not_inserted_wrong_hashdigest_type;
        return;
      }

      // verify that it has not already been added
      if (has_hash_element(hashdb_element)) {
        // element already added
        ++hash_changes.hashes_not_inserted_duplicate_source;
        return;
      }

      // get the source lookup index
      uint64_t source_lookup_index;
      bool status = source_lookup_manager->insert_source_lookup_element(
                                    hash_source_record.repository_name,
                                    hash_source_record.filename,
                                    source_lookup_index);
      if (status == 0) {
        // program error
        assert(0);
      }

      // calculate hash block offset
      uint64_t hash_block_offset =
                 hash_source_record.file_offset / hashdb_settings.hash_block_size;

      // now compose the source lookup record
      source_lookup_record_t source_lookup_record(
               number_of_index_bits_type,
               source_lookup_index,
               hash_block_offset);

      // add element to hash store or hash duplicates store
      const md5_t md5(hashdb_element.first);
      source_lookup_record_t existing_source_lookup_record;
      if (!hash_store->has_source_lookup_record(md5, existing_source_lookup_record)) {

        // the cryptographic hash is new so add it to the hash store
        hash_store->insert_hash_element(md5, source_lookup_record);

        // also, since it is new, add it to bloom filters
        if (use_bloom1) {
          bloom1->add_hash_value(md5);
        }
        if (use_bloom2) {
          bloom2->add_hash_value(md5);
        }

      } else {

        // add the cryptographic hash to the hash duplicates store,
        // changing the hash store as needed

        // identify the existing count
        uint32_t existing_count = existing_source_lookup_record.get_count();

        // stop if count is at max but let 0 mean no max
        if (hashdb_settings.maximum_hash_duplicates != 0
         && existing_count >= hashdb_settings.maximum_hash_duplicates) {

          // there are too many hashes of this value so drop it
          ++hash_changes.hashes_not_inserted_exceeds_max_duplicates;
          return;
        }

        if (existing_count == 1) {
          // copy the existing hash element to the hash duplicates store
          hash_duplicates_store->insert_hash_element(md5, existing_source_lookup_record);
        }

        // add the hash element to the hash duplicates store
        hash_duplicates_store->insert_hash_element(md5, source_lookup_record);

        // increment the existing record count
        source_lookup_record_t incremented_record(existing_count + 1);
        hash_store->change_source_lookup_record(md5, incremented_record);
      }

      // added
      ++hash_changes.hashes_inserted;
      return;
    }

    void remove_hash_element(const hashdb_element_t& hashdb_element) {

      const hash_source_record_t hash_source_record(hashdb_element.second);

      // validate the file offset
      if (hash_source_record.file_offset % hashdb_settings.hash_block_size != 0) {
        // invalid offset value
        ++hash_changes.hashes_not_removed_invalid_file_offset;
        return;
      }

      // validate the hash block size
      if (hash_source_record.hash_block_size != hashdb_settings.hash_block_size) {
        // wrong hash block size
        ++hash_changes.hashes_not_removed_wrong_hash_block_size;
        return;
      }

      // validate the hashdigest type
      hashdigest_type_t type;
      bool has = string_to_hashdigest_type(hash_source_record.hashdigest_type_string, type);
      if (!(has && type == hashdb_settings.hashdigest_type)) {
        // wrong hashdigest type
        ++hash_changes.hashes_not_removed_wrong_hashdigest_type;
        return;
      }

      // see if the hash store has the hash value
      const md5_t md5(hashdb_element.first);
      source_lookup_record_t existing_source_lookup_record;
      has = hash_store->has_source_lookup_record(md5, existing_source_lookup_record);
      if (!has) {
        // there is no matching hash in the hash store to remove
        ++hash_changes.hashes_not_removed_no_hash;
        return;
      }

      // see if the source lookup store has the source location record
      uint64_t source_lookup_index;
      bool status = source_lookup_manager->get_source_lookup_index(
                    hash_source_record.repository_name,
                    hash_source_record.filename,
                    source_lookup_index);

      if (status == false) {
        // no hash_source_record, so no element
        ++hash_changes.hashes_not_removed_different_source;
        return;
      }

      // the hash and source lookup record exist
      // but an element matching this hash and source may not exist

      // get the source lookup record
      source_lookup_record_t source_lookup_record;
      hash_source_record_to_source_lookup_record(
                                  hash_source_record, source_lookup_record);

      if (existing_source_lookup_record.get_count() == 1) {
        // check the hash store

        // validate that the source lookup record matches
        if (existing_source_lookup_record == source_lookup_record) {

          // remove element from hash store
          hash_store->erase_hash_element(md5);
          ++hash_changes.hashes_removed;
          return;
        } else {
          // it is different because it has a different source location record
          ++hash_changes.hashes_not_removed_different_source;
          return;
        }

      } else {
        // check the hash duplicates store

        // get the records from the hash duplicates store
        std::vector<source_lookup_record_t> records;
        hash_duplicates_store->get_source_lookup_record_vector(md5, records);

        // ensure that the number of records match the expected amount
        if (existing_source_lookup_record.get_count() != records.size()) {
          // program error
          assert(0);
        }

        // remove the matching element, adjusting if count drops to 1
        for (std::vector<source_lookup_record_t>::iterator it = records.begin();
                     it != records.end(); it++) {
          if (*it == source_lookup_record) {
            // remove the element
            hash_duplicates_store->erase_hash_element(md5, source_lookup_record);

            // hash duplicates store is not to have just one source lookup
            // record for a hash
            // if the duplicates store ends up with one remaining record,
            // remove it
            // and put its source lookup record back into the hash store
            if (existing_source_lookup_record.get_count() == 2) {

              // move back the last hash value from the hash duplicates store
              records.erase(it);
              source_lookup_record_t remaining_record = records[0];

              hash_store->change_source_lookup_record(md5, remaining_record);
              hash_duplicates_store->erase_hash_element(md5, remaining_record);
            } else {
              // decrement the count in the hash store
              uint32_t count = existing_source_lookup_record.get_count();
              source_lookup_record_t decremented_record(count - 1);
              hash_store->change_source_lookup_record(md5, decremented_record);
            }
            ++hash_changes.hashes_removed;
            return;
          }
        }

        // all the elements in the hash duplicates store are different
        // because they all have different source location records
        ++hash_changes.hashes_not_removed_different_source;
        return;
      }
    }

    bool get_hash_source_records(const md5_t& md5,
                     std::vector<hash_source_record_t>& hash_source_records) const {

      // clear the vector
      hash_source_records.clear();

      // get the one source lookup record from the hash store
      source_lookup_record_t record;
      bool has = hash_store->has_source_lookup_record(md5, record);
      if (!has) {
        return false;
      }

      hash_source_record_t hash_source_record;
      if (record.get_count() == 1) {
        // get one element from the hash store
        source_lookup_record_to_hash_source_record(record, hash_source_record);
        hash_source_records.push_back(hash_source_record);
      } else {
        // get multiple source lookup records from the hash duplicates store
        std::vector<source_lookup_record_t> records;
        hash_duplicates_store->get_source_lookup_record_vector(md5, records);
        for (std::vector<source_lookup_record_t>::iterator it = records.begin();
                     it != records.end(); it++) {
          source_lookup_record_to_hash_source_record(*it, hash_source_record);
          hash_source_records.push_back(hash_source_record);
        }
      }
      return true;
    }

    /**
     * Iterator for all cryptographic hashes in the hashdb,
     * where dereferencing produces pair of md5_t and hash_source_record_t.
     */
    class hashdb_iterator_t {

    private:

      // state required to support hashdb iterator
      const hashdb_db_manager_t* hashdb_db_manager;
      hash_store_t::hash_store_iterator_t hash_store_iterator;
      bool use_duplicates;
      std::vector<source_lookup_record_t> duplicates;
      std::vector<source_lookup_record_t>::iterator duplicates_iterator;
      hashdb_element_t hashdb_element;
      bool at_end;

      // set state based on hash store iterator
      // and contents of referenced pair element
      // uses state variables:
      //        hash_store_iterator
      //        at_end                end of hash_store_iterator
      //        use_duplicates
      //        duplicates
      //        duplicates_iterator
      //
      // current active hashdb element:
      //        hashdb_element

      // read the hash store and set the state based on duplicates
      void read_hash_store() {
        use_duplicates = false;

        if (hash_store_iterator == hashdb_db_manager->hash_store->end()) {
          // no elements
          at_end = true;
          return;
        }

        // evaluate hash store element
        md5_t md5 = hash_store_iterator->first;
        source_lookup_record_t source_lookup_record = hash_store_iterator->second;

        // set state based on count
        if (source_lookup_record.get_count() == 1) {
          // use hash store
          use_duplicates = false;
        } else {
          // use hash duplicates store
          use_duplicates = true;
          hashdb_db_manager->hash_duplicates_store->get_source_lookup_record_vector(md5, duplicates);
          // program error if none
          if (duplicates.empty()) {
            assert(0); 
          }
          duplicates_iterator = duplicates.begin();
        }
      }

      void read_hashdb_element() {
        if (hash_store_iterator == hashdb_db_manager->hash_store->end()) {
          // no elements
          at_end = true;
          return;
        }

        source_lookup_record_t source_lookup_record;

        if (!use_duplicates) {
          // use hash store
          source_lookup_record = hash_store_iterator->second;
        
        } else {
          // use hash duplicates store
          if (duplicates_iterator == duplicates.end()) {
            assert(0);
          }
          source_lookup_record = *duplicates_iterator;
        }

        // look up the hash source record
        hash_source_record_t hash_source_record;

        hashdb_db_manager->source_lookup_record_to_hash_source_record(
                        source_lookup_record, hash_source_record);

        // set hashdb element
        md5_t md5 = hash_store_iterator->first;
 
        hashdb_element = hashdb_element_t(md5, hash_source_record);
      }

      void next_hashdb_element() {
        if (!use_duplicates) {
          // increment hash store
          ++hash_store_iterator;
          read_hash_store();
        } else {
          // increment hash duplicates store
          ++duplicates_iterator;
          if (duplicates_iterator == duplicates.end()) {
            ++hash_store_iterator;
            read_hash_store();
          }
        }
      }

    public:
      // iterator constructor
      hashdb_iterator_t(const hashdb_db_manager_t* _hashdb_db_manager, bool _at_end) :
             hashdb_db_manager(_hashdb_db_manager),
             hash_store_iterator(_hashdb_db_manager->hash_store, _at_end),
             use_duplicates(false),
             duplicates(),
             duplicates_iterator(),
             hashdb_element(),
             at_end(_at_end) {

        // start the iterator
        read_hash_store();
        read_hashdb_element();
      }

      hashdb_iterator_t& operator++() {
        next_hashdb_element();
        read_hashdb_element();
        return *this;
      }

      hashdb_iterator_t(const hashdb_iterator_t& other) :
        hashdb_db_manager(other.hashdb_db_manager),
        hash_store_iterator(other.hash_store_iterator),
        use_duplicates(other.use_duplicates),
        duplicates(other.duplicates),
        duplicates_iterator(other.duplicates_iterator),
        hashdb_element(other.hashdb_element),
        at_end(other.at_end) {
      }

      hashdb_iterator_t& operator=(const hashdb_iterator_t& other) {
        hashdb_db_manager = other.hashdb_db_manager;
        hash_store_iterator = other.hash_store_iterator;
        use_duplicates = other.use_duplicates;
        duplicates = other.duplicates;
        duplicates_iterator = other.duplicates_iterator;
        hashdb_element = other.hashdb_element;
        at_end = other.at_end;
        return *this;
      }

      bool const operator==(const hashdb_iterator_t& other) const {
        // the iterators must be for the same hashdb manager
        if (this->hashdb_db_manager != other.hashdb_db_manager) {
          return false;
        }

        // both at end
        if (this->at_end && other.at_end) {
          return true;
        }

        // one at end
        if (this->at_end || other.at_end) {
          return false;
        }

        // neither at end
        return (this->hashdb_element == other.hashdb_element);
      }

      bool const operator!=(const hashdb_iterator_t& other) const {
        return (!(*this==other));
      }

      hashdb_element_t const* operator->() const {
        // program error if accessing when done
        if (at_end) {
          assert(0);
        }
        return &hashdb_element;
      }

      hashdb_element_t const& operator*() const {
        // program error if accessing when done
        if (at_end) {
          assert(0);
        }
        return hashdb_element;
      }
    };

    hashdb_iterator_t begin() const {
      return hashdb_iterator_t(this, false);
    }

    hashdb_iterator_t end() const {
      return hashdb_iterator_t(this, true);
    }

    void report_status(std::ostream& os) const {
      os << "hashdb status: ";
      os << "hashdb path=" << hashdb_dir;
      os << ", file mode=" << file_mode_type;
      os << "\n";
      hash_store->report_status(std::cout);
      hash_duplicates_store->report_status(std::cout);
      bloom1->report_status(std::cout, 1);
      bloom2->report_status(std::cout, 2);
    }

    void report_status(dfxml_writer& x) const {
//      x.push("hashdb_status");
      x.xmlout("hashdb_path", hashdb_dir);
      x.xmlout("file_mode", file_mode_type_to_string(file_mode_type));
      hash_store->report_status(x);
      hash_duplicates_store->report_status(x);
      bloom1->report_status(x, 1);
      bloom2->report_status(x, 2);
//      x.pop();
    }
};

#endif

