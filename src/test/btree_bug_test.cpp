// Test to manifest the use_count bug, 1/15/2014

#include "file_modes.h"
#include "map_btree.hpp"
#include <iostream>
#include <cstdio>
#include <boost/detail/lightweight_main.hpp>

static const char temp_file[] = "temp_file";

typedef map_btree_t<uint64_t, uint64_t> map_t;

void test_rw() {

  size_t size __attribute__((unused));
  map_t::map_pair_it_bool_t pair_it_bool; 

  // clean up from any previous run
  remove(temp_file);

  // create new map
  map_t map(temp_file, RW_NEW);

  // check count
  size = map.size();

  // change entry invalid
  pair_it_bool = map.change(6000006, 60);

  // check count stayed same
  size = map.size();
}

void test_ro() {
  // open and reclose Read Only
  map_t map(temp_file, READ_ONLY);
}

int cpp_main(int argc, char* argv[]) {

  test_rw();
  test_ro();

  // done
  return 0;
}

