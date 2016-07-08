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
 * Provides usage and detailed usage for the hashdb tool.
 */

#ifndef USAGE_HPP
#define USAGE_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include "../src_libhashdb/hashdb.hpp" // for settings

namespace usage {

void overview() {
  // print overview
  std::cout
  << "hashdb Version " << PACKAGE_VERSION  << "\n"
  << "Usage: hashdb [-h|--help|-h all] [-v|-V|--version]\n"
  << "       hashdb [-h <command>]\n"
  << "       hashdb [options] <command> [<args>]\n"
  << "\n"
  << "New Database:\n"
  << "  create [-b <block size>] [-a <byte alignment>]\n"
  << "         [-m <max count:max sub-count>]\n"
  << "         [-t <hash prefix bits:hash suffix bytes>]\n"
  << "\n"
  << "Import/Export:\n"
  << "  ingest [-r <repository name>] [-w <whitelist.hdb>] [-s <step size>]\n"
  << "         [-x <rel>] <hashdb.hdb> <import directory>\n"
  << "  import_tab [-r <repository name>] [-w <whitelist.hdb>] <hashdb> <tab file>\n"
  << "  import <hashdb> <json file>\n"
  << "  export <hashdb> <json file>\n"
  << "\n"
  << "Database Manipulation:\n"
  << "  add <source hashdb> <destination hashdb>\n"
  << "  add_multiple <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  add_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "  add_range <source hashdb> <destination hashdb> <m:n>\n"
  << "  intersect <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  intersect_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  subtract <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  subtract_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  subtract_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "\n"
  << "Scan:\n"
  << "  scan_list [-j e|o|c|a] <hashdb> <hash list file>\n"
  << "  scan_hash [-j e|o|c|a] <hashdb> <hex block hash>\n"
  << "  scan_image [-s <step size>] [-j e|o|c|a] [-x <r>] <hashdb> <media image>\n"
  << "\n"
  << "Statistics:\n"
  << "  size <hashdb>\n"
  << "  histogram <hashdb>\n"
  << "  duplicates [-j e|o|c|a] <hashdb> <number>\n"
  << "  hash_table [-j e|o|c|a] <hashdb> <hex file hash>\n"
  << "  read_bytes <media image> <offset> <count>\n"
  << "\n"
  << "Performance Analysis:\n"
  << "  add_random <hashdb> <hex file hash> <count>\n"
  << "  scan_random <hashdb> <count>\n"
  << "  add_same <hashdb> <hex file hash> <count>\n"
  << "  scan_same <hashdb> <count>\n"
  << "  test_scan_stream <hashdb> <count>\n"
  ;
}

// New Database
static void create() {

  const hashdb::settings_t settings;

  std::cout
  << "create [-b <block size>] [-a <byte alignment>]\n"
  << "       [-m <max count:max sub-count>]\n"
  << "       [-t <hash prefix bits:hash suffix bytes>]\n"
  << "  Create a new <hashdb> hash database.\n"
  << "\n"
  << "  Options:\n"
  << "  -b, --block_size=<block size>\n"
  << "    <block size>, in bytes, or use 0 for no restriction\n"
  << "    (default " << settings.block_size << ")\n"
  << "  -a, --byte_alignment=<n>\n"
  << "    <byte_alignment>, in bytes, or 1 for any alignment (default " << settings.byte_alignment << ")\n"
  << "  -m, --max_counts=<max count:max sub-count>\n"
  << "    The maximum number of source file offset references to store for a\n"
  << "    hash and the maximum number of source file offset references associated\n"
  << "    with a source to store for a hash (default " << settings.max_count << ":"<< settings.max_sub_count << ")\n"
  << "  -t, --tuning=<hash prefix bits:hash suffix bytes>\n"
  << "    The number of hash prefix bits and suffix bytes to use for\n"
  << "    optimizing hash storage (default " << settings.hash_prefix_bits << ":"
  <<        settings.hash_suffix_bytes << ")\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>   the file path to the new hash database to create\n"
  ;
}

// Import/Export
static void ingest() {
  std::cout
  << "ingest [-r <repository name>] [-w <whitelist.hdb>] [-s <step size>]\n"
  << "       [-x <rel>] <hashdb.hdb> <import directory>\n"
  << "  Import hashes recursively from <import directory> into hash database\n"
  << "    <hashdb>.\n"
  << "\n"
  << "  Options:\n"
  << "  -r, --repository_name=<repository name>\n"
  << "    The repository name to use for the set of hashes being imported.\n"
  << "    (default is \"repository_\" followed by the <import directory> path).\n"
  << "  -w, --whitelist_dir\n"
  << "    The path to a whitelist hash database.  Hashes matching this database\n"
  << "    will be marked with a whitelist entropy flag.\n"
  << "  -s, --step_size\n"
  << "    The step size to move along while calculating hashes.  Step size must\n"
  << "    be divisible by the byte alignment defined in the database.\n"
  << "  -x, --disable_processing\n"
  << "    Disable further processing:\n"
  << "      r disables recursively processing embedded data.\n"
  << "      e disables calculating entropy.\n"
  << "      l disables calculating block labels.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <import dir>   the directory to recursively import from\n"
  << "  <hashdb>       the hash database to insert the imported hashes into\n"
  ;
}

static void import_tab() {
  std::cout
  << "import_tab [-r <repository name>] [-w <whitelist.hdb>] <hashdb> <tab file>\n"
  << "  Import hashes from file <tab file> into hash database <hashdb>.\n"
  << "\n"
  << "  Options:\n"
  << "  -r, --repository_name=<repository name>\n"
  << "    The repository name to use for the set of hashes being imported.\n"
  << "    (default is \"repository_\" followed by the <import directory> path).\n"
  << "  -w, --whitelist_dir\n"
  << "    The path to a whitelist hash database.  Hashes matching this database\n"
  << "    will be marked with a whitelist entropy flag.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to insert the imported hashes into\n"
  << "  <NIST file>    the NIST file to import hashes from\n"
  ;
}

static void import() {
  std::cout
  << "import <hashdb> <json file>\n"
  << "  Import hashes from file <json file> into hash database <hashdb>.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to insert the imported hashes into\n"
  << "  <json file>    the JSON file to import hashes from\n"
  ;
}

static void export_json() {
  std::cout
  << "export <hashdb> <json file>\n"
  << "  Export hashes from hash database <hashdb> into file <json file>.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to export\n"
  << "  <json file>    the JSON file to export the hash database into.\n"
  ;
}

// Database Manipulation
static void add() {
  std::cout
  << "add <source hashdb> <destination hashdb>\n"
  << "  Copy hashes from the <source hashdb> to the <destination hashdb>.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb>       the source hash database to copy hashes from\n"
  << "  <destination hashdb>  the destination hash database to copy hashes into\n"
  ;
}

static void add_multiple() {
  std::cout
  << "add_multiple <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  Perform a union add of <source hashdb 1> and <source hashdb 2>\n"
  << "  into the <destination hashdb>.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb 1>     a hash database to copy hashes from\n"
  << "  <source hashdb 2>     a second hash database to copy hashes from\n"
  << "  <destination hashdb>  the destination hash database to copy hashes into\n"
  ;
}

static void add_repository() {
  std::cout
  << "add_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "  Copy hashes from the <source hashdb> to the <destination hashdb>\n"
  << "  when the <repository name> matches.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb>       the source hash database to copy hashes from\n"
  << "  <destination hashdb>  the destination hash database to copy hashes into\n"
  << "  <repository name>     the repository name to match when adding hashes\n"
  ;
}

static void add_range() {
  std::cout
  << "add_range <source hashdb> <destination hashdb> <m:n>\n"
  << "  Copy the hashes from the <source hashdb> to the <destination hashdb>\n"
  << "  that have source reference count values between m and n.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb>       the hash database to copy hashes from that have a\n"
  << "                        source count within range m and n\n"
  << "  <destination hashdb>  the hash database to copy hashes to when the\n"
  << "                        source count is within range m and n\n"
  << "  <m:n>                 the minimum and maximum count value range in which\n"
  << "                        hashes will be copied\n"
  ;
}

static void intersect() {
  std::cout
  << "intersect <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  Copy hashes that are common to both <source hashdb 1> and\n"
  << "  <source hashdb 2> into <destination hashdb>.  Hashes and their sources\n"
  << "  must match.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb 1>     a hash databases to copy the intersection of\n"
  << "  <source hashdb 2>     a second hash databases to copy the intersection of\n"
  << "  <destination hashdb>  the destination hash database to copy the\n"
  << "                        intersection of exact matches into\n"
  ;
}

static void intersect_hash() {
  std::cout
  << "intersect_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  Copy hashes that are common to both <source hashdb 1> and\n"
  << "  <source hashdb 2> into <destination hashdb>.  Hashes match when hash\n"
  << "  values match, even if their associated source repository name and\n"
  << "  filename do not match.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb 1>     a hash databases to copy the intersection of\n"
  << "  <source hashdb 2>     a second hash databases to copy the intersection of\n"
  << "  <destination hashdb>  the destination hash database to copy the\n"
  << "                        intersection of hashes into\n"
  ;
}

static void subtract() {
  std::cout
  << "subtract <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  Copy hashes that are in <souce hashdb 1> and not in <source hashdb 2>\n"
  << "  into <destination hashdb>.  Hashes and their sources must match.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb 1>     the hash database containing hash values to be\n"
  << "                        added if they are not also in the other database\n"
  << "  <source hashdb 2>     the hash database containing the hash values that\n"
  << "                        will not be added\n"
  << "  <destination hashdb>  the hash database to add the difference of the\n"
  << "                        exact matches into\n"
  ;
}

static void subtract_hash() {
  std::cout
  << "subtract_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "  Copy hashes that are in <souce hashdb 1> and not in <source hashdb 2>\n"
  << "  into <destination hashdb>.  Hashes match when hash values match, even if\n"
  << "  their associated source repository name and filename do not match.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb 1>     the hash database containing hash values to be\n"
  << "                        added if they are not also in the other database\n"
  << "  <source hashdb 2>     the hash database containing the hash values that\n"
  << "                        will not be added\n"
  << "  <destination hashdb>  the hash database to add the difference of the\n"
  << "                        hashes into\n"
  ;
}

static void subtract_repository() {
  std::cout
  << "subtract_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "  Copy hashes from the <source hashdb> to the <destination hashdb>\n"
  << "  when the <repository name> does not match.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <source hashdb>       the source hash database to copy hashes from\n"
  << "  <destination hashdb>  the destination hash database to copy hashes into\n"
  << "  <repository name>     the repository name to exclude when adding hashes\n"
  ;
}

static void scan_list() {
  std::cout
  << "scan_list [-j e|o|c|a] <hashdb> <hash list file>\n"
  << "  Scan hash database <hashdb> for hashes in <hash list file> and print out\n"
  << "  matches.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "  -x, --disable_processing\n"
  << "    Disable further processing:\n"
  << "      r disables recursively processing embedded data.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>          the file path to the hash database to use as the\n"
  << "                    lookup source\n"
  << "  <hashes file>     the file containing hash values to scan for\n"
  ;
}

void scan_hash() {
  std::cout
  << "scan_hash [-j e|o|c|a] <hashdb> <hex block hash>\n"
  << "  Scan hash database <hashdb> for the specified <hash value> and print\n"
  << "  out matches.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>          the file path to the hash database to use as the\n"
  << "                    lookup source\n"
  << "  <hex block hash>  the hash value to scan for\n"
  ;
}

void scan_image() {
  std::cout
  << "scan_image [-s <step size>] [-j e|o|c|a] [-x <r>] <hashdb> <media image>\n"
  << "  Scan hash database <hashdb> for hashes in <media image> and print out\n"
  << "  matches.\n"
  << "\n"
  << "  Options:\n"
  << "  -s, --step_size\n"
  << "    The step size to move along while calculating hashes.  Step size must\n"
  << "    be divisible by the byte alignment defined in the database.\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "  -x, --disable_processing\n"
  << "    Disable further processing:\n"
  << "      r disables recursively processing embedded data.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>          the file path to the hash database to use as the\n"
  << "                    lookup source\n"
  << "  <media image>     the media image file to scan for matching block hashes\n"
  ;
}

// Statistics
static void size() {
  std::cout
  << "size <hashdb>\n"
  << "  Print the sizes of the database tables inside the given <hashdb> database.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to print size information for\n"
  ;
}

static void sources() {
  std::cout
  << "sources <hashdb>\n"
  << "  Print source information indicating where the hashes in the <hashdb>\n"
  << "  came from.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to print all the repository name,\n"
  << "                 filename source information for\n"
  ;
}

static void histogram() {
  std::cout
  << "histogram <hashdb>\n"
  << "  Print the histogram of hashes for the given <hashdb> database.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to print the histogram of hashes for\n"
  ;
}

static void duplicates() {
  std::cout
  << "duplicates [-j e|o|c|a] <hashdb> <number>\n"
  << "  Print the hashes in the given <hashdb> database that are sourced the\n"
  << "  given <number> of times.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to print duplicate hashes about\n"
  << "  <number>       the requested number of duplicate hashes\n"
  ;
}

static void hash_table() {
  std::cout
  << "hash_table [-j e|o|c|a] <hashdb> <hex file hash>\n"
  << "  Print hashes from the given <hashdb> database that are associated with\n"
  << "  the <source_id> source index.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>              the hash database to print hashes from\n"
  << "  <hex file hash>       the file hash of the source to print hashes for\n"
  ;
}

static void read_bytes() {
  std::cout
  << "read_bytes <media image> <offset> <count>\n"
  << "  Print <count> number of raw bytes starting at the specified <offset> in\n"
  << "  the <media image> file.\n"
  << "\n"
  << "  Parameters:\n"
  << "  <media image>  the media image file to print raw bytes from\n"
  << "  <offset>       the offset in the media image file to read from\n"
  << "  <count>        the number of raw bytes to read\n"
  ;
}

// Performance Analysis
static void add_random() {
  std::cout
  << "add_random <hashdb> <hex file hash> <count>\n"
  << "  Add <count> randomly generated hashes into hash database <hashdb>.\n"
  << "  Write performance data in the database's log.txt file.\n"
  << "\n"
  << "  Options:\n"
  << "  -r, --repository=<repository name>\n"
  << "    The repository name to use for the set of hashes being added.\n"
  << "    (default is \"repository_add_random\").\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to add randomly generated hashes into\n"
  << "  <hex file hash>the file hash of the source to print hashes for\n"
  << "  <count>        the number of randomly generated hashes to add\n"
  ;
}

static void scan_random() {
  std::cout
  << "scan_random <hashdb> <count>\n"
  << "  Scan for random hashes in the <hashdb> database.  Write performance\n"
  << "  data in the database's log.txt file.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to scan\n"
  << "  <count>        the number of randomly generated hashes to scan for\n"
  ;
}

static void add_same() {
  std::cout
  << "add_same <hashdb> <hex file hash> <count>\n"
  << "  Add <count> MD5 hashes of value 0x800000... into hash database <hashdb>.\n"
  << "  Write performance data in the database's log.txt file.\n"
  << "\n"
  << "  Options:\n"
  << "  -r, --repository=<repository name>\n"
  << "    The repository name to use for the set of hashes being added.\n"
  << "    (default is \"repository_add_same\").\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to add hashes of the same value into\n"
  << "  <hex file hash>the file hash of the source to print hashes for\n"
  << "  <count>        the number of hashes of the same value to add\n"
  ;
}

static void scan_same() {
  std::cout
  << "scan_same <hashdb> <count>\n"
  << "  Scan for the same hash value in the <hashdb> database.  Write\n"
  << "  performance data in the database's log.txt file.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to scan\n"
  << "  <count>        the number of randomly generated hashes to scan for\n"
  ;
}

static void test_scan_stream() {
  std::cout
  << "test_scan_stream <hashdb> <count>\n"
  << "  Run <count> scan_stream requests, where each request contains 10K MD5\n"
  << "  hashes of value 0x800000....  Write performance data in the database's\n"
  << "  log.txt file.\n"
  << "\n"
  << "  Options:\n"
  << "  -j, --json_scan_mode\n"
  << "    The JSON scan mode selects optimization and output (default is o):\n"
  << "      e return expanded output.\n"
  << "      o return expanded output optimized to not repeat hash and source\n"
  << "        information.\n"
  << "      c return hash duplicates count\n"
  << "      a return approximate hash duplicates count\n"
  << "\n"
  << "  Parameters:\n"
  << "  <hashdb>       the hash database to scan\n"
  << "  <count>        the number of scan requests to issue\n"
  ;
}

static void all() {
  overview();

  // New Database
  std::cout << "\nNew Database:\n";
  create();

  // Import/Export
  std::cout << "\nImport/Export:\n";
  ingest();
  import_tab();
  import();
  export_json();

  // Database Manipulation
  std::cout << "\nDatabase Manipulation:\n";
  add();
  add_multiple();
  add_repository();
  add_range();
  intersect();
  intersect_hash();
  subtract();
  subtract_hash();
  subtract_repository();

  // Scan
  std::cout << "\nScan:\n";
  scan_list();
  scan_hash();
  scan_image();

  // Statistics
  std::cout << "\nStatistics:\n";
  size();
  sources();
  histogram();
  duplicates();
  hash_table();
  read_bytes();

  // Performance Analysis
  std::cout << "\nPerformance Analysis:\n";
  add_random();
  scan_random();
  add_same();
  scan_same();
  test_scan_stream();
}

void usage(const std::string& command) {

  // all topics
  if (command == "all") all();

  // New Database
  else if (command == "create") create();

  // Import/Export
  else if (command == "ingest") ingest();
  else if (command == "import_tab") import_tab();
  else if (command == "import") import();
  else if (command == "export") export_json();

  // Database Manipulation
  else if (command == "add") add();
  else if (command == "add_multiple") add_multiple();
  else if (command == "add_repository") add_repository();
  else if (command == "add_range") add_range();
  else if (command == "intersect") intersect();
  else if (command == "intersect_hash") intersect_hash();
  else if (command == "subtract") subtract();
  else if (command == "subtract_hash") subtract_hash();
  else if (command == "subtract_repository") subtract_repository();

  // Scan
  else if (command == "scan_list") scan_list();
  else if (command == "scan_hash") scan_hash();
  else if (command == "scan_image") scan_image();

  // Statistics
  else if (command == "size") size();
  else if (command == "sources") sources();
  else if (command == "histogram") histogram();
  else if (command == "duplicates") duplicates();
  else if (command == "hash_table") hash_table();
  else if (command == "read_bytes") read_bytes();

  // Performance Analysis
  else if (command == "add_random") add_random();
  else if (command == "scan_random") scan_random();
  else if (command == "add_same") add_same();
  else if (command == "scan_same") scan_same();
  else if (command == "test_scan_stream") test_scan_stream();

  // fail
  else {
    std::cerr << "Error: unsupported command '" << command << "'.\n";
  }
}

} // end namespace usage

#endif

