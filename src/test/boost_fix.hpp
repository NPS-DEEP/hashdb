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
#ifndef BOOST_FIX_HPP
#define BOOST_FIX_HPP

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
#endif

