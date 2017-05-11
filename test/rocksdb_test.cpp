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
 * Test the LMDB data managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <string>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/merge_operator.h>

//#include "unit_test.h"
//#include "../src_libhashdb/hashdb.hpp"
//#include "directory_helper.hpp"

using namespace rocksdb;


void test_column_families_example() {
  std::string kDBPath = "temp_rocksdb_column_families_example";

  // open DB
  Options options;
  options.create_if_missing = true;
  DB* db;
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

  // create column family
  ColumnFamilyHandle* cf;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "new_cf", &cf);
  assert(s.ok());

  // close DB
  delete cf;
  delete db;

  // open DB with two column families
  std::vector<ColumnFamilyDescriptor> column_families;
  // have to open default column family
  column_families.push_back(ColumnFamilyDescriptor(
      kDefaultColumnFamilyName, ColumnFamilyOptions()));
  // open the new one, too
  column_families.push_back(ColumnFamilyDescriptor(
      "new_cf", ColumnFamilyOptions()));
  std::vector<ColumnFamilyHandle*> handles;
  s = DB::Open(DBOptions(), kDBPath, column_families, &handles, &db);
  assert(s.ok());

  // put and get from non-default column family
  s = db->Put(WriteOptions(), handles[1], Slice("key"), Slice("value"));
  assert(s.ok());
  std::string value;
  s = db->Get(ReadOptions(), handles[1], Slice("key"), &value);
  assert(s.ok());

  // atomic write
  WriteBatch batch;
  batch.Put(handles[0], Slice("key2"), Slice("value2"));
  batch.Put(handles[1], Slice("key3"), Slice("value3"));
  batch.Delete(handles[0], Slice("key"));
  s = db->Write(WriteOptions(), &batch);
  assert(s.ok());

  // drop column family
  s = db->DropColumnFamily(handles[1]);
  assert(s.ok());

  // close db
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

class Changes {
  public:
  uint64_t counter;
  Changes() : counter(0) {
  }
};

class StringAddOperator : public AssociativeMergeOperator {
  private:
  // do not allow copy or assignment
  StringAddOperator(const StringAddOperator&);
  StringAddOperator& operator=(const StringAddOperator&);

  public:
  Changes* changes;
  StringAddOperator(Changes* c) : changes(c) {
  }

  virtual bool Merge(
    const Slice& key,
    const Slice* existing_value,
    const Slice& value,
    std::string* new_value,
    Logger* logger) const override {

    // append value
    std::stringstream ss;
    if (existing_value) {
      ss << std::string(existing_value->data(), existing_value->size());
      ++changes->counter;
    }
    ss << value.ToString(false);
    *new_value = ss.str();
    return true;
  }

  virtual const char* Name() const override {
    return "StringAddOperator";
  }
};

class NumericAddOperator : public AssociativeMergeOperator {
  private:
  // do not allow copy or assignment
  NumericAddOperator(const NumericAddOperator&);
  NumericAddOperator& operator=(const NumericAddOperator&);

  public:
  Changes* changes;
  NumericAddOperator(Changes* c) : changes(c) {
  }

  virtual bool Merge(
    const Slice& key,
    const Slice* existing_value,
    const Slice& value,
    std::string* new_value,
    Logger* logger) const override {

    // append value
    std::stringstream ss;
    if (existing_value) {
      ss << std::atoi(existing_value->data()) +
            std::atoi(value.data());
      ++changes->counter;
    } else {
      ss << std::atoi(value.data());
    }
    *new_value = ss.str();
    return true;
  }

  virtual const char* Name() const override {
    return "NumericAddOperator";
  }
};

void test_merge() {
  std::string kDBPath = "temp_rocksdb_test_merge";

  Changes changes;

  // open DB
  Options options;
  options.create_if_missing = true;
  DB* db;
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

  // create column families
  ColumnFamilyHandle* cf1;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "cf_string", &cf1);
  assert(s.ok());
  ColumnFamilyHandle* cf2;
  s = db->CreateColumnFamily(ColumnFamilyOptions(), "cf_numeric", &cf2);
  assert(s.ok());

  // close DB
  delete cf1;
  delete cf2;
  delete db;

  // open DB with column families
  std::vector<ColumnFamilyDescriptor> column_families;

  // have to open default column family
  column_families.push_back(ColumnFamilyDescriptor(
      kDefaultColumnFamilyName, ColumnFamilyOptions()));

  // String CF
  ColumnFamilyOptions cf_options_string;
  cf_options_string.merge_operator.reset(new StringAddOperator(&changes));
  column_families.push_back(ColumnFamilyDescriptor(
                                         "cf_string", cf_options_string));

  // Numeric CF
  ColumnFamilyOptions cf_options_numeric;
  cf_options_numeric.merge_operator.reset(new NumericAddOperator(&changes));
  column_families.push_back(ColumnFamilyDescriptor(
                                         "cf_numeric", cf_options_numeric));

  // CF handles
  std::vector<ColumnFamilyHandle*> handles;
  s = DB::Open(DBOptions(), kDBPath, column_families, &handles, &db);
  assert(s.ok());

  // use cf_string
  s = db->Merge(WriteOptions(), handles[1], Slice("key"), Slice("value1"));
  assert(s.ok());
  s = db->Merge(WriteOptions(), handles[1], Slice("key"), Slice("VALUE2"));
  assert(s.ok());
  std::string value;
  s = db->Get(ReadOptions(), handles[1], Slice("key"), &value);
  assert(s.ok());
  std::cerr << "Get from cf_string: '" << value << "'\n";

  // use cf_numeric
  std::stringstream ss;
  ss << 5L;
  s = db->Merge(WriteOptions(), handles[2], Slice("key2"), Slice(ss.str()));

  assert(s.ok());
  std::stringstream ss2;
  ss2 << 7L;
  s = db->Merge(WriteOptions(), handles[2], Slice("key2"), Slice(ss2.str()));
  assert(s.ok());
  s = db->Get(ReadOptions(), handles[2], Slice("key2"), &value);
  assert(s.ok());
  std::cerr << "Get from cf_numeric: '" << std::atoi(value.c_str()) << "'\n";

  // drop column families so open with no CF works
  s = db->DropColumnFamily(handles[1]);
  assert(s.ok());
  s = db->DropColumnFamily(handles[2]);
  assert(s.ok());

  // close db
  for (auto handle : handles) {
    delete handle;
  }
  delete db;

  std::cout << "changes: " << changes.counter << "\n";
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  test_column_families_example();
  test_merge();

  // done
  std::cout << "rocksdb_test Done.\n";
  return 0;
}

