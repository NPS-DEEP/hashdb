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
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "usage.hpp"
#include "command_line.hpp"
#include "commands.hpp"
#include "globals.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/btree/helpers.hpp>
#include <getopt.h>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

static const std::string see_usage = "Please type 'hashdb -h' for usage.";

// options
static bool has_hash_block_size = false; // p
static bool has_max = false;             // m
static bool has_repository_name = false; // r
static bool has_sector_size = false;     // s
static bool has_bloom = false;                 // A
static bool has_bloom_n = false;               // B
static bool has_bloom_kM = false;              // C

// parsing
static size_t parameter_count = 0;

// option values
static uint32_t optional_hash_block_size = 0;
static uint32_t optional_max = 0;
static std::string repository_name_string = "";
static uint32_t optional_sector_size = 0;
static bool bloom_is_used = false;
static uint32_t bloom_k_hash_functions = 0;
static uint32_t bloom_M_hash_size = 0;

// arguments
static std::string command;
static std::string hashdb_arg1;
static std::string hashdb_arg2;
static std::string hashdb_arg3;

void run_command();

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

// ************************************************************
// main
// ************************************************************
int main(int argc,char **argv) {
  command_line_t::command_line_string = dfxml_writer::make_command_line(argc, argv);

#ifdef HAVE_MCHECK
  mtrace();
#endif

  // manage error condition of no arguments
  if(argc==1) {
      usage();
      exit(1);
  }

  // parse options
  int option_index; // not used
  while (1) {

    const struct option long_options[] = {
      // basic
      {"help", no_argument, 0, 'h'},
      {"Help", no_argument, 0, 'H'},
      {"version", no_argument, 0, 'v'},
      {"Version", no_argument, 0, 'V'},
      {"quiet", no_argument, 0, 'q'},
      {"flags", required_argument, 0, 'f'},

      // options
      {"hash_block_size", required_argument, 0, 'p'},
      {"max", required_argument, 0, 'm'},
      {"repository", required_argument, 0, 'r'},
      {"sector_size", required_argument, 0, 's'},

      // bloom filter settings
      {"bloom", required_argument, 0, 'A'},
      {"bloom_n", required_argument, 0, 'B'},
      {"bloom_kM", required_argument, 0, 'C'},

      // end
      {0,0,0,0}
    };

    int ch = getopt_long(argc, argv, "hHvVqf:p:m:r:s:A:B:C:", long_options, &option_index);
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
      case 'v': {	// version
        std::cout << "hashdb " << PACKAGE_VERSION << "\n";
        exit(0);
        break;
      }
      case 'V': {	// Version
        std::cout << "hashdb " << PACKAGE_VERSION << "\n";
        exit(0);
        break;
      }

      case 'q': {	// quiet mode
        globals_t::quiet_mode = true;
        break;
      }

      case 'f': {	// btree flags
        std::vector<std::string> flags = split(std::string(optarg), ':');
        for (std::vector<std::string>::iterator flags_it = flags.begin(); flags_it != flags.end(); ++flags_it) {
          if (*flags_it == "preload") {
            globals_t::btree_flags |= boost::btree::flags::preload;
          } else if (*flags_it == "cache_branches") {
            globals_t::btree_flags |= boost::btree::flags::cache_branches;
          } else if (*flags_it == "least_memory") {
            globals_t::btree_flags |= boost::btree::flags::least_memory;
          } else if (*flags_it == "low_memory") {
            globals_t::btree_flags |= boost::btree::flags::low_memory;
          } else if (*flags_it == "balanced") {
            globals_t::btree_flags |= boost::btree::flags::balanced;
          } else if (*flags_it == "fast") {
            globals_t::btree_flags |= boost::btree::flags::fast;
          } else if (*flags_it == "fastest") {
            globals_t::btree_flags |= boost::btree::flags::fastest;
          } else {
            std::cerr << "Invalid B-Tree flag value '" << *flags_it << "'\n";
            exit(1);
          }
        }
        break;
      }

      case 'p': {	// hash block size
        has_hash_block_size = true;
        try {
          optional_hash_block_size = boost::lexical_cast<uint32_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for hash_block_size: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }

        // make sure hash block size is valid
        if (optional_hash_block_size % HASHDB_BYTE_ALIGNMENT != 0) {
          std::cerr << "Invalid value for hash block size: "
                    << optional_hash_block_size
                    << ".  Value must be > 0 and divisible by "
                    << HASHDB_BYTE_ALIGNMENT << ".\n" << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'm': {	// maximum
        has_max = true;
        try {
          optional_max = boost::lexical_cast<uint32_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for max: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }

      // repository name option
      case 'r': {	// repository name
        has_repository_name = true;
        repository_name_string = std::string(optarg);
        break;
      }

      // sector size option
      case 's': {	// repository name
        has_sector_size = true;
        try {
          optional_sector_size = boost::lexical_cast<uint32_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for sector size: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }

      // bloom filter settings
      case 'A': {	// bloom <state>
        has_bloom = true;
        bool is_ok_bloom_state = string_to_bloom_state(optarg, bloom_is_used);
        if (!is_ok_bloom_state) {
          std::cerr << "Invalid value for bloom filter state: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'B': {	// bloom_n <n> expected total number of hashes
        has_bloom_n = true;
        try {
          uint64_t n = boost::lexical_cast<uint64_t>(optarg);
          bloom_k_hash_functions = 3;
          bloom_M_hash_size = bloom_filter_manager_t::approximate_n_to_M(n);
        } catch (...) {
          std::cerr << "Invalid value for bloom filter, expected total number of hashes: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'C': {	// bloom_kM <k:M> k hash functions and M hash size
        has_bloom_kM = true;
        std::vector<std::string> params = split(std::string(optarg), ':');

        if (params.size() == 2) {
          try {
            bloom_k_hash_functions = boost::lexical_cast<uint32_t>(params[0]);
            bloom_M_hash_size = boost::lexical_cast<uint32_t>(params[1]);
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

  // run the command
  run_command();

#ifdef HAVE_MCHECK
  muntrace();
#endif

  // done
  return 0;
}

void require_parameter_count(size_t count) {
  if (count != parameter_count) {
    std::cerr << "Incorrect number of parameters provided in this command.\n";
    std::cerr << "Expected " << count
              << ", but received " << parameter_count << ".\n";
    exit(1);
  }
}

void no_p() {
  if (has_hash_block_size) {
    std::cerr << "Error: the -p hash_block_size option is not allowed for this command.\n";
      exit(1);
    }
}
void no_m() {
  if (has_max) {
    std::cerr << "Error: the -m max option is not allowed for this command.\n";
      exit(1);
    }
}
void no_r() {
  if (has_repository_name) {
    std::cerr << "Error: the -r repository_name option is not allowed for this command.\n";
      exit(1);
    }
}
void no_s() {
  if (has_sector_size) {
    std::cerr << "Error: the -s sector_size option is not allowed for this command.\n";
      exit(1);
    }
}
void no_A() {
  if (has_bloom) {
    std::cerr << "Error: the -A bloom option is not allowed for this command.\n";
      exit(1);
    }
}
void no_B() {
  if (has_bloom_n) {
    std::cerr << "Error: the -B bloom_n option is not allowed for this command.\n";
      exit(1);
    }
}
void no_C() {
  if (has_bloom_kM) {
    std::cerr << "Error: the -B bloom_kM option is not allowed for this command.\n";
      exit(1);
    }
}
void manage_bloom_settings(hashdb_settings_t& settings) {
  // set bloom values in settings
  if (has_bloom) {
    settings.bloom1_is_used = bloom_is_used;
  }
  if (has_bloom_n || has_bloom_kM) {
    settings.bloom1_k_hash_functions = bloom_k_hash_functions;
    settings.bloom1_M_hash_size = bloom_M_hash_size;
  }

  // check that bloom filter n or kM are selected but not both
  if (has_bloom_n && has_bloom_kM) {
    std::cerr << "Error: either a Bloom filter n value or a bloom filter k:M value may be\n";
    std::cerr << "specified, but not both.  " << see_usage << "\n";
    exit(1);
  }

  // check that bloom filter parameters are valid
  try {
    bloom_filter_manager_t::validate_bloom_settings(settings);
  } catch (std::runtime_error& e) {
    std::cerr << "Bloom filter input error: " << e.what()
              << ".\nAborting.\n";
    exit(1);
  }
}

void run_command() {

  if (command == "create") {
    no_r(); no_s();
    require_parameter_count(1);
    hashdb_settings_t create_settings;
    if (has_hash_block_size) {
      create_settings.hash_block_size = optional_hash_block_size;
    }
    if (has_max) {
      create_settings.maximum_hash_duplicates = optional_max;
    }
    manage_bloom_settings(create_settings);
    commands_t::create(create_settings, hashdb_arg1);
  } else if (command == "import") {
    no_p(); no_m(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    // compose a repository name if one is not provided
    if (!has_repository_name) {
      repository_name_string = "repository_" + hashdb_arg1;
    }
    commands_t::import(hashdb_arg1, hashdb_arg2, repository_name_string);
  } else if (command == "import_tab") {
    no_p(); no_m(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    // compose a repository name if one is not provided
    if (!has_repository_name) {
      repository_name_string = "repository_" + hashdb_arg1;
    }
    uint32_t import_tab_sector_size = (has_sector_size) ? optional_sector_size :
                                   globals_t::default_import_tab_sector_size;
    commands_t::import_tab(hashdb_arg1, hashdb_arg2, repository_name_string,
                                                     import_tab_sector_size);
  } else if (command == "export") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::do_export(hashdb_arg1, hashdb_arg2);
  } else if (command == "add") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::add(hashdb_arg1, hashdb_arg2);
  } else if (command == "add_multiple") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::add_multiple(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "add_repository") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
  } else if (command == "intersect") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::intersect(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "intersect_hash") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::intersect_hash(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "subtract") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::subtract(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "subtract_hash") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::subtract_hash(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "deduplicate") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::deduplicate(hashdb_arg1, hashdb_arg2);
  } else if (command == "scan") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::scan(hashdb_arg1, hashdb_arg2);
  } else if (command == "scan_hash") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::scan_hash(hashdb_arg1, hashdb_arg2);
  } else if (command == "scan_expanded") {
    no_p(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    uint32_t scan_expanded_max = (has_max) ? optional_max :
                                        globals_t::default_scan_expanded_max;
    commands_t::scan_expanded(hashdb_arg1, hashdb_arg2, scan_expanded_max);
  } else if (command == "scan_expanded_hash") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::scan_expanded_hash(hashdb_arg1, hashdb_arg2);
  } else if (command == "server") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::server(hashdb_arg1, hashdb_arg2);
  } else if (command == "size") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(1);
    commands_t::size(hashdb_arg1);
  } else if (command == "sources") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(1);
    commands_t::sources(hashdb_arg1);
  } else if (command == "histogram") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(1);
    commands_t::histogram(hashdb_arg1);
  } else if (command == "duplicates") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::duplicates(hashdb_arg1, hashdb_arg2);
  } else if (command == "hash_table") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(3);
    commands_t::hash_table(hashdb_arg1, hashdb_arg2, hashdb_arg3);
  } else if (command == "expand_identified_blocks") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::expand_identified_blocks(hashdb_arg1, hashdb_arg2);
  } else if (command == "explain_identified_blocks") {
    no_p(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    uint32_t identified_blocks_max = (has_max) ? optional_max :
                            globals_t::default_explain_identified_blocks_max;
    commands_t::explain_identified_blocks(hashdb_arg1, hashdb_arg2,
                                                      identified_blocks_max);
  } else if (command == "rebuild_bloom") {
    no_p(); no_m(); no_r(); no_s();
    require_parameter_count(1);
    hashdb_settings_t rebuild_settings;
    manage_bloom_settings(rebuild_settings);
    commands_t::rebuild_bloom(rebuild_settings, hashdb_arg1);
  } else if (command == "upgrade") {
    no_p(); no_m(); no_r(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(1);
    commands_t::upgrade(hashdb_arg1);
  } else if (command == "add_random") {
    no_p(); no_m(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    // compose a repository name if one is not provided
    if (!has_repository_name) {
      repository_name_string = "repository_" + hashdb_arg1;
    }
    commands_t::add_random(hashdb_arg1, hashdb_arg2, repository_name_string);
  } else if (command == "scan_random") {
    no_p(); no_m(); no_s(); no_A(); no_B(); no_C();
    require_parameter_count(2);
    commands_t::scan_random(hashdb_arg1, hashdb_arg2);
  } else {
    // invalid command
    std::cerr << "Error: '" << command << "' is not a recognized command.  " << see_usage << "\n";
  }
}

