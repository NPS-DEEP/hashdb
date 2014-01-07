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
 * Test the source lookup encoding module.
 */

#include "source_lookup_encoding.hpp"
#include <iostream>
#include <boost/detail/lightweight_main.hpp>
#include <boost/detail/lightweight_test.hpp>

// boost must allow exceptions for BOOST_TEST_THROWS
#ifdef BOOST_NO_EXCEPTIONS
  #error boost must allow exceptions for BOOST_TEST_THROWS
#endif

// BOOST_TEST_THROWS isn't available until boost v1.54, so add it here
#ifndef BOOST_TEST_THROWS
namespace boost {
namespace detail {
inline void throw_failed_impl(char const * excep, char const * file, int line, char const * function)
{
   BOOST_LIGHTWEIGHT_TEST_OSTREAM
    << file << "(" << line << "): Exception '" << excep << "' not thrown in function '"
    << function << "'" << std::endl;
   ++test_errors();
}
} // detail
} // boost

#ifndef BOOST_NO_EXCEPTIONS
   #define BOOST_TEST_THROWS( EXPR, EXCEP )                    \
      try {                                                    \
         EXPR;                                                 \
         ::boost::detail::throw_failed_impl                    \
         (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
      }                                                        \
      catch(EXCEP const&) {                                    \
      }                                                        \
      catch(...) {                                             \
         ::boost::detail::throw_failed_impl                    \
         (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
      }                                                        \
   //
#else
   #define BOOST_TEST_THROWS( EXPR, EXCEP )
#endif
#endif
namespace {
}

int cpp_main(int argc, char* argv[]) {

  // 32 bits, small values
  uint64_t temp = 0xffffffffffffffff;
  temp = source_lookup_encoding::get_source_lookup_encoding(32, 2, 3);
std::cout << "slet.temp.a " << temp << "\n";
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(32, temp), 2);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(32, temp), 3);

  // 32 bits, large value for source_lookup_index
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(32, 0xffffffff, 4);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(32, temp), 0xffffffff);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(32, temp), 4);

  // 32 bits, large value for hash_block_offset
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(32, 5, 0xffffffff);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(32, temp), 5);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(32, temp), 0xffffffff);

  // 32 bits, invalid values
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(32, 0, 0x100000000), std::runtime_error);
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(32, 0x100000000, 0), std::runtime_error);

  // 40 bits, small values
  temp = 0xffffffffffffffff;
  temp = source_lookup_encoding::get_source_lookup_encoding(40, 6, 7);
std::cout << "slet.temp.a " << temp << "\n";
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(40, temp), 6);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(40, temp), 7);

  // 40 bits, large value for source_lookup_index
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(40, 0xffffffffff, 8);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(40, temp), 0xffffffffff);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(40, temp), 8);

  // 40 bits, large value for hash_block_offset
  temp = 0;
  temp = source_lookup_encoding::get_source_lookup_encoding(40, 9, 0xffffffffff);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 1);
  BOOST_TEST_EQ(source_lookup_encoding::get_source_lookup_index(40, temp), 9);
  BOOST_TEST_EQ(source_lookup_encoding::get_hash_block_offset(40, temp), 0xffffffffff);

  // 40 bits, invalid values
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(40, 0, 0x100000000000), std::runtime_error);
  BOOST_TEST_THROWS(temp = source_lookup_encoding::get_source_lookup_encoding(40, 0x10000000, 0), std::runtime_error);

  // count functionality
  temp = source_lookup_encoding::get_source_lookup_encoding(2);
  BOOST_TEST_EQ(source_lookup_encoding::get_count(temp), 2);

  // done
  int status = boost::report_errors();
  return status;
}

