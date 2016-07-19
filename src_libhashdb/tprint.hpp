/**
 * \file
 * Print text threadsafely.
 */

#ifndef TPRINT_HPP
#define TPRINT_HPP

#include <string>
#include <ostream>

namespace hashdb {

  void tprint(std::ostream& os, const std::string& text);

}

#endif

