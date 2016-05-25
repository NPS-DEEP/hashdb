/**
 * \file
 * Produce a list of filenames suitable for processing.
 */

#ifndef FILENAME_LIST_HPP
#define FILENAME_LIST_HPP

#include <string>
#include "filename_t.hpp"

namespace hasher {

  std::string filename_list(const std::string& utf8_filename,
                            hasher::filenames_t* files);

}

#endif

