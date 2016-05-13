/**
 * \file
 * provide system-specific filename type and conversion function for Windows.
 */

#ifndef FILENAME_T_HPP
#define FILENAME_T_HPP

#include <string>

namespace hasher {

#ifdef WIN32
  typedef std::wstring filename_t;
#else    
  typedef std::string filename_t;
#endif

  // get filename in native string type
  filename_t utf8_to_native(const std::string &fn8);

}

#endif

