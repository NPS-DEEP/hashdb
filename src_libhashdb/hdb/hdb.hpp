// Author:  Bruce Allen
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
 * Open or create a DB.  Offer resources.
 */

#include <config.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/merge_operator.h>

#ifndef HDB_HPP
#define HDB_HPP

namespace hdb {

class db_t {

  public:
  changes_t changes;
  rocksdb::Comparator* comparators[5];
  rocksdb::AssociativeMergeOperator* merge_operators[5];
  rocksdb::ColumnFamilyOptions cfo[5];
  std::vector<rocksdb::ColumnFamilyDecriptor> cfd;
  std::vector<rocksdb::ColumnFamilyHandle*> cfh;
  rocksdb::DB* db;
  bool is_open;

  private:
  // do not allow copy or assignment
  db_t(const db_t&);
  db_t& operator=(const db_t&);

  void open_resources() {
    // options
    rocksdb::ColumnFamilyOptions cfo;

    // source_id_store
    comparators[0] = new source_id_comparator_t();
    merge_operators[0] = new source_id_merge_operator_t(changes);
    cfo[0] = ColumnFamilyOptions();
    cfo[0].merge_operator.reset(merge_operators[0]);
    cfo[0].comparator = comparators[0];

    // source_name_store

  }

  void close_resources() {
    if (is_open) {
      for (int i=0; i<5; i++) {
        if (comparators[i]) delete comparators[i];
        if (merge_operators[i]) delete merge_operators[i];
      }
      for (auto handle : cfh) {
        delete handle;
      }
    }
  }

  public:
  db_t() : changes(), merge_operators(), cfo(), cfd(), cfh(), db(nullptr),
           is_open(false) {
  }
  ~db_t() {
    close_resources();
  }

  // return column family handle based on name
  rocksdb::ColumnFamilyHandle* get_cfh(const std::string& name) {
    if (!is_open) {
      assert(0);
    }
    if (name == "source_id_store") {
      return cfh[1];
    } else if (name == "source_name_store") {
      return cfh[2];
    } else if (name == "source_data_store") {
      return cfh[3];
    } else if (name == "hash_store") {
      return cfh[4];
    } else if (name == "hash_data_store") {
      return cfh[5];
    } else {
      assert(0);
    }
  }

  std::string create(const std::string& hashdb_dir) {

    if (is_open) {
      return "Unable to create DB because a DB is already open";
    }

    // open new DB
    Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;
    status = db_status(rocksdb::DB::Open(options, hashdb_dir, &db);
    if (status != "") {
      return status;
    }

    open_resources();

    // install column families

    // source_id_store
    status = db_status(db->CreateColumnFamily(cfo[0], "source_id_store",
                                              &cfh[0]));
    if (status != "") {
      close_resources();
      delete db;
      return status;
    }

    // source_name_store
    status = db_status(db->CreateColumnFamily(cfo[1], "source_name_store",
                                              &cfh[1]));
    if (status != "") {
      close_resources();
      delete db;
      return status;
    }

    // source_data_store
    status = db_status(db->CreateColumnFamily(cfo[2], "source_data_store",
                                              &cfh[2]));
    if (status != "") {
      close_resources();
      delete db;
      return status;
    }

    // hash_store
    status = db_status(db->CreateColumnFamily(cfo[3], "hash_store",
                                              &cfh[3]));
    if (status != "") {
      close_resources();
      delete db;
      return status;
    }

    // hash_data_store
    status = db_status(db->CreateColumnFamily(cfo[4], "hash_data_store",
                                              &cfh[4]));
    if (status != "") {
      close_resources();
      delete db;
      return status;
    }

    // close resources
    close_resources();
    delete db;
    return "";
  }

  // open
  std::string open(const std::string& hashdb_dir, bool open_read_only) {
    if (is_open) {
      return "A DB is already open";
    }

    // open resources
    open_resources();

    // goup the column family descriptors
    descriptors.clear();

    // have to add the unused default column family
    descriptors.push_back(ColumnFamilyDescriptor(
                           kDefaultColumnFamilyName, ColumnFamilyOptions()));

    // hashdb column families
    descriptors.push_back(ColumnFamilyDescriptor("source_id_store", cfo[0]);
    descriptors.push_back(ColumnFamilyDescriptor("source_name_store", cfo[0]);
    descriptors.push_back(ColumnFamilyDescriptor("source_data_store", cfo[0]);
    descriptors.push_back(ColumnFamilyDescriptor("hash_store", cfo[0]);
    descriptors.push_back(ColumnFamilyDescriptor("hash_data_store", cfo[0]);

    // open DB
    std::string status;
    if (open_read_only) {
      status = db_status(DB::OpenForReadOnly(DB_Options(), hashdb_dir,
                                             cfd, &cfh, &db);
    } else {
      status = db_status(DB::Open(DB_Options(), hashdb_dir, cfd, &cfh, &db);
    }

    if (status != "") {
      close_resources();
      return status;
    }

    // opened
    is_open = true;
    return "";
  }
};

} // end namespace hdb

#endif

