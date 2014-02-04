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
#include "hashdb_runtime_options.hpp"
#include "usage.hpp"
#include "command_line.hpp" // zz make this better

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

//uint64_t approximate_M_to_n(uint32_t M);
//uint32_t approximate_n_to_M(uint64_t n);

static const std::string see_usage = "Please type 'hashdb -h' for usage.";

// user commands, which are not the demultiplexed commands seen in commands.hpp
// create       a new hashdb
// import       hashes from DFXML to hashdb
// export       hashdb to a DFXML file
// copy         hashdb to second hashdb
// merge        hashdb1 and hashdb2 into hashdb3
// remove       hashes in hashdb1 from hashdb2
// remove_dfxml hashes in dfxml from hashdb
// deduplicate  some or all hash duplicates
// rebuild_bloom rebuild bloom filters
// query_hash   print hashes in DFXML file that match hashdb
// get_hash_source create file with hash sources from identified_blocks.txt
// get_hashdb_info show hashdb database statistics
// server       run as server

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

// settings and options
static hashdb_settings_t hashdb_settings = hashdb_settings_t();
static hashdb_runtime_options_t hashdb_runtime_options = hashdb_runtime_options_t();

// option state used to assure options are not specified when they should not be
// option categories
//static bool has_create_options = false;
//static bool has_bloom_options = false;
static bool has_repository_name = false;
static bool has_server_path_or_socket = false;

// bloom filter validation
bool has_b1n = false;
bool has_b1kM = false;
bool has_b2n = false;
bool has_b2kM = false;

//zz these go away
bool has_tuning = false;
bool has_tuning_bloom = false;
bool has_exclude_duplicates = false;

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

/*
void no_has_tuning(const std::string& action) {
  if (has_tuning) {
    std::cerr << "hashdb tuning parameters are not allowed in command to " << action << ".\n";
    exit(1);
  }
}
void no_has_tuning_bloom(const std::string& action) {
  if (has_tuning_bloom) {
    std::cerr << "Bloom filter tuning parameters are not allowed in command to " << action << ".\n";
    exit(1);
  }
}
void no_has_repository_name(const std::string& action) {
  if (has_repository_name) {
    std::cerr << "The \"--repository\" option is not allowed in command to " << action << ".\n";
    exit(1);
  }
}
void no_has_server_socket_endpoint(const std::string& action) {
  if (has_server_path_or_socket) {
    std::cerr << "The \"--socket\" option is not allowed in command to " << action << ".\n";
    exit(1);
  }
}
void no_has_exclude_duplicates(const std::string& action) {
  if (has_exclude_duplicates) {
    std::cerr << "The \"--exclude_duplicates\" option is not allowed in command to " << action << ".\n";
    exit(1);
  }
}
*/

/*
void require_hash_block_sizes_match(const std::string& hashdb_dir1, const std::string& hashdb_dir2,
                               const std::string& action) {
  hashdb_settings_t settings1;
  hashdb_settings_reader_t::read_settings(hashdb_dir1+"settings.xml", settings1);
  hashdb_settings_t settings2;
  hashdb_settings_reader_t::read_settings(hashdb_dir2+"settings.xml", settings2);

  if (settings1.hash_block_size != settings2.hash_block_size) {
    std::cerr << "Error: The hash block size for the databases do not match.\n";
    std::cerr << "The hash block size for " << hashdb_dir1 << " is " << settings1.hash_block_size << "\n";
    std::cerr << "but the hash block size for " << hashdb_dir2 << " is " << settings2.hash_block_size << ".\n";
    std::cerr << "Aborting command to " << action << ".\n";
    exit(1);
  }
}
*/

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

      // command options
      {"repository", required_argument, 0, 'r'},
      {"socket", required_argument, 0, 's'},
      {"exclude_duplicates", required_argument, 0, 'x'},

      // tuning
      {"hash_block_size", required_argument, 0, 'p'},
      {"max_duplicates", required_argument, 0, 'm'},
      {"storage_type", required_argument, 0, 't'},
      {"shards", required_argument, 0, 'n'},
      {"bits", required_argument, 0, 'i'},

      // bloom tuning
      {"b1", required_argument, 0, 'A'},
      {"b1n", required_argument, 0, 'B'},
      {"b1kM", required_argument, 0, 'C'},
      {"b2", required_argument, 0, 'D'},
      {"b2n", required_argument, 0, 'E'},
      {"b2kM", required_argument, 0, 'F'},
      {0,0,0,0}
    };

    int ch = getopt_long(argc, argv, "hHVr:s:x:p:m:t:n:i:A:B:C:D:E:F", long_options, &option_index);
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
      case 'r': {	// repository name
        has_repository_name = true;
        hashdb_runtime_options.repository_name = optarg;
        break;
      }
      case 's': {	// server socket endpoint
        has_server_path_or_socket = true;
        hashdb_runtime_options.server_path_or_socket = optarg;
        break;
      }
      case 'x': {	// remove duplicates during copy
        has_exclude_duplicates = true;
        try {
          hashdb_runtime_options.exclude_duplicates_count = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for exclude duplicates count: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'p': {	// hash block size
        has_tuning = true;
        try {
          hashdb_settings.hash_block_size = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for hash_block_size: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'm': {	// maximum hash duplicates
        has_tuning = true;
        try {
          hashdb_settings.maximum_hash_duplicates = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for maximum hash duplicates: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 't': {	// storage type
        has_tuning = true;
        bool is_ok_map = string_to_map_type(optarg, hashdb_settings.map_type);
        bool is_ok_multimap = string_to_multimap_type(optarg, hashdb_settings.multimap_type);
        if (!is_ok_map || !is_ok_multimap) {
          std::cerr << "Invalid value for storage type: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'n': {	// number of shards
        has_tuning = true;
        try {
          hashdb_settings.map_shard_count = boost::lexical_cast<size_t>(optarg);
          hashdb_settings.multimap_shard_count = boost::lexical_cast<size_t>(optarg);
        } catch (...) {
          std::cerr << "Invalid value for number of shards: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'i': {	// number of index bits
        has_tuning = true;
        // boost can't cast to uint8_t
        uint32_t temp = boost::lexical_cast<uint32_t>(optarg);
        if (temp < 32 || temp > 40) {
          std::cerr << "Invalid value for number of index bits: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        hashdb_settings.number_of_index_bits = (uint8_t)temp;
        break;
      }
      case 'A': {	// b1 bloom 1 state
        has_tuning_bloom = true;
        bool is_ok_bloom1_state = string_to_bloom_state(optarg, hashdb_settings.bloom1_is_used);
        if (!is_ok_bloom1_state) {
          std::cerr << "Invalid value for bloom filter 1 state: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'B': {	// b1n bloom 1 expected total number of hashes
        has_tuning_bloom = true;
        has_b1n = true;
        try {
          uint64_t n1 = boost::lexical_cast<uint64_t>(optarg);
          hashdb_settings.bloom1_k_hash_functions = 3;
          hashdb_settings.bloom1_M_hash_size = approximate_n_to_M(n1);
        } catch (...) {
          std::cerr << "Invalid value for bloom filter 1 expected total number of hashes: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'C': {	// b1kM bloom 1 k hash functions and M hash size
        has_tuning_bloom = true;
        has_b1kM = true;
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
        has_tuning_bloom = true;
        bool is_ok_bloom2_state = string_to_bloom_state(optarg, hashdb_settings.bloom2_is_used);
        if (!is_ok_bloom2_state) {
          std::cerr << "Invalid value for bloom filter 2 state: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'E': {	// b2n bloom 2 expected total number of hashes
        has_tuning_bloom = true;
        has_b2n = true;
        try {
          uint64_t n2 = boost::lexical_cast<uint64_t>(optarg);
          hashdb_settings.bloom2_k_hash_functions = 3;
          hashdb_settings.bloom2_M_hash_size = approximate_n_to_M(n2);
        } catch (...) {
          std::cerr << "Invalid value for bloom filter 2 expected total number of hashes: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }
        break;
      }
      case 'F': {	// b2kM bloom 2 k hash functions and M hash size
        has_tuning_bloom = true;
        has_b2kM = true;
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
      default:
//        std::cerr << "unexpected command character " << ch << "\n";
        exit(1);
    }
  }

  // ************************************************************
  // prepare to run the command
  // ************************************************************

/*
  // check that bloom filter n or kM are selected but not both
  if ((has_b1n && has_b1kM) || (has_b2n && has_b2kM)) {
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
*/

  // now parse tokens that are not consumed by options
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
//zz not yet  const int num_args = argc;
  const std::string arg1((argc>=1) ? argv[0] : "");
  const std::string arg2((argc>=2) ? argv[1] : "");
  const std::string arg3((argc>=3) ? argv[2] : "");

  // generate usable repository name if one is not provided
  // this works globally because all commands that use hashdb_runtime_options.repository_name
  // uniformly require arg1
  if (hashdb_runtime_options.repository_name == "") {
    hashdb_runtime_options.repository_name = "repository_" + arg1;
  }

  // ************************************************************
  // run the command
  // ************************************************************
// create       a new hashdb
// import       hashes from DFXML to hashdb
// export       hashdb to a DFXML file
// copy         hashdb to second hashdb
// merge        hashdb1 and hashdb2 into hashdb3
// remove       hashes in hashdb1 from hashdb2
// remove_dfxml hashes in dfxml from hashdb
// deduplicate  some or all hash duplicates
// rebuild_bloom rebuild bloom filters
// query_hash   print hashes in DFXML file that match hashdb
// get_hash_source create file with hash sources from identified_blocks.txt
// get_hashdb_info show hashdb database statistics
// server       run as server

  if (command == "create") {
    // etc.
  }

/*
  // copy
  // copy hashes from dfxml or hashdb to new or existing hashdb
  if (command == COMMAND_COPY) {
    // validate argument count
    if (num_args != 2) {
      std::cerr << "The copy command requires 2 parameters.  " << see_usage << "\n";
      exit(1);
    }
std::cerr << "copy.a num args " << num_args << " arg1: '" << arg1 << "' arg2: '" << arg2 << "' arg3: '" << arg3 << "'\n";

    // copy dfxml to new hashdb
    if (is_dfxml(arg1) && !is_present(arg2)) {
      no_has_server_socket_endpoint(ACTION_COPY_DFXML_NEW);
      no_has_exclude_duplicates(ACTION_COPY_DFXML_NEW);

      create_hashdb(arg2, hashdb_settings);
      commands_t::do_copy_new_dfxml(arg1, hashdb_runtime_options.repository_name, arg2);

    // copy dfxml modifying existing hashdb
    } else if (is_dfxml(arg1) && is_hashdb(arg2)) {
      no_has_tuning(ACTION_COPY_DFXML_EXISTING);
      no_has_tuning_bloom(ACTION_COPY_DFXML_EXISTING);
      no_has_server_socket_endpoint(ACTION_COPY_DFXML_EXISTING);
      no_has_exclude_duplicates(ACTION_COPY_DFXML_EXISTING);

      commands_t::do_copy_dfxml(arg1, hashdb_runtime_options.repository_name, arg2);

    // copy hashdb to new hashdb
    } else if (is_hashdb(arg1) && !is_present(arg2) && !has_exclude_duplicates) {
      no_has_repository_name(ACTION_COPY_NEW);
      no_has_server_socket_endpoint(ACTION_COPY_NEW);

      create_hashdb(arg2, hashdb_settings);
      require_hash_block_sizes_match(arg1, arg2, ACTION_COPY_NEW);
      commands_t::do_copy_new(arg1, arg2);

    // copy hashdb to new hashdb, excluding all duplicates
    } else if (is_hashdb(arg1) && !is_present(arg2) && has_exclude_duplicates) {
      no_has_repository_name(ACTION_COPY_NEW_EXCLUDE_DUPLICATES);
      no_has_server_socket_endpoint(ACTION_COPY_NEW_EXCLUDE_DUPLICATES);

      create_hashdb(arg2, hashdb_settings);
      require_hash_block_sizes_match(arg1, arg2, ACTION_COPY_NEW_EXCLUDE_DUPLICATES);
      commands_t::do_copy_new_exclude_duplicates(arg1, arg2, hashdb_runtime_options.exclude_duplicates_count);

    // copy hashdb to existing hashdb
    } else if (is_hashdb(arg1) && is_hashdb(arg2)) {
      no_has_tuning(ACTION_COPY_EXISTING);
      no_has_tuning_bloom(ACTION_COPY_EXISTING);
      no_has_repository_name(ACTION_COPY_EXISTING);
      no_has_server_socket_endpoint(ACTION_COPY_EXISTING);
      no_has_exclude_duplicates(ACTION_COPY_EXISTING);

      require_hash_block_sizes_match(arg1, arg2, ACTION_COPY_EXISTING);
      commands_t::do_copy(arg1, arg2);

    // else something went wrong
    } else {
      // program error if here
      std::cerr << "The copy command failed.  Please check the filenames provided.\n";
      exit(1);
    }

  // remove
  // remove hashes in dfxml or hashdb from hashdb
  } else if (command == COMMAND_REMOVE) {
    // validate argument count
    if (num_args != 2) {
      std::cerr << "The remove command requires 2 parameters.  " << see_usage << "\n";
      exit(1);
    }

    // remove dfxml from hashdb
    if (is_dfxml(arg1) && is_hashdb(arg2)) {
      no_has_tuning(ACTION_REMOVE_DFXML);
      no_has_tuning_bloom(ACTION_REMOVE_DFXML);
      no_has_server_socket_endpoint(ACTION_REMOVE_DFXML);
      no_has_exclude_duplicates(ACTION_REMOVE_DFXML);

      commands_t::do_remove_dfxml(arg1, hashdb_runtime_options.repository_name, arg2);

    // remove hashdb from hashdb
    } else if (is_hashdb(arg1) && is_hashdb(arg2)) {
      no_has_tuning(ACTION_REMOVE);
      no_has_tuning_bloom(ACTION_REMOVE);
      no_has_repository_name(ACTION_REMOVE);
      no_has_server_socket_endpoint(ACTION_REMOVE);
      no_has_exclude_duplicates(ACTION_REMOVE);

      require_hash_block_sizes_match(arg1, arg2, ACTION_REMOVE);
      commands_t::do_remove(arg1, arg2);

    // else something went wrong
    } else {
      // program error if here
      std::cerr << "The remove command failed.  Please check the filenames provided.\n";
      exit(1);
    }

  // merge
  // merge hashes in hashdb 1 and hashdb into new hashdb3
  } else if (command == COMMAND_MERGE) {
    // validate argument count
    if (num_args != 3) {
      std::cerr << "The merge command requires 3 parameters.  " << see_usage << "\n";
      exit(1);
    }

    no_has_repository_name(ACTION_MERGE);
    no_has_server_socket_endpoint(ACTION_MERGE);
    no_has_exclude_duplicates(ACTION_MERGE);

    create_hashdb(arg3, hashdb_settings);
    require_hash_block_sizes_match(arg1, arg2, ACTION_MERGE);
    require_hash_block_sizes_match(arg1, arg3, ACTION_MERGE);
    commands_t::do_merge(arg1, arg2, arg3);

  // rebuild bloom
  // rebuild hashdb bloom filters
  } else if (command == COMMAND_REBUILD_BLOOM) {
    // validate argument count
    if (num_args != 1) {
      std::cerr << "The rebuild_bloom command requires 1 parameter.  " << see_usage << "\n";
      exit(1);
    }

    no_has_tuning(ACTION_REBUILD_BLOOM);
    no_has_repository_name(ACTION_REBUILD_BLOOM);
    no_has_server_socket_endpoint(ACTION_REBUILD_BLOOM);
    no_has_exclude_duplicates(ACTION_REBUILD_BLOOM);

    // change existing bloom settings
    reset_bloom_filters(arg1, hashdb_settings);
    commands_t::do_rebuild_bloom(arg1);

  // export
  // export hashes in hashdb to dfxml
  } else if (command == COMMAND_EXPORT) {
    // validate argument count
    if (num_args != 2) {
      std::cerr << "The export command requires 2 parameters.  " << see_usage << "\n";
      exit(1);
    }

    no_has_tuning(ACTION_EXPORT);
    no_has_tuning_bloom(ACTION_EXPORT);
    no_has_repository_name(ACTION_EXPORT);
    no_has_server_socket_endpoint(ACTION_EXPORT);
    no_has_exclude_duplicates(ACTION_EXPORT);

    commands_t::do_export(arg1, arg2);

  // info
  // info for hashdb
  } else if (command == COMMAND_INFO) {
    // validate argument count
    if (num_args != 1) {
      std::cerr << "The info command requires 1 parameter.  " << see_usage << "\n";
      exit(1);
    }

    no_has_tuning(ACTION_INFO);
    no_has_tuning_bloom(ACTION_INFO);
    no_has_repository_name(ACTION_INFO);
    no_has_exclude_duplicates(ACTION_INFO);

    commands_t::do_info(arg1);

  // server
  // server accessing hashdb
  } else if (command == COMMAND_SERVER) {
    // validate argument count
    if (num_args != 1) {
      std::cerr << "The server command requires 1 parameter.  " << see_usage << "\n";
      exit(1);
    }

    no_has_tuning(ACTION_SERVER);
    no_has_tuning_bloom(ACTION_SERVER);
    no_has_repository_name(ACTION_SERVER);
    no_has_exclude_duplicates(ACTION_SERVER);

    commands_t::do_server(arg1, hashdb_runtime_options.server_socket_endpoint);

  } else {
    std::cerr << "Error: '" << command << "' is not a recognized command.  " << see_usage << "\n";
  }
*/

  // done
  return 0;
}

