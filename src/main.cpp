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
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif
#include "file_modes.h"
// zz not yet #include "map_types.h"
// zz not yet #include "multimap_types.h"
#include "hashdb_settings.hpp"
#include "usage.hpp"
#include "bloom_filter_calculator.hpp"
#include "command_line.hpp" // zz make this better
#include "commands.hpp" // zz make this better

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

static const std::string see_usage = "Please type 'hashdb -h' for usage.";

/*
static const std::string COMMAND_CREATE = "create";
static const std::string COMMAND_IMPORT = "import";
static const std::string COMMAND_EXPORT = "export";
static const std::string COMMAND_COPY = "copy";
static const std::string COMMAND_MERGE = "merge";
static const std::string COMMAND_REMOVE = "remove";
static const std::string COMMAND_REMOVE_DFXML = "remove_dfxml";
static const std::string COMMAND_DEDUPLICATE = "deduplicate";
static const std::string COMMAND_REBUILD_BLOOM = "rebuild_bloom";
static const std::string COMMAND_QUERY_HASH = "query_hash";
static const std::string COMMAND_GET_HASH_SOURCE = "get_hash_source";
static const std::string COMMAND_GET_HASHDB_INFO = "get_hashdb_info";
static const std::string COMMAND_SERVER = "server";
*/

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
static std::string repository_name = "";

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
  if (has_hashdb_settings) {
    std::cerr << "bloom filter settings are not allowed in this command.\n";
    exit(1);
  }
}
void require_no_repository_name() {
  if (has_hashdb_settings) {
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

// ************************************************************
// main
// ************************************************************
int main(int argc,char **argv) {
  command_line_t::command_line_string = dfxml_writer::make_command_line(argc, argv);

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
      {"Version", no_argument, 0, 'V'},

      // hashdb settings
      {"hash_block_size", required_argument, 0, 'p'},
      {"max_duplicates", required_argument, 0, 'm'},
      {"storage_type", required_argument, 0, 't'},
      {"algorithm", required_argument, 0, 'a'},
      {"bits", required_argument, 0, 'b'},

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

    int ch = getopt_long(argc, argv, "hHVp:m:t:a:b:A:B:C:D:E:F:r", long_options, &option_index);
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
      case 'H': {	// help
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
      case 't': {	// storage type
        has_hashdb_settings = true;
        bool is_ok_map = string_to_map_type(optarg, hashdb_settings.map_type);
        bool is_ok_multimap = string_to_multimap_type(optarg, hashdb_settings.multimap_type);
        if (!is_ok_map || !is_ok_multimap) {
          std::cerr << "Invalid value for storage type: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'a': {	// hash algorithm
        has_hashdb_settings = true;
        bool is_ok_hashdigest_type = string_to_hashdigest_type(optarg, hashdb_settings.hashdigest_type);
        if (!is_ok_hashdigest_type) {
          std::cerr << "Invalid value for hashdigest type: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'b': {	// number of index bits
        has_hashdb_settings = true;
        // boost can't cast to uint8_t
        uint32_t temp = boost::lexical_cast<uint32_t>(optarg);
        if (temp < 32 || temp > 40) {
          std::cerr << "Invalid value for number of index bits: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        hashdb_settings.source_lookup_index_bits = (uint8_t)temp;
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
        repository_name = optarg;
        break;
      }
/*
      case 's': {	// server socket endpoint
        has_server_path_or_socket = true;
        zz server_path_or_socket = optarg;
        break;
      }
*/
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
  const std::string command(argv[0]);
  argc--;
  argv++;

  // get any arguments
  parameter_count = argc;
  const std::string arg1((argc>=1) ? argv[0] : "");
  const std::string arg2((argc>=2) ? argv[1] : "");
  const std::string arg3((argc>=3) ? argv[2] : "");

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

  // generate usable repository name if one is not provided
  // this works globally because all commands that use repository_name
  // uniformly require arg1
  if (repository_name == "") {
    repository_name = "repository_" + arg1;
  }

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
    commands_t::create(hashdb_settings, arg1);
  } else if (command == "import") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_parameter_count(2);
    commands_t::import(repository_name, arg1, arg2);
  } else if (command == "export") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::do_export(arg1, arg2);
  } else if (command == "copy") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::copy(arg1, arg2);
  } else if (command == "merge") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(3);
    commands_t::merge(arg1, arg2, arg3);
  } else if (command == "remove") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::remove(arg1, arg2);
  } else if (command == "remove_all") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::remove_all(arg1, arg2);
  } else if (command == "remove_dfxml") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_parameter_count(2);
    commands_t::remove_dfxml(repository_name, arg1, arg2);
  } else if (command == "remove_all_dfxml") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::remove_all_dfxml(arg1, arg2);
  } else if (command == "deduplicate") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::deduplicate(arg1, arg2);
  } else if (command == "rebuild_bloom") {
    require_no_hashdb_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t::rebuild_bloom(hashdb_settings, arg1);
  } else if (command == "server") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::server(arg1, arg2);
  } else if (command == "query_hash") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(2);
    commands_t::query_hash(arg1, arg2);
  } else if (command == "get_hash_source") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(3);
    commands_t::get_hash_source(arg1, arg2, arg3);
  } else if (command == "get_hashdb_info") {
    require_no_hashdb_settings();
    require_no_bloom_filter_settings();
    require_no_repository_name();
    require_parameter_count(1);
    commands_t::get_hashdb_info(arg1);
  } else {
    std::cerr << "Error: '" << command << "' is not a recognized command.  " << see_usage << "\n";
  }

  // done
  return 0;
}

