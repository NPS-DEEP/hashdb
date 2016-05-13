// Author:  Bruce Allen
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
 * \brief Block Hash Database (hashdb)
 * \mainpage Block Hash Database (hashdb) Tool and Library
 * \section intro_sec Introduction
 * __hashdb__ is a tool for finding previously identified blocks of data
 * in media such as disk images. hashdb provides the following:
 * - The hashdb tool used to create hash databases, import block hashes,
 *   provide lookup services, and otherwise manage block hash databases.
 * - The hashdb library that allows other programs to create hash databases
 *   or scan for block hashes. The hashdb scanner in bulk_extractor uses
 *   libhashdb to search for previously identified blocks of data.
 *
 * Please see the hashdb home page at <https://github.com/simsong/hashdb/wiki>.
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
#include "usage.hpp"
#include "commands.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <getopt.h>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

#include "../src_libhashdb/hashdb.hpp" // for settings

// default settings
static const std::string default_repository_name = "";
static const std::string default_whitelist_dir = "";

// usage
static const std::string see_usage = "Please type 'hashdb -h' for usage.";

// user-selected options
static bool has_quiet = false;
static bool has_byte_alignment = false;
static bool has_block_size = false;
static bool has_step_size = false;
static bool has_repository_name = false;
static bool has_whitelist_dir = false;
static bool has_embedded_data = false;
static bool has_max_source_offset_pairs = false;
static bool has_tuning = false;

// option values
hashdb::settings_t settings;
static std::string repository_name = default_repository_name;
static std::string whitelist_dir = default_whitelist_dir;
static size_t step_size = settings.block_size;

// arguments
static std::string cmd= "";         // the command line invocation text
static std::string command = "";    // the hashdb command
static std::vector<std::string> args;

// helper functions
void run_command();
void usage() {
  // usage.hpp
  usage(default_repository_name, default_whitelist_dir);
}

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

// from dfxml/dfxml_writer.h
static std::string make_command_line(int argc,char * const *argv){
    std::string command_line;
    for(int i=0;i<argc;i++){
        // append space separator between arguments
        if(i>0) command_line.push_back(' ');
        if (strchr(argv[i],' ') != NULL) {
            // the argument has a space, so quote the argument
            command_line.append("\"");
            command_line.append(argv[i]);
            command_line.append("\"");
        } else {
            // the argument has no space, so append as is
            command_line.append(argv[i]);
        }
    }
    return command_line;
}

int main(int argc,char **argv) {

#ifdef HAVE_MCHECK
  mtrace();
#endif

  // manage error condition of no arguments
  if(argc==1) {
      usage();
      exit(1);
  }

  // compose the command line before parsing input
  cmd = make_command_line(argc, argv);

  // parse options
  int option_index; // not used by hashdb
  while (1) {

    const struct option long_options[] = {
      // options
      {"help",                          no_argument, 0, 'h'},
      {"Help",                          no_argument, 0, 'H'},
      {"version",                       no_argument, 0, 'v'},
      {"Version",                       no_argument, 0, 'V'},
      {"quiet",                         no_argument, 0, 'q'},
      {"byte_alignment",          required_argument, 0, 'a'},
      {"block_size",              required_argument, 0, 'b'},
      {"step_size",               required_argument, 0, 's'},
      {"repository_name",         required_argument, 0, 'r'},
      {"whitelist_dir",           required_argument, 0, 'w'},
      {"embedded_data",           required_argument, 0, 'e'},
      {"max_source_offset_pairs", required_argument, 0, 'm'},
      {"tuning",                  required_argument, 0, 't'},

      // end
      {0,0,0,0}
    };

    int ch = getopt_long(argc, argv, "hHvVqa:b:s:r:w:em:t:",
                         long_options, &option_index);
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
        has_quiet = true;
        break;
      }

      case 'a': {	// byte alignment
        has_byte_alignment = true;
        settings.byte_alignment = std::atoi(optarg);
        break;
      }

      case 'b': {	// block size
        has_block_size = true;
        settings.block_size = std::atoi(optarg);
        break;
      }

      case 's': {	// block size
        has_step_size = true;
        step_size = std::atoi(optarg);
        break;
      }

      case 'r': {	// repository name
        has_repository_name = true;
        repository_name = std::string(optarg);
        break;
      }

      case 'w': {	// whitelist directory
        has_whitelist_dir = true;
        whitelist_dir = std::string(optarg);
        break;
      }

      case 'e': {	// process embedded data
        has_embedded_data = true;
        break;
      }

      case 'm': {	// max source file offset pairs
        has_max_source_offset_pairs = true;
        settings.max_source_offset_pairs = std::atoi(optarg);
        break;
      }

      case 't': {	// tuning hash_prefix_bits:hash_suffix_bytes
        has_tuning = true;
        std::vector<std::string> params = split(std::string(optarg), ':');

        if (params.size() != 2) {
          std::cerr << "Invalid value for tuning: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }

        settings.hash_prefix_bits = std::atoi(params[0].c_str());
        settings.hash_suffix_bytes = std::atoi(params[1].c_str());
        break;
      }

      default:
//        std::cerr << "unexpected command character " << ch << "\n";
        exit(1);
    }
  }

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

  // get any arguments as vector<string> args
  for (int i=0; i<argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  // run the command
  run_command();

#ifdef HAVE_MCHECK
  muntrace();
#endif

  // done
  return 0;
}

void check_options(const std::string& options) {
  // fail if an option is not in the options set
  if (has_byte_alignment && options.find("a") == std::string::npos) {
    std::cerr << "The -a byte_alignment option is not allowed for this command.\n";
    exit(1);
  }
  if (has_block_size && options.find("b") == std::string::npos) {
    std::cerr << "The -b block_size option is not allowed for this command.\n";
    exit(1);
  }
  if (has_step_size && options.find("s") == std::string::npos) {
    std::cerr << "The -s step_size option is not allowed for this command.\n";
    exit(1);
  }
  if (has_repository_name && options.find("r") == std::string::npos) {
    std::cerr << "The -r repository_name option is not allowed for this command.\n";
    exit(1);
  }
  if (has_whitelist_dir && options.find("w") == std::string::npos) {
    std::cerr << "The -w whitelist_dir option is not allowed for this command.\n";
    exit(1);
  }
  if (has_embedded_data && options.find("e") == std::string::npos) {
    std::cerr << "The -e embedded_data option is not allowed for this command.\n";
    exit(1);
  }
  if (has_max_source_offset_pairs && options.find("m") ==
      std::string::npos) {
    std::cerr << "The -m max_source_offset_pairs option is not allowed for this command.\n";
    exit(1);
  }
  if (has_max_source_offset_pairs && options.find("t") ==
      std::string::npos) {
    std::cerr << "The -t tuning option is not allowed for this command.\n";
    exit(1);
  }

  // fail if block size is incompatible with byte alignment
  if (settings.byte_alignment == 0 ||
      (settings.block_size % settings.byte_alignment) != 0) {
    std::cerr << "Incompatible values for block size: "
              << settings.block_size
              << " and byte alignment: " << settings.byte_alignment
              << ".  block size must be divisible by byte alignment.\n"
              << see_usage << "\n";
    exit(1);
  }
}

void check_params(const std::string& options, size_t param_count) {
  // check options
  check_options(options);
  // check param count
  if (param_count != args.size()) {
    std::cerr << "The number of paramters provided is not valid for this command.\n";
    exit(1);
  }
}



void run_command() {
  // new database
  if (command == "create") {
    check_params("bamt", 1);
    commands::create(args[0], settings, cmd);

  // import
  } else if (command == "ingest") {
    check_params("srw", 2);
    if (repository_name == "") {
      repository_name = args[1];
    }
    commands::ingest(args[0], args[1], step_size,
                     repository_name, whitelist_dir, has_embedded_data, cmd);

  } else if (command == "import_tab") {
    check_params("rw", 2);
    if (repository_name == "") {
      repository_name = args[1];
    }
    commands::import_tab(args[0], args[1], repository_name, whitelist_dir, cmd);

  } else if (command == "import") {
    check_params("", 2);
    commands::import_json(args[0], args[1], cmd);

  } else if (command == "export") {
    check_params("", 2);
    commands::export_json(args[0], args[1], cmd);

  // database manipulation
  } else if (command == "add") {
    check_params("", 2);
    commands::add(args[0], args[1], cmd);

  } else if (command == "add_multiple") {
    check_options("");
    // check param count
    if (args.size() < 2) {
      std::cerr << "The number of paramters provided is not valid for this command.\n";
      exit(1);
    }
    commands::add_multiple(args, cmd);

  } else if (command == "add_repository") {
    check_params("", 3);
    commands::add_repository(args[0], args[1], args[2], cmd);

  } else if (command == "add_range") {
    check_params("", 3);
    size_t m = 0;
    size_t n = 0;
    std::string range = args[2];
    size_t colon_index = range.find(':');
    if (colon_index == std::string::npos) {
      std::cerr << "Range syntax is invalid and needs to include `:`.\n";
      exit(1);
    }
    if (colon_index != 0) {
      m = std::atoi(range.substr(0, colon_index).c_str());
    }
    if (colon_index + 1 != range.size()) {
      n = std::atoi(range.substr(colon_index + 1, std::string::npos).c_str());
    }
    commands::add_range(args[0], args[1], m, n, cmd);

  } else if (command == "intersect") {
    check_params("", 3);
    commands::intersect(args[0], args[1], args[2], cmd);

  } else if (command == "intersect_hash") {
    check_params("", 3);
    commands::intersect_hash(args[0], args[1], args[2], cmd);

  } else if (command == "subtract") {
    check_params("", 3);
    commands::subtract(args[0], args[1], args[2], cmd);

  } else if (command == "subtract_hash") {
    check_params("", 3);
    commands::subtract_hash(args[0], args[1], args[2], cmd);

  } else if (command == "subtract_repository") {
    check_params("", 3);
    commands::subtract_repository(args[0], args[1], args[2], cmd);

  // scan
  } else if (command == "scan_list") {
    check_params("", 2);
    commands::scan_list(args[0], args[1], cmd);

  } else if (command == "scan_hash") {
    check_params("", 2);
    commands::scan_hash(args[0], args[1], cmd);

  } else if (command == "scan_image") {
    check_params("s", 2);
    commands::scan_image(args[0], args[1], step_size, has_embedded_data);

  // statistics
  } else if (command == "size") {
    check_params("", 1);
    commands::size(args[0], cmd);

  } else if (command == "sources") {
    check_params("", 1);
    commands::sources(args[0], cmd);

  } else if (command == "histogram") {
    check_params("", 1);
    commands::histogram(args[0], cmd);

  } else if (command == "duplicates") {
    check_params("", 2);
    commands::duplicates(args[0], args[1], cmd);

  } else if (command == "hash_table") {
    check_params("", 2);
    commands::hash_table(args[0], args[1], cmd);

  // performance analysis
  } else if (command == "add_random") {
    check_params("", 2);
    commands::add_random(args[0], args[1], cmd);

  } else if (command == "scan_random") {
    check_params("", 2);
    commands::scan_random(args[0], args[1], cmd);

  } else if (command == "add_same") {
    check_params("", 2);
    commands::add_same(args[0], args[1], cmd);

  } else if (command == "scan_same") {
    check_params("", 2);
    commands::scan_same(args[0], args[1], cmd);

  // error
  } else {
    std::cerr << "Error: unsupported command.\nAborting.\n";
    exit(1);
  }
}

