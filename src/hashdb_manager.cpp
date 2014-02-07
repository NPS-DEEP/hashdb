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
#include "hashdb_types.h"
#include "hashdb_db_manager.hpp"
#include "commands.hpp"
#include "command_line.hpp"
#include "hashdb_settings_reader.hpp"
#include "hashdb_settings_writer.hpp"

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

uint64_t approximate_M_to_n(uint32_t M);
uint32_t approximate_n_to_M(uint64_t n);

static const std::string see_usage = "Please type 'hashdb_manager -h' for usage.";

// user commands, which are not the demultiplexed commands seen in commands.hpp
static const std::string COMMAND_COPY = "copy";
static const std::string COMMAND_REMOVE = "remove";
static const std::string COMMAND_MERGE = "merge";
static const std::string COMMAND_REBUILD_BLOOM = "rebuild_bloom";
static const std::string COMMAND_EXPORT = "export";
static const std::string COMMAND_INFO = "info";
static const std::string COMMAND_SERVER = "server";

// options
static std::string repository_name = "";
static std::string server_socket_endpoint = "tcp://*:14500";
static settings_t hashdb_settings;
static size_t exclude_duplicates_count = 0;

// option state used to assure options are not specified when they should not be
bool has_tuning = false;
bool has_tuning_bloom = false;
bool has_repository_name = false;
bool has_server_socket_endpoint = false;
bool has_exclude_duplicates = false;

// bloom filter validation
bool has_b1n = false;
bool has_b1kM = false;
bool has_b2n = false;
bool has_b2kM = false;

void usage() {
  // default settings
  const settings_t s;

  // print usage
  std::cout
  << "hashdb_manager Version " << PACKAGE_VERSION  << "\n"
  << "Usage: hashdb_manager -h | -H | -V | <command>\n"
  << "    -h, --help    print this message\n"
  << "    -H            print detailed help including usage notes and examples\n"
  << "    --Version     print version number\n"
  << "\n"
  << "hashdb_manager supports the following <command> options:\n"
  << "\n"
  << "copy [<hashdb tuning parameter>]+ [-r <repository name>] <input> <hashdb>\n"
  << "    Copies the hashes in the <input> into the <hashdb> hash database.\n"
  << "\n"
  << "    Options:\n"
  << "    <hashdb tuning parameter>\n"
  << "        When a new <hashdb> hash database is being created,\n"
  << "        <hashdb tuning parameter> options may be provided to configure the\n"
  << "        hash database.  Please see <hashdb tuning parameter> options and\n"
  << "        <bloom filter tuning parameter> options for settings and default\n"
  << "        values.\n"
  << "\n"
  << "    -r, --repository=<repository name>\n"
  << "        When importing hashes from a md5deep generated DFXML <input> file,\n"
  << "        where a repository name is not specified, a <repository name> may\n"
  << "        be provided to speify the repository from which cryptographic hashes\n"
  << "        of hash blocks are sourced.  (default is \"repository_\" followed\n"
  << "        by the <DFXML file> path).\n"
  << "\n"
  << "    -x, --exclude_duplicates=<count>\n"
  << "        When copying hashes from an <input> hashdb hash dtatabase to a new\n"
  << "        <hashdb> hash database, do not copy any hashes that have <count>\n"
  << "        or more duplicates.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <input>    a md5deep generated DFXML file or another hashdb hash database\n"
  << "    <hashdb>   a hash database being created or a hash database being\n"
  << "               copied to\n"
  << "\n"
  << "remove [-r <repository name>] <input> <hashdb>\n"
  << "    Removes hashes in the <input> from the <hashdb> hash database.\n"
 << "\n"
  << "    Options:\n"
  << "    -r, --repository=<repository name>\n"
  << "        When removing hashes identified from a md5deep generated DFXML\n"
  << "        <input> file, where a repository name is not specified, a\n"
  << "        <repository name> may be provided to speify the repository from\n"
  << "        which cryptographic hashes of hash blocks will be removed.\n"
  << "        (default is \"repository_\"\n followed by the <DFXML file> path)\n"
  << "\n"
  << "    Parameters:\n"
  << "    <input>    a md5deep generated DFXML file or another hashdb hash database\n"
  << "    <hashdb>   a hash database in which hashes in the <input> will be\n"
  << "               removed\n"
  << "\n"
  << "merge [<hashdb tuning parameter>]+ <hashdb input 1> <hashdb input 2>\n"
  << "        <hashdb output>\n"
  << "    Merges hashes in the <hashdb input 1> and <hashdb input 2> databases\n"
  << "    into the new <hashdb output> database.\n"
  << "\n"
  << "    Options:\n"
  << "    <hashdb tuning parameter>\n"
  << "        When a new <hashdb> hash database is being created,\n"
  << "        <hashdb tuning parameter> options may be provided to configure the\n"
  << "        hash database.  Please see <hashdb tuning parameter> options and\n"
  << "        <bloom filter tuning parameter> options for settings and default\n"
  << "        values.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb input 1>    a hashdb hash database input\n"
  << "    <hashdb input 2>    a second hashdb hash database input\n"
  << "    <hashdb output>     a new hashdb hash database that will contain the\n"
  << "                        merged inputs\n"
  << "\n"
  << "rebuild_bloom [<bloom filter tuning parameter>]+ <hashdb>\n"
  << "    Rebuilds the bloom filters in the <hashdb> hash database.\n"
  << "\n"
  << "    Options:\n"
  << "    <bloom filter tuning parameter>\n"
  << "        Please see <bloom filter tuning parameter> options for settings\n"
  << "        and default values.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>    a hash database for which the bloom filters will be rebuilt\n"
  << "\n"
  << "export <hashdb> <DFXML file>\n"
  << "    Exports the hashes in the <hashdb> hash database to a new <DFXML file>.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb input>   a hash database whose hash values are to be exported\n"
  << "    <dfxml output>   a DFXML file containing the hashes in the <hashdb input>\n"
  << "\n"
  << "info <hashdb>\n"
  << "    Displays information about the <hashdb> hash database to stdout.\n"
  << "\n"
  << "    Parameters:\n"
  << "    <hashdb>         a hash database whose database information is to be\n"
  << "                     displayed\n"
  << "\n"
  << "server [-s] <server socket endpoint> <hashdb>\n"
  << "    Starts hashdb_manager as a query server service for supporting hashdb\n"
  << "    queries.\n"
  << "\n"
  << "    Options:\n"
  << "    -s, --socket=<server socket endpoint>\n"
  << "        specifies the <server socket endpoint> to make available for clients.\n"
  << "        Valid socket transports supported by the zmq messaging kernel are\n"
  << "        tcp, ipc, and inproc.  Currently, only tcp is tested.\n"
  << "        (default '" << server_socket_endpoint << "')\n"
  << "\n"
  << "<hashdb tuning parameter> options set the configuration of a new hash\n"
  << "database:\n"
  << "    -p, --hash_block_size=<hash block size>\n"
  << "        <hash block size>, in bytes, used to generate hashes (default " << s.hash_block_size << ")\n"
  << "\n"
  << "    -m, --max_duplicates=<maximum>\n"
  << "        <maximum> number of hash duplicates allowed, or 0 for no limit\n"
  << "        (default " << s.maximum_hash_duplicates << ")\n"
  << "\n"
  << "    -t, --storage_type=<storage type>\n"
  << "        <storage type> to use in the hash database, where <storage type>\n"
  << "        is one of: btree | hash | red-black-tree | sorted-vector\n"
  << "        (default " << s.map_type << ")\n"
  << "\n"
  << "    -n, --shards=<number of shards>\n"
  << "        <number of shards> to use (default " << s.map_shard_count << ")\n"
  << "\n"
  << "    -i, --bits=<number of index bits>\n"
  << "        <number of index bits> to use for the source lookup index, between\n"
  << "        32 and 40 (default " << (uint32_t)s.number_of_index_bits << ")\n"
  << "        The number of bits used for the hash block offset value is\n"
  << "        (64 - <number of index bits>).\n"
  << "\n"
  << "<bloom filter tuning parameter> settings can help performance during hash\n"
  << "queries:\n"
  << "    --b1 <state>\n"
  << "        sets bloom filter 1 <state> to enabled | disabled (default " << bloom_state_to_string(s.bloom1_is_used) << ")\n"
  << "    --b1n <n>\n"
  << "        expected total number <n> of unique hashes (default " << approximate_M_to_n(s.bloom1_M_hash_size) << ")\n"
  << "    --b1kM <k:M>\n"
  << "        number of hash functions <k> and bits per hash <M> (default <k>=" << s.bloom1_k_hash_functions << "\n"
  << "        and <M>=" << s.bloom1_M_hash_size << " or <M>=value calculated from value in --b1n)\n"
  << "    --b2 <state>\n"
  << "        sets bloom filter 1 <state> to enabled | disabled (default " << bloom_state_to_string(s.bloom2_is_used) << ")\n"
  << "    --b2n <total>\n"
  << "        expected total number <n> of unique hashes (default " << approximate_M_to_n(s.bloom2_M_hash_size) << ")\n"
  << "    --b2kM <k:M>\n"
  << "        number of hash functions <k> and bits per hash <M> (default <k>=" << s.bloom2_k_hash_functions << "\n"
  << "        and <M>=" << s.bloom2_M_hash_size << " or <M>=value calculated from value in --b2n)\n"
  << "\n"
  ;
}

void detailed_usage() {
  // print usage notes and examples
  std::cout
  << "Notes:\n"
  << "Using the md5deep tool to generate hash data:\n"
  << "hashdb_manager imports hashes from DFXML files that contain cryptographic\n"
  << "hashes of hash blocks.  These files can be generated using the md5deep tool\n"
  << "or by exporting a hash database using the hashdb_manager \"export\" command.\n"
  << "When using the md5deep tool to generate hash data, the \"-p <partition size>\"\n"
  << "option must be set to the desired hash block size.  This value must match\n"
  << "the hash block size that hashdb_manager expects or else no hashes will be\n"
  << "copied in.  The md5deep tool also requires the \"-d\" option in order to\n"
  << "instruct md5deep to generate output in DFXML format.\n"
  << "\n"
  << "Selecting an optimal hash database storage type:\n"
  << "The storage type option, \"-t\", selects the storage type to use in the\n"
  << "hash database.  Each storage type has advantages and disadvantages:\n"
  << "    btree           Provides fast build times, fast access times, and is\n"
  << "                    fairly compact.\n"
  << "                    Currently, btree may have threading issues and may\n"
  << "                    crash when performing concurrent queries.\n"
  << "\n"
  << "    hash            Provides fastest query times and is very compact,\n"
  << "                    but is very slow during building.  We recommend\n"
  << "                    building a hash database using the btree storage type,\n"
  << "                    and, once built, copying it to a new hash database\n"
  << "                    using the hash storage type option.\n"
  << "\n"
  << "    red-black-tree  Similar in performance to btree, but not as fast or\n"
  << "                    compact.\n"
  << "\n"
  << "    sorted-vector   Similar in performance to hash.\n"
  << "\n"
  << "Improving query speed by using sharding:\n"
  << "Sharding splits hashes so that internal to the hash database, they are\n"
  << "distributed across multiple files.  The purpose of sharding is to reduce\n"
  << "the size of data structures and files.  It is not clear that sharding helps\n"
  << "performance by reducing the size of data structures.  Sharding does not\n"
  << "help performance by using multiple files because the files must all be\n"
  << "opened anyway.  In the future, when shards can be distributed across multiple\n"
  << "parallel processors, sharding can help performance significantly.\n"
  << "\n"
  << "Improving query speed by using Bloom filters:\n"
  << "Bloom filters can speed up performance during hash queries by quickly\n"
  << "indicating if a hash value is not in the hash database.  When the Bloom\n"
  << "filter indicates that a hash value is not in the hash database, an actual\n"
  << "hash database lookup is not required, and time is saved.  If the Bloom\n"
  << "filter indicates that the hash value may be in the hash database, a hash\n"
  << "database lookup is required and no time is saved.\n"
  << "\n"
  << "Bloom filters can be large and can take up lots of disk space and memory.\n"
  << "A Bloom filter with a false positive rate between 1\% and 10\% is effictive.\n"
  << "If the false-positive rate is low, the Bloom filter is unnecessarily large,\n"
  << "and it could be smaller.  If the false-positive rate is too high, there\n"
  << "will be so many false positives that hash database lookups will be required\n"
  << "anyway, defeating the value of the bloom filter.\n"
  << "\n"
  << "Up to two Bloom filters may be used.  The idea of using two is that the\n"
  << "first would be smaller and would thus be more likely to be fully cached\n"
  << "in memory.  If the first Bloom filter indicates that the hash may be present,\n"
  << "then the second bloom filter, which should be larger, is checked.  If the\n"
  << "second Bloom filter indicates that the hash may be present, then a hash\n"
  << "database lookup is required to be sure.\n"
  << "\n"
  << "Performing hash queries using the hashid scanner with bulk_extractor:\n"
  << "bulk_extractor may be used to scan the hash database for matching\n"
  << "cryptographic hashes if the hashid scanner is configured and enabled.\n"
  << "The hashid scanner runs either as a client with hashdb_manager running as\n"
  << "a server to perform hash queries, or loads the hash database drectly and\n"
  << "performs queries directly.  The hashid scanner takes parameters from\n"
  << "bulk_extractor using bulk_extractor's \"-S name=value\" control parameter.\n"
  << " hashid accepts the following parameters:\n"
  << "\n"
  << "   -S query_type=use_path\n"
  << "      <query_type> used to perform the query, where <query_type>\n"
  << "      is one of use_path | use_socket (default use_path)\n"
  << "      use_path   - Lookups are performed from a hashdb in the filesystem\n"
  << "                   at the specified <path>.\n"
  << "      use_socket - Lookups are performed from a server service at the\n"
  << "                   specified <socket>.\n"
  << "   -S path=a valid hashdb directory path is required\n"
  << "      Specifies the <path> to the hash database to be used for performing\n"
  << "      the query service.  This option is only used when the query type\n"
  << "      is set to \"use_path\".\n"
  << "   -S socket=tcp://localhost:14500\n"
  << "      Specifies the client <socket> endpoint to use to connect with the\n"
  << "      hashdb_manager server (default 'tcp://localhost:14500').  Valid socket\n"
  << "      transports supported by the zmq messaging kernel are tcp, ipc, and\n"
  << "      inproc.  Currently, only tcp is tested.  This opition is only valid\n"
  << "      when the query type is set to \"use_socket\".\n"
  << "   -S hash_block_size=4096    Hash block size, in bytes, used to generate\n"
  << "      cryptographic hashes\n"
  << "   -S sector_size=512    Sector size, in bytes\n"
  << "      Hashes are generated on each sector_size boundary.\n"
  << "\n"
  << "Performing hash queries using the hashdb_checker tool:\n"
  << "The hashdb_checker tool runs as a client service to scan a DFXML file for\n"
  << "cryptographic hash values that match values in a hash database. In order\n"
  << "to work, the hashdb_checker tool requires that the hashdb_manager tool be\n"
  << "running as a server hash database query service at a matching socket\n"
  << "endpoint.  Please type \"hashdb_checker --help\" for more information on\n"
  << "the usage of the hashdb_checker tool.\n"
  << "\n"
  << "Improving startup speed by keeping a hash database open:\n"
  << "In the future, a dedicated provision may be created for this, but for now,\n"
  << "the time required to open a hash database may be avoided by keeping a\n"
  << "persistent hash database open by starting a hash database query server\n"
  << "service and keeping it running.  Now this hash database will open quickly\n"
  << "for other query services because it will already be cached in memory.\n"
  << "Caution, though, do not change the contents of a hash database that is\n"
  << "opened by multiple processes because this will make the copies inconsistent.\n"
  << "\n"
  << "Overloaded uses of the term \"hash\":\n"
  << "The term \"hash\" is overloaded and can mean any of the following:\n"
  << "   The MD5 hash value being recorded in the hash database.\n"
  << "   The hash storage type, specifically an unordered map,  used for storing\n"
  << "   information in the hash database.\n"
  << "   The hash that the hash storage type uses in order to map a MD5 hash\n"
  << "   record onto a hash storage slot.\n"
  << "   The hash that the Bloom filter uses to map onto a specific bit within\n"
  << "   the Bloom filter.\n"
  << "\n"
  << "Log files:\n"
  << "Commands that create or modify a hash database produce a log file in the\n"
  << "hash database directory called \"log.xml\".  Currently, the log file is\n"
  << "replaced each time.  In the future, log entries will append to existing\n"
  << "content.\n"
  << "\n"
  << "Known bugs:\n"
  << "Performing hash queries in a threaded environment using the btree storage\n"
  << "type causes intermittent crashes.  This was observed when running the\n"
  << "bulk_extractor hashid scanner when bulk_extractor was scanning recursive\n"
  << "directories.  This bug will be addressed in a future release of boost\n"
  << "btree.\n"
  << "\n"
  << "Examples:\n"
  << "This example uses the md5deep tool to generate cryptographic hashes from\n"
  << "hash blocks in a file, and is suitable for importing into a hash database\n"
  << "using the hashdb_manager \"copy\" command.  Specifically:\n"
  << "\"-p 4096\" sets the hash block partition size to 4096 bytes.\n"
  << "\"-d\" instructs the md5deep tool to produce output in DFXML format.\n"
  << "\"my_file\" specifies the file that cryptographic hashes will be generated\n"
  << "for.\n"
  << "The output of md5deep is directed to file \"my_dfxml_file\".\n"
  << "    md5deep -p 4096 -d my_file > my_dfxml_file\n"
  << "\n"
  << "This example uses the md5deep tool to generate hashes recursively under\n"
  << "subdirectories, and is suitable for importing into a hash database using\n"
  << "the hashdb_manager \"copy\" command.  Specifically:\n"
  << "\"-p 4096\" sets the hash block partition size to 4096 bytes.\n"
  << "\"-d\" instructs the md5deep tool to produce output in DFXML format.\n"
  << "\"-r mydir\" specifies that hashes will be generated recursively under\n"
  << "directory mydir.\n"
  << "The output of md5deep is directed to file \"my_dfxml_file\".\n"
  << "    md5deep -p 4096 -d -r my_dir > my_dfxml_file\n"
  << "\n"
  << "This example copies hashes from DFXML input file my_dfxml_file to new hash\n"
  << "database my_hashdb, categorizing the hashes as sourced from repository\n"
  << "\"my repository\":\n"
  << "    hashdb_manager copy -r \"my repository\" my_dfxml_file my_hashdb\n"
  << "\n"
  << "This example copies hashes from hash database my_hashdb1 to hash database\n"
  << "my_hashdb2.  If my_hashdb2 does not exist, it will be created.  If\n"
  << "my_hashdb2 exists, hashes from my_hashdb1 will be added to it.\n"
  << "    hashdb_manager copy my_hashdb1 my_hashdb2\n"
  << "\n"
  << "This example copies hashes from my_hashdb1 to new hash database my_hashdb2,\n"
  << "but uses \"-m 5\" to copy only the first five duplicate hashes of each\n"
  << "duplicate hash value:\n"
  << "    hashdb_manager copy -m 5 my_hashdb1 my_hashdb2\n"
  << "\n"
  << "This example copies hashes from my_hashdb1 to new hash database my_hashdb2,\n"
  << "but uses \"-x 5\" to not copy any hashes from my_hashdb1 that have 5 or more\n"
  << "duplicates.\n"
  << "    hashdb_manager copy -x 5 my_hashdb1 my_hashdb2\n"
  << "\n"
  << "This example copies hashes from my_hashdb1 to new hash database my_hashdb2\n"
  << "using various tuning parameters.  Specifically:\n"
  << "\"-p 512\" specifies that the hash database will contain hashes for data\n"
  << "hashed with a hash block size of 512 bytes.\n"
  << "\"-m 2\" specifies that when there are duplicate hashes, only the first\n"
  << "two hashes of a duplicate hash value will be copied.\n"
  << "\"-t hash\" specifies that hashes will be recorded using the \"hash\" storage\n"
  << "type algorithm.\n"
  << "\"-n 4\" specifies that, internal to the hash database, hash values will be\n"
  << "sharded across four files.\n"
  << "\"-i 34\" specifies that 34 bits are allocated for the source lookup index,\n"
  << "allowing 2^34 entries of source lookup data.  Note that this leaves 2^30\n"
  << "entries remaining for hash block offset values.\n"
  << "\"--b1 enabled\" specifies that Bloom filter 1 is enabled.\n"
  << "\"--b1n 50000000\" specifies that Bloom filter 1 should be sized to expect\n"
  << "50,000,000 different hash values.\n"
  << "\"--b2 enabled\" specifies that Bloom filter 2 is enabled.\n"
  << "\"--b2kM 4:32 enabled\" specifies that Bloom filter 2 will be configured to\n"
  << "have 4 hash functions and that the Bloom filter hash function size will be\n"
  << "32 bits, consuming 512MiB of disk space.\n"
  << "    hashdb_manager copy -p 512 -m 2 -t hash -n 4 -i 34 --b1 enabled\n"
  << "                --b1n 50000000 --b2 enabled --b2kM 4:32 my_hashdb1 my_hashdb2\n"
  << "\n"
  << "This example removes hashes in my_dfxml_file from my_hashdb using a DFXML\n"
  << "repository source name of \"my repository\":\n"
  << "    hashdb_manager remove -r \"my repository\" my_dfxml_file my_hashdb\n"
  << "\n"
  << "This example merges my_hashdb1 and my_hashdb2 into new hash database\n"
  << "my_hashdb3:\n"
  << "    hashdb_manager merge my_hashdb1 my_hashdb2 my_hashdb3\n"
  << "\n"
  << "This example rebuilds the Bloom filters for hash database my_hashdb to\n"
  << "optimize it to work well with 50,000,000 different hash values:\n"
  << "    hashdb_manager rebuild_bloom --b1n 50000000 my_hashdb\n"
  << "\n"
  << "This example exports hashes in my_hashdb to new DFXML file my_dfxml:\n"
  << "    hashdb_manager export my_hashdb my_dfxml\n"
  << "\n"
  << "This example displays the history attribution log of hash database my_hashdb.\n"
  << "Output is directed to stdout.\n"
  << "    hashdb_manager info my_hashdb\n"
  << "\n"
  << "This example starts hashdb_manager as a server service using socket endpoint\n"
  << "\"tcp://*:14501\".  It provides hash lookups using hash database my_hashdb:\n"
  << "    hashdb_manager server -s tcp://*:14501 my_hashdb\n"
  << "\n"
  << "This example uses bulk_extractor to run the hashid scanner to scan for\n"
  << "hash values in a media file where the hash queries are performed\n"
  << "locally from a hashdb database that is opened by the hashid scanner.\n"
  << "Parameters to bulk_extractor for this example follow:\n"
  << "\"-S query_type=use_path\" tells the scanner to perform hash queries\n"
  << "using a hashdb at a local file path.\n"
  << "\"-S path=my_hashdb\" tells the scanner to perform hash queries\n"
  << "using local hashdb my_hashdb.\n"
  << "\"-S hash_block_size=4096\" tells the scanner to create cryptographic hashes\n"
  << "on 4096-byte chunks of data.\n"
  << "\"-S sector_size=512\" tells the scanner to create cryptographic hashes at\n"
  << "every 512-byte sector boundary.\n"
  << "\"-o scanner_output\" tells bulk_extractor to put scanner output into the\n"
  << "scanner_output directory.\n"
  << "File \"my_imagefile\" is the name of the image file that the scanner will use.\n"
  << "Specifically, the scanner will create hashes from hash blocks at each\n"
  << "sector boundary.\n"
  << "    bulk_extractor -S query_type=use_path\n"
  << "                   -S path=my_hashdb\n"
  << "                   -S hash_block_size=4096\n"
  << "                   -S sector_size=512\n"
  << "                   -o scanner_output my_imagefile\n"
  << "\n"
  << "This example uses bulk_extractor to run the scan_hashid scanner to scan\n"
  << "for hash values in a media file where the hash queries are performed\n"
  << "remotely using a hash database query server service available at a socket\n"
  << "endpoint.  Parameters to bulk_extractor for this example follow:\n"
  << "\"-S query_type=use_socket\" tells the scanner to perform hash queries\n"
  << "using a query server at a socket endpoint.\n"
  << "\"-S socket=tcp://localhost:14501\" sets the socket so that queries use a\n"
  << "hashdb query server at socket endpoint \"tcp://localhost:14501\".\n"
  << "hashdb_manager must be running and available at\n"
  << "socket endpoint \"tcp://*:14501\" or else this example will fail because\n"
  << "a server service is not available.  Please see the example for starting\n"
  << "hashdb_manager as a server query service.\n"
  << "\"-S hash_block_size=4096\" tells the scanner to create cryptographic\n"
  << "hashes on 4096-byte chunks of data.\n"
  << "\"-S sector_size=512\" tells the scanner to create cryptographic hashes at\n"
  << "every 512-byte sector boundary.\n"
  << "\"-o scanner_output\" tells bulk_extractor to put scanner output into the\n"
  << "scanner_output directory.\n"
  << "File \"my_imagefile\" is the name of the image file that the scanner will use.\n"
  << "Specifically, the scanner will create hashes from hash blocks at each\n"
  << "sector boundary.\n"
  << "    bulk_extractor -S query_type=use_socket\n"
  << "                   -S socket=tcp://localhost:14501\n"
  << "                   -S hash_block_size=4096\n"
  << "                   -S sector_size=512\n"
  << "                   -o scanner_output my_imagefile\n"
  << "\n"
  << "This example uses the hashdb_checker tool to determine if hash values in\n"
  << "file my_dfxml match hash values in the hashdb that is opened locally for\n"
  << "querying from.\n"
  << "Parameters to the hashdb_checker tool follow:\n"
  << "\"query_hash\" tells hashdb_checker to perform a hash query.\n"
  << "\"-q use_socket\" directs the query to use a hash database query server.\n"
  << "service for performing the hash lookup.\n"
  << "\"-s tcp://localhost:14501\" specifies the client socket endpoint as\n"
  << "\"tcp://localhost:14501\".  hashdb_manager must be running and available\n"
  << "at socket endpoint \"tcp://*:14501\" or else this example will fail\n"
  << "because a server service is not available.  Please see the example for\n"
  << "starting hashdb_manager as a server query service.\n"
  << "File \"my_dfxml\" is the name of the DFXML file containing hashes that will\n"
  << "be scanned for.\n"
  << "Output is directed to stdout.\n"
  << "    hashdb_checker query_hash -q use_socket -s tcp://localhost:14501 my_dfxml\n"
  << "\n"
  << "This example uses the hashdb_checker tool to look up source information\n"
  << "in feature file \"identified_blocks.txt\" created by the hashid scanner\n"
  << "while running bulk_extractor.\n"
  << "Parameters to the hashdb_checker tool follow:\n"
  << "\"query_source\" tells hashdb_checker to perform a source lookup query.\n"
  << "\"-q use_path\" directs the query to perform the queries using a path to\n"
  << "a hashdb resident in the local filesystem.\n"
  << "\"-p my_hashdb\" specifies \"my_hshdb\" as the file path to the hash database.\n"
  << "\"identified_blocks.txt\" is the feature file containing the hash values\n"
  << "to look up source information for.\n"
  << "Output is directed to stdout.\n"
  << "    hashdb_checker query_source -q use_path -p my_hashdb identified_blocks.txt\n"
  << "\n"
  << "This example uses the hashdb_checker tool to display information about\n"
  << "the hashdb being used by a server query service.\n"
  << "Parameters to the hashdb_checker tool follow:\n"
  << "\"query_hashdb_info\" tells hashdb_checker to return information about\n"
  << "the hashdb that it is using.\n"
  << "\"-q use_socket\" directs the query to use a hash database query server.\n"
  << "\"-s tcp://localhost:14501\" specifies the client socket endpoint as\n"
  << "\"tcp://localhost:14501\".  hashdb_manager must be running and available\n"
  << "at socket endpoint \"tcp://*:14501\" or else this example will fail\n"
  << "because a server service is not available.  Please see the example for\n"
  << "starting hashdb_manager as a server query service.\n"
  << "Output is directed to stdout.\n"
  << "    hashdb_checker query_hashdb_info -q use_socket -s tcp://localhost:14501\n"
  << "\n"
  ;
}

// ************************************************************
// helpers
// ************************************************************
// if file exists, delete it or fail
static void delete_file(const std::string& file) {
  bool is_present = (access(file.c_str(),F_OK) == 0);
  if (is_present) {
    int success = remove(file.c_str());
    if (success != 0) {
      std::cerr << "Error:\nUnable to delete file '"
                << file << "'.\n"
                << strerror(errno)
                << "\nCannot continue.\n";
      exit(1);
    }
  }
}

// create the new hashdb or fail
static void create_hashdb(const std::string& hashdb_dir,
                          const settings_t& hashdb_tuning_settings) {
  // make sure the new hashdb directory does not exist
  bool is_present = (access(hashdb_dir.c_str(),F_OK) == 0);
  if (is_present) {
    std::cerr << " Error: new hashdb directory '" << hashdb_dir
              << "' already exists.\nCannot continue.\n";
    exit(1);
  }

  // create the new hashdb directory
#ifdef WIN32
  if(mkdir(hashdb_dir.c_str())){
    std::cerr << "Error: Could not make new hashdb directory '" << hashdb_dir
              << "'.\nCannot continue.\n";
    exit(1);
  }
#else
  if(mkdir(hashdb_dir.c_str(),0777)){
    std::cerr << "Error: Could not make new hashdb directory '" << hashdb_dir
              << "'.\nCannot continue.\n";
    exit(1);
  }
#endif

  // write the tuning settings to the new settings file
  hashdb_settings_writer::write_settings(hashdb_dir, hashdb_tuning_settings);
}

// determine that a path is to a hashdb
static bool is_hashdb(std::string path) {
  // deemed is_hashdb if the settings file is present under the path
  std::string settings_filename = hashdb_filenames_t::settings_filename(path);
  bool is_present = (access(settings_filename.c_str(),F_OK) == 0);
  return is_present;
}

// determine that a path is to a dfxml file
// NOTE: this should check xml to decide.
static bool is_dfxml(std::string path) {
  // deemed is_dfxml if file exists and is not hashdb
  bool is_present = (access(path.c_str(),F_OK) == 0);
  if (!is_present) {
    return false;
  }
  if (is_hashdb(path)) {
    return false;
  }
  // improvable logic: present and not hashdb therefore is dfxml
  return true;
}

// determine if something is at the path
static bool is_present(std::string path) {
  bool is_present = (access(path.c_str(),F_OK) == 0);
  return is_present;
}

// change existing bloom settings
static void reset_bloom_filters(const std::string& hashdb_dir,
                                const settings_t& new_hashdb_settings) {

  // require hashdb_dir
  if (!is_hashdb(hashdb_dir)) {
    std::cerr << "Error:\nFile '"
              << hashdb_dir << "' does not exist.\n"
              << "The hash database does not exist.\n"
              << "Cannot continue.\n";
    exit(1);
  }
 
  // get existing hashdb tuning settings
  settings_t existing_hashdb_settings;
  hashdb_settings_reader_t::read_settings(hashdb_dir+"settings.xml", existing_hashdb_settings);

  // change the bloom filter settings
  existing_hashdb_settings.bloom1_is_used = new_hashdb_settings.bloom1_is_used;
  existing_hashdb_settings.bloom1_k_hash_functions = new_hashdb_settings.bloom1_k_hash_functions;
  existing_hashdb_settings.bloom1_M_hash_size = new_hashdb_settings.bloom1_M_hash_size;
  existing_hashdb_settings.bloom2_is_used = new_hashdb_settings.bloom2_is_used;
  existing_hashdb_settings.bloom2_k_hash_functions = new_hashdb_settings.bloom2_k_hash_functions;
  existing_hashdb_settings.bloom2_M_hash_size = new_hashdb_settings.bloom2_M_hash_size;

  // write back the changed settings
  hashdb_settings_writer::write_settings(hashdb_dir, existing_hashdb_settings);

  // calculate the bloom filter filenames
  std::string bloom1_path = hashdb_filenames_t::bloom1_filename(hashdb_dir);
  std::string bloom2_path = hashdb_filenames_t::bloom2_filename(hashdb_dir);

  // delete the existing bloom filter files
  delete_file(bloom1_path);
  delete_file(bloom2_path);
}

// 0.17 ~= -1/(math.log(0.06)/(math.log(2)*math.log(2)))
// M = \lg(-\frac{n \ln(p)}{\ln(2)^2})
//
// Where p is held at 6% (0.06)
//
// In this code we first find a notional size in bits of the bloom filter needed
// to get a false positive rate 0f 6%.  We then take the ceiling of the log base 2 of that
// size.  This gives us the number of bits needed to express the bloom filter while
// guaranteeing that the false positive rate is at or lower than 6%.  For example, at
// n = 1billion, this results in M = 33 bits giving a false positive rate of about 1.7%.
// If we had chosen M = 32 bits our false positive rate would have been almost 13%.
//
// approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
uint64_t approximate_M_to_n(uint32_t M) {
  uint64_t m = (uint64_t)1<<M;
  uint64_t n = m * 0.17;
//std::cout << "Bloom filter conversion: for M=" << M << " use n=" << n << "\n";
  return n;
}

// approximate bloom conversions for k=3 and p false positive = ~ 1.1% to 6.4%
uint32_t approximate_n_to_M(uint64_t n) {
  uint64_t m = n / 0.17;
  uint32_t M = 1;
  // fix with actual math formula, but this works
  while ((m = m/2) > 0) {
    M++;
  }
//std::cout << "Bloom filter conversion: for n=" << n << " use M=" << M << "\n";
  return M;
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
  if (has_server_socket_endpoint) {
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
void require_hash_block_sizes_match(const std::string& hashdb_dir1, const std::string& hashdb_dir2,
                               const std::string& action) {
  settings_t settings1;
  hashdb_settings_reader_t::read_settings(hashdb_dir1+"settings.xml", settings1);
  settings_t settings2;
  hashdb_settings_reader_t::read_settings(hashdb_dir2+"settings.xml", settings2);

  if (settings1.hash_block_size != settings2.hash_block_size) {
    std::cerr << "Error: The hash block size for the databases do not match.\n";
    std::cerr << "The hash block size for " << hashdb_dir1 << " is " << settings1.hash_block_size << "\n";
    std::cerr << "but the hash block size for " << hashdb_dir2 << " is " << settings2.hash_block_size << ".\n";
    std::cerr << "Aborting command to " << action << ".\n";
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
        repository_name = optarg;
        break;
      }
      case 's': {	// server socket endpoint
        has_server_socket_endpoint = true;
        server_socket_endpoint = optarg;
        break;
      }
      case 'x': {	// remove duplicates during copy
        has_exclude_duplicates = true;
        try {
          exclude_duplicates_count = boost::lexical_cast<size_t>(optarg);
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
  const int num_args = argc;
  const std::string arg1((argc>=1) ? argv[0] : "");
  const std::string arg2((argc>=2) ? argv[1] : "");
  const std::string arg3((argc>=3) ? argv[2] : "");

  // generate usable repository name if one is not provided
  // this works globally because all commands that use repository_name
  // uniformly require arg1
  if (repository_name == "") {
    repository_name = "repository_" + arg1;
  }

  // ************************************************************
  // run the command
  // ************************************************************
  // private
  const std::string ACTION_COPY_DFXML_NEW("copy DFXML hashes to new hashdb");
  const std::string ACTION_COPY_DFXML_EXISTING("copy DFXML hashes to existing hashdb");
  const std::string ACTION_COPY_NEW("copy hashdb to new hashdb");
  const std::string ACTION_COPY_NEW_EXCLUDE_DUPLICATES("copy hashdb to new hashdb excluding duplicates");
  const std::string ACTION_COPY_EXISTING("copy hashdb to existing hashdb");
  const std::string ACTION_REMOVE_DFXML("remove DFXML hashes from hashdb");
  const std::string ACTION_REMOVE("remove hashdb from hashdb");
  const std::string ACTION_MERGE("merge hashdb1 and hashdb2 to new hashdb3");
  const std::string ACTION_REBUILD_BLOOM("rebuild bloom for hashdb");
  const std::string ACTION_EXPORT("export hashdb to new DFXML");
  const std::string ACTION_INFO("report info about hashdb to stdout");
  const std::string ACTION_SERVER("start server using hashdb");

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
      commands_t::do_copy_new_dfxml(arg1, repository_name, arg2);

    // copy dfxml modifying existing hashdb
    } else if (is_dfxml(arg1) && is_hashdb(arg2)) {
      no_has_tuning(ACTION_COPY_DFXML_EXISTING);
      no_has_tuning_bloom(ACTION_COPY_DFXML_EXISTING);
      no_has_server_socket_endpoint(ACTION_COPY_DFXML_EXISTING);
      no_has_exclude_duplicates(ACTION_COPY_DFXML_EXISTING);

      commands_t::do_copy_dfxml(arg1, repository_name, arg2);

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
      commands_t::do_copy_new_exclude_duplicates(arg1, arg2, exclude_duplicates_count);

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

      commands_t::do_remove_dfxml(arg1, repository_name, arg2);

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

    commands_t::do_server(arg1, server_socket_endpoint);

  } else {
    std::cerr << "Error: '" << command << "' is not a recognized command.  " << see_usage << "\n";
  }

  // done
  return 0;
}

