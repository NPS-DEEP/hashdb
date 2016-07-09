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
#if defined(__MINGW64__) && defined(__cplusplus)
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
static bool has_help = false;
static bool has_byte_alignment = false;
static bool has_block_size = false;
static bool has_step_size = false;
static bool has_repository_name = false;
static bool has_whitelist_dir = false;
static bool has_disable_recursive_processing = false;
static bool has_disable_calculate_entropy = false;
static bool has_disable_calculate_labels = false;
static bool has_json_scan_mode = false;
static bool has_max_counts = false;
static bool has_tuning = false;

// option values
hashdb::settings_t settings;
static std::string repository_name = default_repository_name;
static std::string whitelist_dir = default_whitelist_dir;
static size_t step_size = settings.block_size;
static hashdb::scan_mode_t scan_mode = hashdb::scan_mode_t::EXPANDED_OPTIMIZED;

// arguments
static std::string cmd= "";         // the command line invocation text
static std::string command = "";    // the hashdb command
static std::vector<std::string> args;

// helper functions
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

static void set_disable_processing(const std::string& list) {
  for (std::string::const_iterator it=list.begin(); it!= list.end(); ++it) {
    if (*it == 'r') {
      has_disable_recursive_processing = true;
    } else if (*it == 'e') {
      has_disable_calculate_entropy = true;
    } else if (*it == 'l') {
      has_disable_calculate_labels = true;
    } else {
      std::cerr << "Invalid disable processing option: '" << *it
                << "'.  " << see_usage << "\n";
      exit(1);
    }
  }
}

static void set_scan_mode(const std::string& mode) {
  if (mode == "e") scan_mode = hashdb::scan_mode_t::EXPANDED;
  else if (mode == "o") scan_mode = hashdb::scan_mode_t::EXPANDED_OPTIMIZED;
  else if (mode == "c") scan_mode = hashdb::scan_mode_t::COUNT;
  else if (mode == "a") scan_mode = hashdb::scan_mode_t::APPROXIMATE_COUNT;
  else {
    std::cerr << "Invalid scan mode option: '" << mode
              << "'.  " << see_usage << "\n";
    exit(1);
  }
}

int main(int argc,char **argv) {

#ifdef HAVE_MCHECK
  mtrace();
#endif

  // manage error condition of no arguments
  if(argc==1) {
      usage::overview();
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
      {"byte_alignment",          required_argument, 0, 'a'},
      {"block_size",              required_argument, 0, 'b'},
      {"step_size",               required_argument, 0, 's'},
      {"repository_name",         required_argument, 0, 'r'},
      {"whitelist_dir",           required_argument, 0, 'w'},
      {"disable_processing",      required_argument, 0, 'x'},
      {"json_scan_mode",          required_argument, 0, 'j'},
      {"max_counts",              required_argument, 0, 'm'},
      {"tuning",                  required_argument, 0, 't'},

      // end
      {0,0,0,0}
    };

    int ch = getopt_long(argc, argv, "hHvVa:b:s:r:w:x:j:m:t:",
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
        has_help = true;
        break;
      }
      case 'H': {	// Help
        has_help = true;
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

      case 'x': {	// disable processing
        set_disable_processing(std::string(optarg));
        break;
      }

      case 'j': {	// select JSON mode
        has_json_scan_mode = true;
        set_scan_mode(std::string(optarg));
        break;
      }

      case 'm': {	// max counts
        has_max_counts = true;
        std::vector<std::string> params = split(std::string(optarg), ':');

        if (params.size() != 2) {
          std::cerr << "Invalid value for max counts: '" << optarg << "'.  " << see_usage << "\n";
          exit(1);
        }

        settings.max_count = std::atoi(params[0].c_str());
        settings.max_sub_count = std::atoi(params[1].c_str());
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

  // handle help requested without topic argument
  if (has_help && argc < 1) {
    usage::overview();
    return 0;
  }

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

  // if a help topic was requested, provide that instead
  if (has_help) {
    usage::usage(command);
    return 0;
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
  if (has_disable_recursive_processing && options.find("R") == std::string::npos) {
    std::cerr << "The -x r disable recursively processing embedded data option is not allowed for this command.\n";
    exit(1);
  }
  if (has_disable_calculate_entropy && options.find("E") == std::string::npos) {
    std::cerr << "The -x e disable calculate entropy option is not allowed for this command.\n";
    exit(1);
  }
  if (has_disable_calculate_labels && options.find("L") == std::string::npos) {
    std::cerr << "The -x l disable calculate labels option is not allowed for this command.\n";
    exit(1);
  }
  if (has_disable_calculate_labels && options.find("j") == std::string::npos) {
    std::cerr << "The -j JSON scan mode option is not allowed for this command.\n";
    exit(1);
  }
  if (has_max_counts && options.find("m") ==
      std::string::npos) {
    std::cerr << "The -m max_counts option is not allowed for this command.\n";
    exit(1);
  }
  if (has_tuning && options.find("t") ==
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
    check_params("srwREL", 2);
    if (repository_name == "") {
      repository_name = args[1];
    }
    commands::ingest(args[0], args[1], step_size,
             repository_name, whitelist_dir,
             has_disable_recursive_processing,
             has_disable_calculate_entropy,
             has_disable_calculate_labels,
             cmd);

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
    check_params("j", 2);
    commands::scan_list(args[0], args[1], scan_mode, cmd);

  } else if (command == "scan_hash") {
    check_params("j", 2);
    commands::scan_hash(args[0], args[1], scan_mode, cmd);

  } else if (command == "scan_image") {
    check_params("sRj", 2);
    commands::scan_image(args[0], args[1], step_size,
                         has_disable_recursive_processing, scan_mode, cmd);

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
    check_params("j", 2);
    commands::duplicates(args[0], args[1], scan_mode, cmd);

  } else if (command == "hash_table") {
    check_params("j", 2);
    commands::hash_table(args[0], args[1], scan_mode, cmd);

  } else if (command == "read_bytes") {
    check_params("", 3);
    commands::read_bytes(args[0], args[1], args[2]);

  // performance analysis
  } else if (command == "add_random") {
    check_params("", 2);
    commands::add_random(args[0], args[1], cmd);

  } else if (command == "scan_random") {
    check_params("j", 2);
    commands::scan_random(args[0], args[1], scan_mode, cmd);

  } else if (command == "add_same") {
    check_params("", 2);
    commands::add_same(args[0], args[1], cmd);

  } else if (command == "scan_same") {
    check_params("j", 2);
    commands::scan_same(args[0], args[1], scan_mode, cmd);

  } else if (command == "test_scan_stream") {
    check_params("j", 2);
    commands::test_scan_stream(args[0], args[1], scan_mode, cmd);

  // error
  } else {
    std::cerr << "Error: unsupported command '" << command << "'.\nAborting.\n";
    exit(1);
  }
}

