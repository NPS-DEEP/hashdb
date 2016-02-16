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

void usage(const std::string& repository_name,
           const std::string& whitelist_dir) {

  const hashdb::settings_t settings;

  // print usage
  std::cout
  << "hashdb Version " << PACKAGE_VERSION  << "\n"
  << "Usage: hashdb [-h|--help] [-v|-V|--version] [-q|--quiet]\n"
  << "              [options] <command> [<args>]\n"
  << "\n"
  << "  -h, --help         print this message\n"
  << "  -v, -V, --version, --Version    print version number\n"
  << "  -q, --quiet        quiet mode\n"
  << "\n"
  << "hashdb supports the following commands:\n"
  << "\n"
  << "New database:\n"
  << "  create [options] <hashdb>\n"
  << "    Create a new <hashdb> hash database.\n"
  << "\n"
  << "    Options:\n"
  << "    -b, --block_size=<block size>\n"
  << "      <block size>, in bytes, or use 0 for no restriction\n"
  << "      (default " << settings.block_size << ")\n"
  << "    -s, --sector_size=<n>\n"
  << "      <sector_size>, in bytes, or 1 for any alignment (default " << settings.sector_size << ")\n"
  << "    -m, --max_id_offset_pairs=<max>\n"
  << "      The maximum number of source ID, source file offset references to\n"
  << "      store for a hash (default " << settings.max_id_offset_pairs << ")\n"
  << "    -t, --tuning=<hash prefix bits:hash suffix bytes>\n"
  << "      The number of hash prefix bits and suffix bytes to use for\n"
  << "      optimizing hash storage (default " << settings.hash_prefix_bits << ":"
  <<        settings.hash_suffix_bytes << ")\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>   the file path to the new hash database to create\n"
  << "\n"
  << "Import/Export:\n"
  << "  import [-r <repository name>] [-w <whitelist.hdb>] <hashdb.hdb>\n"
  << "         <import directory>\n"
  << "    Import hashes recursively from <import directory> into hash database\n"
  << "      <hashdb>.\n"
  << "\n"
  << "    Options:\n"
  << "    -r, --repository_name=<repository name>\n"
  << "      The repository name to use for the set of hashes being imported.\n"
  << "      (default is \"repository_\" followed by the <import directory> path).\n"
  << "    -w, --whitelist_dir\n"
  << "      The path to a whitelist hash database.  Hashes matching this database\n"
  << "      will be marked with a whitelist entropy flag.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <import dir>   the directory to recursively import from\n"
  << "    <hashdb>       the hash database to insert the imported hashes into\n"
  << "\n"
  << "  import_tab [-r <repository name>] [-s <sector size>] <hashdb> <tab file>\n"
  << "    Import hashes from file <tab file> into hash database <hashdb>.\n"
  << "\n"
  << "    Options:\n"
  << "    -r, --repository_name=<repository name>\n"
  << "      The repository name to use for the set of hashes being imported.\n"
  << "      (default is \"repository_\" followed by the <import directory> path).\n"
  << "    -w, --whitelist_dir\n"
  << "      The path to a whitelist hash database.  Hashes matching this database\n"
  << "      will not be imported.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to insert the imported hashes into\n"
  << "    <NIST file>    the NIST file to import hashes from\n"
  << "\n"
  << "  import_json <hashdb> <json file>\n"
  << "    Import hashes from file <json file> into hash database <hashdb>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to insert the imported hashes into\n"
  << "    <json file>    the JSON file to import hashes from\n"
  << "\n"
  << "  export_json <hashdb> <json file>\n"
  << "    Export hashes from hash database <hashdb> into file <json file>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to export\n"
  << "    <json file>    the JSON file to export the hash database into.\n"
  << "\n"
  << "Database manipulation:\n"
  << "  add <source hashdb> <destination hashdb>\n"
  << "    Copy hashes from the <source hashdb> to the <destination hashdb>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb>       the source hash database to copy hashes from\n"
  << "    <destination hashdb>  the destination hash database to copy hashes into\n"
  << "\n"
  << "  add_multiple <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "    Perform a union add of <source hashdb 1> and <source hashdb 2>\n"
  << "    into the <destination hashdb>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb 1>     a hash database to copy hashes from\n"
  << "    <source hashdb 2>     a second hash database to copy hashes from\n"
  << "    <destination hashdb>  the destination hash database to copy hashes into\n"
  << "\n"
  << "  add_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "    Copy hashes from the <source hashdb> to the <destination hashdb>\n"
  << "    when the <repository name> matches.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb>       the source hash database to copy hashes from\n"
  << "    <destination hashdb>  the destination hash database to copy hashes into\n"
  << "    <repository name>     the repository name to match when adding hashes\n"
  << "\n"
  << "  intersect <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "    Copy hashes that are common to both <source hashdb 1> and\n"
  << "    <source hashdb 2> into <destination hashdb>.  Hashes and their sources\n"
  << "    must match.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb 1>     a hash databases to copy the intersection of\n"
  << "    <source hashdb 2>     a second hash databases to copy the intersection of\n"
  << "    <destination hashdb>  the destination hash database to copy the\n"
  << "                          intersection of exact matches into\n"
  << "\n"
  << "  intersect_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "    Copy hashes that are common to both <source hashdb 1> and\n"
  << "    <source hashdb 2> into <destination hashdb>.  Hashes match when hash\n"
  << "    values match, even if their associated source repository name and\n"
  << "    filename do not match.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb 1>     a hash databases to copy the intersection of\n"
  << "    <source hashdb 2>     a second hash databases to copy the intersection of\n"
  << "    <destination hashdb>  the destination hash database to copy the\n"
  << "                          intersection of hashes into\n"
  << "\n"
  << "  subtract <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "    Copy hashes that are in <souce hashdb 1> and not in <source hashdb 2>\n"
  << "    into <destination hashdb>.  Hashes and their sources must match.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb 1>     the hash database containing hash values to be\n"
  << "                          added if they are not also in the other database\n"
  << "    <source hashdb 2>     the hash database containing the hash values that\n"
  << "                          will not be added\n"
  << "    <destination hashdb>  the hash database to add the difference of the\n"
  << "                          exact matches into\n"
  << "\n"
  << "  subtract_hash <source hashdb 1> <source hashdb 2> <destination hashdb>\n"
  << "    Copy hashes that are in <souce hashdb 1> and not in <source hashdb 2>\n"
  << "    into <destination hashdb>.  Hashes match when hash values match, even if\n"
  << "    their associated source repository name and filename do not match.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb 1>     the hash database containing hash values to be\n"
  << "                          added if they are not also in the other database\n"
  << "    <source hashdb 2>     the hash database containing the hash values that\n"
  << "                          will not be added\n"
  << "    <destination hashdb>  the hash database to add the difference of the\n"
  << "                          hashes into\n"
  << "\n"
  << "  subtract_repository <source hashdb> <destination hashdb> <repository name>\n"
  << "    Copy hashes from the <source hashdb> to the <destination hashdb>\n"
  << "    when the <repository name> does not match.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb>       the source hash database to copy hashes from\n"
  << "    <destination hashdb>  the destination hash database to copy hashes into\n"
  << "    <repository name>     the repository name to exclude when adding hashes\n"
  << "\n"
  << "  deduplicate <source hashdb> <destination hashdb>\n"
  << "    Copy the hashes that appear only once \n"
  << "    from  <source hashdb> to <destination hashdb>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <source hashdb>       the hash database to copy hashes from when source\n"
  << "                          hashes appear only once\n"
  << "    <destination hashdb>  the hash database to copy hashes to when source\n"
  << "                          hashes appear only once\n"
  << "\n"
  << "Scan:\n"
  << "  scan_hash <hashdb> <hex block hash>\n"
  << "    Scan hash database <hashdb> for the specified <hash value> and print\n"
  << "    out matches.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>          the file path to the hash database to use as the\n"
  << "                      lookup source\n"
  << "    <hex block hash>  the hash value to scan for\n"
  << "\n"
  << "Statistics:\n"
  << "  sizes <hashdb>\n"
  << "    Print sizes of database tables inside the given <hashdb> database.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to print size information for\n"
  << "\n"
  << "  sources <hashdb>\n"
  << "    Print source information indicating where the hashes in the <hashdb>\n"
  << "    came from.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to print all the repository name,\n"
  << "                   filename source information for\n"
  << "\n"
  << "  histogram <hashdb>\n"
  << "    Print the histogram of hashes for the given <hashdb> database.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to print the histogram of hashes for\n"
  << "\n"
  << "  duplicates <hashdb> <number>\n"
  << "    Print the hashes in the given <hashdb> database that are sourced the\n"
  << "    given <number> of times.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to print duplicate hashes about\n"
  << "    <number>       the requested number of duplicate hashes\n"
  << "\n"
  << "  hash_table <hashdb> <hex file hash>\n"
  << "    Print hashes from the given <hashdb> database that are associated with\n"
  << "    the <source_id> source index.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>              the hash database to print hashes from\n"
  << "    <hex file hash>       the file hash of the source to print hashes for\n"
  << "\n"
  << "Performance analysis:\n"
  << "  add_random <hashdb> <hex file hash> <count>\n"
  << "    Add <count> randomly generated hashes into hash database <hashdb>.\n"
  << "    Write performance data in the database's log.txt file.\n"
  << "\n"
  << "    Options:\n"
  << "    -r, --repository=<repository name>\n"
  << "      The repository name to use for the set of hashes being added.\n"
  << "      (default is \"repository_add_random\").\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to add randomly generated hashes into\n"
  << "    <hex file hash>the file hash of the source to print hashes for\n"
  << "    <count>        the number of randomly generated hashes to add\n"
  << "\n"
  << "  scan_random <hashdb> <count>\n"
  << "    Scan for random hashes in the <hashdb> database.  Writes performance\n"
  << "    in the database's log.txt file.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>       the hash database to scan\n"
  << "    <count>        the number of randomly generated hashes to scan for\n"
  << "\n"
  << "bulk_extractor hashdb scanner:\n"
  << "  bulk_extractor -e hashdb -S hashdb_mode=import -o outdir1 my_image1\n"
  << "    Imports hashes from my_image1 to outdir1/hashdb.hdb\n"
  << "\n"
  << "  bulk_extractor -e hashdb -S hashdb_mode=scan\n"
  << "                 -S hashdb_scan_path_or_socket=outdir1/hashdb.hdb\n"
  << "                 -o outdir2 my_image2\n"
  << "    Scans hashes from my_image2 against hashes in outdir1/hashdb.hdb\n"
  << "\n"
  ;
}

#endif

