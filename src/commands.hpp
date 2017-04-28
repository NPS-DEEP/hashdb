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
 * Provides hashdb commands.
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include "../src_libhashdb/hashdb.hpp"
#include "import_tab.hpp"
#include "import_json.hpp"
#include "export_json.hpp"
#include "scan_list.hpp"
#include "adder.hpp"
#include "adder_set.hpp"

// Standard includes
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

// leave alone else create using existing settings if new
void create_if_new(const std::string& hashdb_dir,
                   const std::string& from_hashdb_dir,
                   const std::string& command_string) {

  std::string error_message;
  hashdb::settings_t settings;

  // try to read hashdb_dir settings
  error_message = hashdb::read_settings(hashdb_dir, settings);
  if (error_message.size() == 0) {
    // hashdb_dir already exists
    return;
  }

  // no hashdb_dir, so read from_hashdb_dir settings
  error_message = hashdb::read_settings(from_hashdb_dir, settings);
  if (error_message.size() != 0) {
    // bad since from_hashdb_dir is not valid
    std::cerr << "Error: " << error_message << "\n";
    exit(1);
  }

  // create hashdb_dir using from_hashdb_dir settings
  error_message = hashdb::create_hashdb(hashdb_dir, settings, command_string);
  if (error_message.size() != 0) {
    // bad since from_hashdb_dir is not valid
    std::cerr << "Error: " << error_message << "\n";
    exit(1);
  }
}

// require hashdb_dir else fail
static void require_hashdb_dir(const std::string& hashdb_dir) {
  std::string error_message;
  hashdb::settings_t settings;
  error_message = hashdb::read_settings(hashdb_dir, settings);
  if (error_message.size() != 0) {
    std::cerr << "Error: " << error_message << "\n";
    exit(1);
  }
}

static void print_header(const std::string& cmd) {
  std::cout << "# command: " << cmd << "\n"
            << "# hashdb-Version: " << PACKAGE_VERSION << "\n";
}

// helper
/**
 * Return 16 bytes of random hash.
 */
std::string random_binary_hash() {
  char hash[16];
  for (size_t i=0; i<16; i++) {
    // note: uint32_t not used because windows rand only uses 15 bits.
    hash[i]=(static_cast<char>(rand()));
  }
  return std::string(hash, 16);
}

namespace commands {

// ************************************************************
// helpers
// ************************************************************
class in_ptr_t {
  private:
  std::istream* in;

  // do not allow copy or assignment
  in_ptr_t(const in_ptr_t&);
  in_ptr_t& operator=(const in_ptr_t&);

  public:
  in_ptr_t(const std::string& in_filename) : in(NULL) {
    if (in_filename == "-") {
      in = &std::cin;
    } else {
      std::ifstream* inf = new std::ifstream(in_filename.c_str());
      if (!inf->is_open()) {
        std::cerr << "Error: Cannot open " << in_filename
                  << ": " << strerror(errno) << "\n";
        exit(1);
      }
      in = inf;
    }
  }

  ~in_ptr_t() {
    if (in != &std::cin) {
      delete in;
    }
  }

  std::istream* operator()() {
    return in;
  }
};

class out_ptr_t {
  private:
  std::ostream* out;

  // do not allow copy or assignment
  out_ptr_t(const out_ptr_t&);
  out_ptr_t& operator=(const out_ptr_t&);

  public:
  out_ptr_t(const std::string& out_filename) : out(NULL) {
    if (out_filename == "-") {
      out = &std::cout;
    } else {
      std::ofstream* outf = new std::ofstream(out_filename.c_str());
      if (!outf->is_open()) {
        std::cerr << "Error: Cannot open " << out_filename
                  << ": " << strerror(errno) << "\n";
        exit(1);
      }
      out = outf;
    }
  }

  ~out_ptr_t() {
    if (out != &std::cout) {
      delete out;
    }
  }

  std::ostream* operator()() {
    return out;
  }
};

  // ************************************************************
  // new database
  // ************************************************************
  void create(const std::string& hashdb_dir,
              const hashdb::settings_t& settings,
              const std::string& cmd) {

    std::string error_message;
    error_message = hashdb::create_hashdb(hashdb_dir, settings, cmd);

  if (error_message.size() == 0) {
      std::cout << "New database created.\n";
    } else {
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }
  }

  // ************************************************************
  // import/export
  // ************************************************************
  // import recursively from path
  static void ingest(const std::string& hashdb_dir,
                     const std::string& ingest_path,
                     const size_t step_size,
                     const std::string& repository_name,
                     const std::string& whitelist_dir,
                     const bool disable_recursive_processing,
                     const bool disable_calculate_entropy,
                     const bool disable_calculate_labels,
                     const std::string& cmd) {

    // ingest
    std::string error_message = hashdb::ingest(
                    hashdb_dir, ingest_path, step_size, repository_name,
                    whitelist_dir,
                    disable_recursive_processing,
                    disable_calculate_entropy,
                    disable_calculate_labels,
                    cmd);
    if (error_message.size() != 0) {
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }
  }

  // import_tab
  static void import_tab(const std::string& hashdb_dir,
                     const std::string& tab_file,
                     const std::string& repository_name,
                         const std::string& whitelist_dir,
                     const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // resources
    hashdb::import_manager_t manager(hashdb_dir, cmd);
    hashdb::scan_manager_t* whitelist_manager = NULL;
    if (whitelist_dir != "") {
      require_hashdb_dir(whitelist_dir);
      whitelist_manager = new hashdb::scan_manager_t(whitelist_dir);
    }
    progress_tracker_t progress_tracker(hashdb_dir, 0, cmd);

    // open the tab file for reading
    in_ptr_t in_ptr(tab_file);
    ::import_tab(manager, repository_name, tab_file, whitelist_manager,
                 progress_tracker, *in_ptr());

    // done
    if (whitelist_manager != NULL) {
      delete whitelist_manager;
    }
  }

  // import json
  static void import_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // resources
    hashdb::import_manager_t manager(hashdb_dir, cmd);
    progress_tracker_t progress_tracker(hashdb_dir, 0, cmd);

    // open the JSON file for reading
    in_ptr_t in_ptr(json_file);
    ::import_json(manager, progress_tracker, *in_ptr());
  }

  // export json
  static void export_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // resources
    hashdb::scan_manager_t manager(hashdb_dir);
    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    // open the JSON file for writing
    out_ptr_t out_ptr(json_file);

    // print header to file
    *out_ptr() << "# command: '" << cmd << "'\n"
               << "# hashdb-Version: " << PACKAGE_VERSION << "\n";

    // export the hashdb
    ::export_json_hashes(manager, progress_tracker, *out_ptr());
    ::export_json_sources(manager, *out_ptr());
  }

  // export json range
  static void export_json_range(const std::string& hashdb_dir,
                                const std::string& json_file,
                                const std::string& begin_block_hash,
                                const std::string& end_block_hash,
                                const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // resources
    hashdb::scan_manager_t manager(hashdb_dir);
    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    // open the JSON file for writing
    out_ptr_t out_ptr(json_file);

    // print header to file
    *out_ptr() << "# command: '" << cmd << "'\n"
               << "# hashdb-Version: " << PACKAGE_VERSION << "\n";

    // export the range to the hashdb
    ::export_json_range(manager, begin_block_hash, end_block_hash,
                        progress_tracker, *out_ptr());
  }

  // ************************************************************
  // database manipulation
  // ************************************************************
  // add
  static void add(const std::string& hashdb_dir,
                  const std::string& dest_dir,
                  const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    progress_tracker_t progress_tracker(
                                dest_dir, manager_a.size_hashes(), cmd);
    adder_t adder(&manager_a, &manager_b, &progress_tracker);

    // add data for binary_hash from A to B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      // add the hash
      adder.add(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // add_multiple
  // Flow:
  //   1) Create an ordered multimap of key=hash, value=producer_t
  //      where key is the first key from a producer.
  //   2) Consume elements from the ordered multimap and copy them
  //      until the producers are depleted.  Do not enque when a producer
  //      is depleted.  Done when the ordered multimap becomes empty.
  static void add_multiple(const std::vector<std::string>& p_hashdb_dirs,
                           const std::string& cmd) {

    std::vector<std::string> hashdb_dirs = p_hashdb_dirs;

    // read then strip off dest_dir from end of list
    const std::string dest_dir = hashdb_dirs.back();
    hashdb_dirs.pop_back();

    // validate hashdb directories, maybe make dest_dir
    for (std::vector<std::string>::const_iterator it = hashdb_dirs.begin();
                    it != hashdb_dirs.end(); ++it) {
      require_hashdb_dir(*it);
    }
    create_if_new(dest_dir, hashdb_dirs[0], cmd);

    // open the consumer at dest_dir
    hashdb::import_manager_t consumer(dest_dir, cmd);

    // calculate the total hash records for the tracker
    size_t total_hash_records = 0;
    for (std::vector<std::string>::const_iterator it = hashdb_dirs.begin();
                    it != hashdb_dirs.end(); ++it) {
      hashdb::scan_manager_t scan_manager(*it);
      total_hash_records += scan_manager.size_hashes();
    }

    // start progress tracker
    progress_tracker_t progress_tracker(dest_dir, total_hash_records, cmd);

    // define the ordered multimap of key=hash, value=producer_t
    typedef std::pair<hashdb::scan_manager_t*, adder_t*> producer_t;
    typedef std::pair<std::string, producer_t> ordered_producers_value_t;
    typedef std::multimap<std::string, producer_t> ordered_producers_t;

    // create the multimap of ordered producers
    ordered_producers_t ordered_producers;

    // open the producers
    for (std::vector<std::string>::const_iterator it = hashdb_dirs.begin();
                    it != hashdb_dirs.end(); ++it) {
      std::string hashdb_dir = *it;
      hashdb::scan_manager_t* producer = new hashdb::scan_manager_t(hashdb_dir);
      std::string binary_hash = producer->first_hash();
      if (binary_hash.size() != 0) {
        // the producer is not empty, so enqueue it
        // create the adder
        adder_t* adder = new adder_t(producer, &consumer, &progress_tracker);
        ordered_producers.insert(ordered_producers_value_t(binary_hash,
                                      producer_t(producer, adder)));

      // also track total hashes to be processed
      total_hash_records += producer->size_hashes();

      } else {
        // no hashes for this producer so close it
        delete producer;
      }
    }

    // add ordered hashes from producers until all hashes are consumed
    while (ordered_producers.size() != 0) {
      // get the hash, producer, and adder for the first hash
      ordered_producers_t::iterator it = ordered_producers.begin();
      hashdb::scan_manager_t* producer = it->second.first;
      adder_t* adder = it->second.second;

      // add the hash to the consumer
      adder->add(it->first);

      // get the next hash from this producer
      std::string binary_hash = producer->next_hash(it->first);

      // remove this hash, producer_t entry
      ordered_producers.erase(it);

      if (binary_hash.size() != 0) {
        // hash exists so add the hash, producer, and adder
        ordered_producers.insert(ordered_producers_value_t(binary_hash,
                                      producer_t(producer, adder)));
      } else {
        // no hashes for this producer so close it
        delete producer;
        delete adder;
      }
    }
  }

  // add_repository
  static void add_repository(const std::string& hashdb_dir,
                             const std::string& dest_dir,
                             const std::string& repository_name,
                             const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir,
                                        manager_a.size_hashes(), cmd);
    adder_t adder(&manager_a, &manager_b, repository_name, &progress_tracker);

    // add data for binary_hash from A to B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      // add the hash
      adder.add_repository(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // add_range
  static void add_range(const std::string& hashdb_dir,
                        const std::string& dest_dir,
                        const size_t m,
                        const size_t n,
                        const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir,
                                        manager_a.size_hashes(), cmd);
    adder_t adder(&manager_a, &manager_b, &progress_tracker);

    // add data for binary_hash from A to B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      // add the hash
      adder.add_range(binary_hash, m, n);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // intersect A and B into C
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& dest_dir,
                        const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir, manager_a.size_hashes(), cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c,
                                                           &progress_tracker);

    // iterate A to intersect A and B into C
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      adder_set.intersect(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& dest_dir,
                             const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir, manager_a.size_hashes(), cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c,
                                                          & progress_tracker);

    // iterate A to intersect_hash A and B into C
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      adder_set.intersect_hash(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // subtract
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& dest_dir,
                       const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir, manager_a.size_hashes(), cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c,
                                                          &progress_tracker);

    // iterate A to add A to C if A hash and source not in B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {

      // add A to C if A hash and source not in B
      adder_set.subtract(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // subtract_hash
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& dest_dir,
                            const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir, manager_a.size_hashes(), cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c,
                                                          &progress_tracker);

    // iterate A to add A to C if A hash not in B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {

      // add A to C if A hash not in B
      adder_set.subtract_hash(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // subtract_repository
  static void subtract_repository(const std::string& hashdb_dir,
                                  const std::string& dest_dir,
                                  const std::string& repository_name,
                                  const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    progress_tracker_t progress_tracker(dest_dir,
                                        manager_a.size_hashes(), cmd);
    adder_t adder(&manager_a, &manager_b, repository_name, &progress_tracker);

    // add data for binary_hash from A to B
    std::string binary_hash = manager_a.first_hash();
    while (binary_hash.size() != 0) {
      // add the hash
      adder.add_non_repository(binary_hash);
      binary_hash = manager_a.next_hash(binary_hash);
    }
  }

  // ************************************************************
  // scan
  // ************************************************************
  // scan
  static void scan_list(const std::string& hashdb_dir,
                        const std::string& hashes_file,
                        const hashdb::scan_mode_t scan_mode,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // resources
    hashdb::scan_manager_t manager(hashdb_dir);

    // open the hashes list file for reading
    in_ptr_t in_ptr(hashes_file);

    // print header information
    print_header(cmd);

    // scan the list
    ::scan_list(manager, *in_ptr(), scan_mode);

    // done
    std::cout << "# scan_list completed.\n";
  }

  // scan_hash
  static void scan_hash(const std::string& hashdb_dir,
                        const std::string& hex_block_hash,
                        const hashdb::scan_mode_t scan_mode,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // get the binary hash
    std::string binary_hash = hashdb::hex_to_bin(hex_block_hash);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << hex_block_hash << "'\n";
      exit(1);
    }

    // open DB
    hashdb::scan_manager_t scan_manager(hashdb_dir);

    // scan
    std::string expanded_text = scan_manager.find_hash_json(
                                                  scan_mode, binary_hash);

    if (expanded_text.size() != 0) {
      std::cout << expanded_text << std::endl;
    } else {
      std::cout << "Hash not found for '" << hex_block_hash << "'\n";
    }
  }

  // scan_media
  static void scan_media(const std::string& hashdb_dir,
                         const std::string& media_image_filename,
                         const size_t step_size,
                         const bool disable_recursive_processing,
                         const hashdb::scan_mode_t scan_mode,
                         const std::string& cmd) {

    // print header information
    print_header(cmd);

    // scan
    std::string error_message = hashdb::scan_media(hashdb_dir,
                             media_image_filename, step_size,
                             disable_recursive_processing, scan_mode);
    if (error_message.size() == 0) {
      std::cout << "# scan_media completed.\n";
    } else {
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }
  }

  // ************************************************************
  // statistics
  // ************************************************************
  // size
  static void size(const std::string& hashdb_dir,
                   const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    std::cout << manager.size() << std::endl;
  }

  // sources
  static void sources(const std::string& hashdb_dir,
                      const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print the sources
    ::export_json_sources(manager, std::cout);
  }

  // histogram
  static void histogram(const std::string& hashdb_dir,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print header information
    print_header(cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of distinct hashes
    uint64_t total_distinct_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t> hash_histogram;

    // space for variables
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t source_sub_counts;

    // iterate over hashdb and set variables for calculating the histogram
    std::string binary_hash = manager.first_hash();

    // note if the DB is empty
    if (binary_hash.size() == 0) {
      std::cout << "The map is empty.\n";
    }

    while (binary_hash.size() != 0) {
      manager.find_hash(binary_hash, k_entropy, block_label, count,
                        source_sub_counts);
      // update total hashes observed
      total_hashes += count;
      // update total distinct hashes
      if (count == 1) {
        ++total_distinct_hashes;
      }

      // update hash_histogram information
      // look for existing entry
      std::map<uint32_t, uint64_t>::iterator hash_histogram_it =
                                              hash_histogram.find(count);
      if (hash_histogram_it == hash_histogram.end()) {

        // this is the first hash found with this count value
        // so start a new element for it
        hash_histogram.insert(std::pair<uint32_t, uint64_t>(count, 1));

      } else {

        // increment existing value for number of hashes with this count
        uint64_t old_number = hash_histogram_it->second;
        hash_histogram.erase(count);
        hash_histogram.insert(std::pair<uint32_t, uint64_t>(
                                           count, old_number + 1));
      }

      // move forward
      progress_tracker.track_hash_data(source_sub_counts.size());
      binary_hash = manager.next_hash(binary_hash);
    }

    // show totals
    std::cout << "{\"total_hashes\": " << total_hashes << ", "
              << "\"total_distinct_hashes\": " << total_distinct_hashes << "}\n";

    // show hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>::iterator hash_histogram_it2;
    for (hash_histogram_it2 = hash_histogram.begin();
         hash_histogram_it2 != hash_histogram.end(); ++hash_histogram_it2) {
      std::cout << "{\"duplicates\":" << hash_histogram_it2->first
                << ", \"distinct_hashes\":" << hash_histogram_it2->second
                << ", \"total\":" << hash_histogram_it2->first *
                                 hash_histogram_it2->second << "}\n";
    }
  }

  // duplicates
  static void duplicates(const std::string& hashdb_dir,
                         const std::string& number_string,
                         const hashdb::scan_mode_t scan_mode,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert duplicates string to number
    uint32_t number = atoi(number_string.c_str());

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (manager.size_hashes() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_header(cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    bool any_found = false;

    // space for variables
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t source_sub_counts;

    // iterate over hashdb and set variables for finding duplicates
    std::string binary_hash = manager.first_hash();

    while (binary_hash.size() != 0) {
      manager.find_hash(binary_hash, k_entropy, block_label, count,
                                  source_sub_counts);
      if (count == number) {
        // show hash with requested duplicates number
        std::string expanded_text = manager.find_hash_json(
                                                    scan_mode, binary_hash);
        std::cout << hashdb::bin_to_hex(binary_hash) << "\t"
                  << expanded_text << "\n";
        any_found = true;
      }

      // move forward
      progress_tracker.track_hash_data(source_sub_counts.size());
      binary_hash = manager.next_hash(binary_hash);
    }

    // say so if nothing was found
    if (!any_found) {
      std::cout << "No hashes were found with this count.\n";
      return;
    }
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& hex_file_hash,
                         const hashdb::scan_mode_t scan_mode,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // source data
    std::string file_binary_hash = hashdb::hex_to_bin(hex_file_hash);
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t zero_count = 0;
    uint64_t nonprobative_count = 0;

    // see if this source is even present
    bool has_source_data = manager.find_source_data(file_binary_hash,
                       filesize, file_type, zero_count, nonprobative_count);
    if (has_source_data == false) {
      // the source is not present
      std::cout << "There is no source with this file hash\n";
      return;
    }

    // print header information
    print_header(cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    // space for variables
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t source_sub_counts;

    // look for hashes that belong to this source
    // get the first hash
    std::string binary_hash = manager.first_hash();
    while (binary_hash.size() != 0) {

      // read hash data for the hash
      manager.find_hash(binary_hash, k_entropy, block_label, count,
                                                    source_sub_counts);

      // find sources that match the source we are looking for
      for (hashdb::source_sub_counts_t::const_iterator it =
                       source_sub_counts.begin();
                       it!= source_sub_counts.end(); ++it) {
        if (it->file_hash == file_binary_hash) {

          // the source matches so print the hash and move on
          std::string expanded_text = manager.find_hash_json(
                                                    scan_mode, binary_hash);
          std::cout << hashdb::bin_to_hex(binary_hash) << "\t" << expanded_text
                    << "\n";
          break;
        }
      }

      // move forward
      progress_tracker.track_hash_data(source_sub_counts.size());
      binary_hash = manager.next_hash(binary_hash);
    }
  }

  // read_media
  static void read_media(const std::string& media_image_filename,
                         const std::string& media_offset,
                         const std::string& count_string) {

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // read the bytes
    std::string bytes;
    std::string error_message = hashdb::read_media(
                         media_image_filename, media_offset, count, bytes);

    if (error_message.size() == 0) {
      // print the bytes to stdout
      std::cout << bytes << std::flush;
    } else {
      // print the error to stderr
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }
  }

  // read_media_size
  static void read_media_size(const std::string& media_image_filename) {

    // read the media size
    uint64_t media_size;
    std::string error_message = hashdb::read_media_size(
                         media_image_filename, media_size);

    if (error_message.size() == 0) {
      // print the bytes to stdout
      std::cout << media_size << "\n";
    } else {
      // print the error to stderr
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }
  }

  // ************************************************************
  // performance analysis
  // ************************************************************
  // add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& count_string,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // read settings for byte alignment
    hashdb::settings_t settings;
    std::string error_message = hashdb::read_settings(hashdb_dir, settings);
    if (error_message.size() != 0) {
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }

    const uint64_t byte_alignment = settings.byte_alignment;

    // initialize random seed
    srand (time(NULL));

    // open manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // set up the source
    std::string file_binary_hash = hashdb::hex_to_bin("00");
    manager.insert_source_name(file_binary_hash, "add_random_repository_name",
                               "add_random_filename");
    manager.insert_source_data(file_binary_hash, 0, "", 0, 0);

    // get start index for this run
    uint64_t start_index = manager.size_hashes();
    if (start_index > 1) {
      --start_index;
    }

    // insert count random hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // add hash
      manager.insert_hash(random_binary_hash(), 0.0, "", file_binary_hash);

      // update progress tracker
      progress_tracker.track();
    }
  }

  // scan_random
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& count_string,
                          const hashdb::scan_mode_t scan_mode,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // initialize random seed
    srand (time(NULL)+1); // ensure seed is different by advancing 1 second

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // scan random hashes where hash values are unlikely to match
    for (uint64_t i=1; i<=count; ++i) {
      std::string binary_hash = random_binary_hash();

      std::string expanded_text = manager.find_hash_json(
                                                    scan_mode, binary_hash);

      if (expanded_text.size() != 0) {
        std::cout << "Match found, hash "
                  << hashdb::bin_to_hex(binary_hash)
                  << ": " << expanded_text << "\n";
      }

      // update progress tracker
      progress_tracker.track();
    }
  }

  // add_same
  // add same hash but different source offset
  static void add_same(const std::string& hashdb_dir,
                       const std::string& count_string,
                       const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // read settings for byte alignment
    hashdb::settings_t settings;
    std::string error_message = hashdb::read_settings(hashdb_dir, settings);
    if (error_message.size() != 0) {
      std::cerr << "Error: " << error_message << "\n";
      exit(1);
    }

    const uint64_t byte_alignment = settings.byte_alignment;

    // open manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // set up the source
    std::string file_binary_hash = hashdb::hex_to_bin("00");
    manager.insert_source_name(file_binary_hash, "add_same_repository_name",
                               "add_same_filename");
    manager.insert_source_data(file_binary_hash, 0, "", 0, 0);

    // hash to use
    std::string binary_hash =
                   hashdb::hex_to_bin("80000000000000000000000000000000");

    // get start index for this run
    uint64_t start_index = manager.size_hashes();
    if (start_index > 1) {
      --start_index;
    }

    // insert count same hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // add hash
      manager.insert_hash(binary_hash, 0.0, "", file_binary_hash);

      // update progress tracker
      progress_tracker.track();
    }
  }

  // scan_same
  static void scan_same(const std::string& hashdb_dir,
                        const std::string& count_string,
                        const hashdb::scan_mode_t scan_mode,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // hash to use
    std::string binary_hash =
                   hashdb::hex_to_bin("80000000000000000000000000000000");

    // scan same hash repeatedly
    for (uint64_t i=1; i<=count; ++i) {
      std::string expanded_text = manager.find_hash_json(
                                                    scan_mode, binary_hash);

      if (expanded_text.size() == 0) {
        std::cout << "Match not found, hash "
                  << hashdb::bin_to_hex(binary_hash)
                  << ": " << expanded_text << "\n";
      }

      // update progress tracker
      progress_tracker.track();
    }
  }

  // test_scan_stream
  static void test_scan_stream(const std::string& hashdb_dir,
                               const std::string& count_string,
                               const hashdb::scan_mode_t scan_mode,
                               const std::string& cmd) {

    const size_t list_size = 10000;

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = s_to_uint64(count_string);

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // open scan_stream
    hashdb::scan_stream_t scan_stream(&manager, 16, scan_mode);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, list_size * count, cmd);

    // hash to use
    std::string binary_hash =
                   hashdb::hex_to_bin("80000000000000000000000000000000");

    // prepare the unscanned record of 10,000
    std::stringstream ss;
    for (size_t i=0; i<list_size; ++i) {
      const uint16_t index_length = std::to_string(i).size();
      ss << binary_hash;
      ss.write(reinterpret_cast<const char*>(&index_length), sizeof(uint16_t));
      ss << i;
    }
    const std::string unscanned(ss.str());

    // put/get data
    for (uint64_t i=1; i<=count; ++i) {
      scan_stream.put(unscanned);
      const std::string scanned = scan_stream.get();
      if (scanned.size() > 0) {
        progress_tracker.track_count(list_size);
      }
    }

    // get data until processing is done
    while (!scan_stream.empty()) {
      const std::string scanned = scan_stream.get();
      if (scanned.size() > 0) {
        progress_tracker.track_count(list_size);
      }
    }
  }
}

#endif

