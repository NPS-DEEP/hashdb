/**
 * \file
 * provide system-specific filename type and conversion function for Windows.
 */

#ifndef FILENAME_T_HPP
#define FILENAME_T_HPP

#include <string>
#include <set>

namespace hasher {

#ifdef WIN32
  typedef std::wstring filename_t;
  typedef std::set<std::wstring> filenames_t;
#else    
  typedef std::string filename_t;
  typedef std::set<std::string> filenames_t;
#endif

  // get filename in native string type
  filename_t utf8_to_native(const std::string& utf8_string);

  // get filename in utf8 string type
  std::string native_to_utf8(const filename_t& native_string);
}

#endif

