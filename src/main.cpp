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
 * Provides the main entry for the hashdb_manager tool.
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif
#include "hash_t_selector.h"
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "usage.hpp"
#include "bloom_filter_calculator.hpp"
#include "command_line.hpp"
#include "commands.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>
#include <boost/lexical_cast.hpp>
#include <getopt.h>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

static const std::string see_usage = "Please type 'hashdb -h' for usage.";

// settings
static hashdb_settings_t hashdb_settings;

// option state used to assure settings are not specified when they should not be
// option categories
static bool has_hashdb_settings = false;
static bool has_bloom_filter_settings = false;
static bool has_repository_name = false;
static size_t parameter_count = 0;

// bloom filter validation
bool has_bloom1 = false;
bool has_bloom1_n = false;
bool has_bloom1_kM = false;
bool has_bloom2 = false;
bool has_bloom2_n = false;
bool has_bloom2_kM = false;

// repository name
static std::string repository_name_string = "";

// arguments
static std::string command;
static std::string hashdb_arg1;
static std::string hashdb_arg2;
static std::string hashdb_arg3;

// C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
// copied from bulk_extractor file support.cpp
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

void require_no_hashdb_settings() {
  if (has_hashdb_settings) {
    std::cerr << "hashdb settings are not allowed in this command.\n";
    exit(1);
  }
}
void require_no_bloom_filter_settings() {
  if (has_bloom_filter_settings) {
    std::cerr << "bloom filter settings are not allowed in this command.\n";
    exit(1);
  }
}
void require_no_repository_name() {
  if (has_repository_name) {
    std::cerr << "Repository name not allowed in this command.\n";
    exit(1);
  }
}
void require_parameter_count(size_t count) {
  if (count != parameter_count) {
    std::cerr << "Incorrect number of parameters provided in this command.\n";
    std::cerr << "Expected " << count
              << ", but received " << parameter_count << ".\n";
    exit(1);
  }
}

template<typename T> void run_command();
//template void <typename T>run_command() {

// ************************************************************
// main
// ************************************************************
int main(int argc,char **argv) {
  command_line_t::command_line_string = dfxml_writer::make_command_line(argc, argv);

#ifdef HAVE_MCHECK
  mtrace();
#endif

  // manage when there are no arguments
  if(argc==1) {
      usage();
      exit(1);
  }

  // parse options
  int option_index; // not used
  while (1) {

    const struct option long_options[] = {
      // general
      {"help", no_argument, 0, 'h'},
      {"Help", no_argument, 0, 'H'},
      {"Version", no_argument, 0, 'V'},

      // hashdb settings
      {"hash_block_size", required_argument, 0, 'p'},
      {"max_duplicates", required_argument, 0, 'm'},

      // bloom filter settings
      {"bloom1", required_argument, 0, 'A'},
      {"bloom1_n", required_argument, 0, 'B'},
      {"bloom1_kM", required_argument, 0, 'C'},
      {"bloom2", required_argument, 0, 'D'},
      {"bloom2_n", required_argument, 0, 'E'},
      {"bloom2_kM", required_argument, 0, 'F'},

      // repository
      {"repository", required_argument, 0, 'r'},

      // end
      {0,0,0,0}
    };

    int ch = getopt_long(argc, argv, "hHV p:m:A:B:C:D:E:F:r:", long_options, &option_index);
    if (ch == -1) {
      // no more arguments
      break;
    }
    if (ch == 0) {
      // command options set flags and use ch==0
      continue;
    }
    switch (ch) {
      case 'h': {	// help
        usage();
        exit(0);
        break;
      }
      case 'H': {	// Help
        usage();
        detailed_usage();
        exit(0);
        break;
      }
      case 'V': {	// Version
        std::cout << "hashdb_manager " << PACKAGE_VERSION << "\n";
        exit(0);
        break;
      }

      // hashdb_settings
      case 'p': {	// hash block size
        has_hashdb_settings = true;
        try {
          hashdb_settings.hash_block_size = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for hash_block_size: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'm': {	// maximum hash duplicates
        has_hashdb_settings = true;
        try {
          hashdb_settings.maximum_hash_duplicates = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for maximum hash duplicates: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }

      // bloom filter settings
      case 'A': {	// b1 bloom 1 state
        has_bloom_filter_settings = true;
        bool is_ok_bloom1_state = string_to_bloom_state(optarg, hashdb_settings.bloom1_is_used);
        if (!is_ok_bloom1_state) {
          std::cerr << "Invalid value for bloom filter 1 state: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'B': {	// b1n bloom 1 expected total number of hashes
        has_bloom_filter_settings = true;
        has_bloom1_n = true;
        try {
          uint64_t n1 = boost::lexical_cast<uint64_t>(optarg);
          hashdb_settings.bloom1_k_hash_functions = 3;
          hashdb_settings.bloom1_M_hash_size = bloom_filter_calculator::approximate_n_to_M(n1);
        } catch (...) {
          std::cerr << "Invalid value for bloom filter 1 expected total number of hashes: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'C': {	// b1kM bloom 1 k hash functions and M hash size
        has_bloom_filter_settings = true;
        has_bloom1_kM = true;
        std::vector<std::string> params = split(std::string(optarg), ':');

        if (params.size() == 2) {
          try {
            hashdb_settings.bloom1_k_hash_functions = boost::lexical_cast<uint32_t>(params[0]);
            hashdb_settings.bloom1_M_hash_size = boost::lexical_cast<uint32_t>(params[1]);
            break;
          } catch (...) {
            // let fall through to failure
          }
        }
        // bad input
        std::cerr << "Invalid value for bloom filter 1 k:M: '" << optarg << "'.  " << see_usage << "\n";
        exit(1);
        break;
      }
      case 'D': {	// b2 bloom 2 state
        has_bloom_filter_settings = true;
        bool is_ok_bloom2_state = string_to_bloom_state(optarg, hashdb_settings.bloom2_is_used);
        if (!is_ok_bloom2_state) {
          std::cerr << "Invalid value for bloom filter 2 state: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'E': {	// b2n bloom 2 expected total number of hashes
        has_bloom_filter_settings = true;
        has_bloom2_n = true;
        try {
          uint64_t n2 = boost::lexical_cast<uint64_t>(optarg);
          hashdb_settings.bloom2_k_hash_functions = 3;
          hashdb_settings.bloom2_M_hash_size = bloom_filter_calculator::approximate_n_to_M(n2);
        } catch (...) {
          std::cerr << "Invalid value for bloom filter 2 expected total number of hashes: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'F': {	// b2kM bloom 2 k hash functions and M hash size
        has_bloom_filter_settings = true;
        has_bloom2_kM = true;
        std::vector<std::string> params = split(std::string(optarg), ':');
        if (params.size() == 2) {
          try {
            hashdb_settings.bloom2_k_hash_functions = boost::lexical_cast<uint32_t>(params[0]);
            hashdb_settings.bloom2_M_hash_size = boost::lexical_cast<uint32_t>(params[1]);
            break;
          } catch (...) {
            // let fall through to failure
          }
        }
        // bad input
        std::cerr << "Invalid value for bloom filter 2 k:M: '" << optarg << "'.  " << see_usage << "\n";
        exit(1);
        break;
      }

      // repository name option
      case 'r': {	// repository name
        has_repository_name = true;
        repository_name_string = std::string(optarg);
        break;
      }

      default:
//        std::cerr << "unexpected command character " << ch << "\n";
        exit(1);
    }
  }

  // ************************************************************
  // parse the command
  // ************************************************************

  // parse the remaining tokens that were not consumed by options
  argc -= optind;
  argv += optind;

  // get the command
  if (argc < 1) {
    std::cerr << "Error: a command must be provided.\n";
    exit(1);
  }
  command = argv[0];
  argc--;
  argv++;

  // get any arguments
  parameter_count = argc;
  hashdb_arg1 = (argc>=1) ? argv[0] : "";
  hashdb_arg2 = (argc>=2) ? argv[1] : "";
  hashdb_arg3 = (argc>=3) ? argv[2] : "";

  // ************************************************************
  // post-process the options
  // ************************************************************
  // check that bloom filter n or kM are selected but not both
  if ((has_bloom1_n && has_bloom1_kM) || (has_bloom2_n && has_bloom2_kM)) {
    std::cerr << "Error: either a Bloom filter n value or a bloom filter k:M value may be\n";
    std::cerr << "specified, but not both.  " << see_usage << "\n";
    exit(1);
  }

  // check that bloom hash size is compatible with the running system
  uint32_t temp_st = (sizeof(size_t) * 8) -1;
  uint32_t temp_b1 = hashdb_settings.bloom1_M_hash_size;
  uint32_t temp_b2 = hashdb_settings.bloom2_M_hash_size;
  if (temp_b1 > temp_st) {
    std::cerr << "Error: Bloom 1 bits per hash, " << temp_b1
              << ", exceeds " << temp_st
              << ", which is the limit on this system.\n";
    exit(1);
  }
  if (temp_b2 > temp_st) {
    std::cerr << "Error: Bloom 2 bits per hash, " << temp_b2
              << ", exceeds " << temp_st
              << ", which is the limit on this system.\n";
    exit(1);
  }

  // run the command
  run_command<hash_t>();

#ifdef HAVE_MCHECK
  muntrace();
#endif

  // done
  return 0;
}

//template void <typename T>run_command() 
template <typename T>
void run_command() {

  // ************************************************************
  // run the command
//    require_no_hashdb_settings();
//    require_no_bloom_filter_settings();
//    require_no_repository_name();
//    require_parameter_count(1);
  // ************************************************************

  if (command == "create") {
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::create(hashdb_settings, hashdb_arg1);
  } else if (command == "import") {
    // compose a repository name if one is not provided
    if (repository_name_string == "") {
      repository_name_string = "repository_" + hashdb_arg1;
    }
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_parameter_count(2);
    commands_t<T>::import(repository_name_string, hashdb_arg1, hashdb_arg2);
  } else if (command == "export") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::do_export(hashdb_arg1, hashdb_arg2);
  } else if (command == "add") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::add(hashdb_arg1, hashdb_arg2);
  } else if (command == "add_multiple") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(3);
    commands_t<T>::add_multiple(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "intersect") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(3);
    commands_t<T>::intersect(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "subtract") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(3);
    commands_t<T>::subtract(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "deduplicate") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::deduplicate(hashdb_arg1, hashdb_arg2);
  } else if (command == "scan") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::scan(hashdb_arg1, hashdb_arg2);
  } else if (command == "scan_expanded") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::scan_expanded(hashdb_arg1, hashdb_arg2);
  } else if (command == "expand_identified_blocks") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::expand_identified_blocks(hashdb_arg1, hashdb_arg2);
  } else if (command == "server") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::server(hashdb_arg1, hashdb_arg2);
  } else if (command == "size") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::size(hashdb_arg1);
  } else if (command == "sources") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::sources(hashdb_arg1);
  } else if (command == "histogram") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::histogram(hashdb_arg1);
  } else if (command == "duplicates") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::duplicates(hashdb_arg1, hashdb_arg2);
  } else if (command == "hash_table") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::hash_table(hashdb_arg1);
  } else if (command == "rebuild_bloom") {
    require_no_hashdb_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t<T>::rebuild_bloom(hashdb_settings, hashdb_arg1);
  } else if (command == "add_random") {
    // compose a repository name if one is not provided
    if (repository_name_string == "") {
      repository_name_string = "repository_add_random";
    }
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_parameter_count(2);
    commands_t<T>::add_random(repository_name_string, hashdb_arg1, hashdb_arg2);
  } else if (command == "scan_random") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t<T>::scan_random(hashdb_arg1, hashdb_arg2);
  } else {
    // invalid command
    std::cerr << "Error: '" << command << "' is not a recognized command.  " << see_usage << "\n";
  }
}

