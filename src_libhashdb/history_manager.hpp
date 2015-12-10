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
 * Provides history log services.
 */

#ifndef HISTORY_MANAGER_HPP
#define HISTORY_MANAGER_HPP

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

static std::string XML_HEADER = "<?xml version='1.0' encoding='UTF-8'?>";

/**
 * Provides services for managing a log history.
 */
class history_manager_t {
  private:

  // no constructor
  history_manager_t();

  static void read_lines(const std::string& filename,
                  std::vector<std::string>& lines) {
    lines.clear();

    // open log file as text file stream
    std::fstream in(filename.c_str());
    if (!in.is_open()) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "Cannot open " << filename << ": " << strerror(errno) << "\n";
    } else {

      // read log file into vector of lines
      std::string line;
      while(getline(in, line)) {
        lines.push_back(line);
      }

      in.close();
    }
  }

  static void strip_xml_header(std::vector<std::string>& lines) {
    // abort if header is wrong
    if (lines.size() < 1 || lines[0] != XML_HEADER) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "strip_xml_header: invalid header\n";
    } else {

      // strip the header
      lines.erase(lines.begin());
    }
  }

  // strip named outer tag from lines
  static void strip_outer_tag(const std::string& tag,
                       std::vector<std::string>& lines) {

//std::cout << "history_manager.strip_outer tag lines begin:\n";
//for (int i=0; i<lines.size(); ++i) {
//std::cout << "    '" << lines[i] << "'\n";
//}

    std::string open_tag = "<" + tag + ">";
    std::string close_tag = "</" + tag + ">";

    // require minimum size
    if (lines.size() < 3) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "strip_outer_tag: invalid size\n";
    }

    // remove open tag
    std::vector<std::string>::iterator it = lines.begin();
    if (*it == XML_HEADER) {
      ++it;
    }
    if (*it != open_tag) {
      std::cerr << "Warning: history log failure.\n";
    }
    lines.erase(it);

    // remove close tag
    if (lines.back() != close_tag) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "strip_outer_tag: no close tag,\n";
      std::cerr << "'" << lines.back() << "' is not '" << close_tag << "'\n";
    }
    lines.pop_back();
  }

  // embed lines within new tag and indent embedded lines two spaces
  static void embed_in_tag(std::string tag, std::vector<std::string>& lines) {
    std::string open_tag = "<" + tag + ">";
    std::string close_tag = "</" + tag + ">";

    std::vector<std::string> old_lines(lines);
    lines.clear();

    // put in open tag
    lines.push_back("  " + open_tag);

    // put in indented lines
    for (std::vector<std::string>::const_iterator it = old_lines.begin(); it != old_lines.end(); ++it) {
      lines.push_back("  " + *it);
    }

    // put in close tag
    lines.push_back("  " + close_tag);
  }

  // windows rename does not overwrite, so do this
  static void move_onto(std::string history_file, std::string backup_history_file) {

    // remove existing history file; it will not exist if the db is new
    std::remove(backup_history_file.c_str());

    // rename scratch file to history file, replacing existing history file
    int status = std::rename(history_file.c_str(), backup_history_file.c_str());
    if (status != 0) {
      std::cerr << "Warning: unable to back up '" << history_file
                << "' to '" << backup_history_file << "': "
                << strerror(status) << "\n";
    }
  }

  public:
  /**
   * Append hashdb log to history.
   */
  static void append_log_to_history(const std::string& hashdb_dir) {

    const std::string history_filename = hashdb_dir + "/history.xml";
    const std::string old_history_filename = hashdb_dir + "/_old_history.xml";

    // move history to old_history if it history exists
    if (access(history_filename.c_str(), F_OK) == 0) {
      move_onto(history_filename.c_str(), old_history_filename.c_str());
    }

    // read the old history lines, if available
    std::vector<std::string> history_lines;
    if (access(old_history_filename.c_str(), F_OK) == 0) {
      read_lines(old_history_filename, history_lines);

      // strip off header and outer tags
      strip_xml_header(history_lines);
      strip_outer_tag("history", history_lines);
    }

    // read the log lines
    const std::string log_filename = hashdb_dir + "/log.xml";
    std::vector<std::string> log_lines;
    read_lines(log_filename, log_lines);

    // strip off header and outer tags from log lines
    strip_xml_header(log_lines);
    strip_outer_tag("log", log_lines);

    // open history output stream
    std::fstream outf;
    outf.open(history_filename.c_str(),std::ios_base::out);
    if (!outf.is_open()) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "append_log_to_history: unable to open history file\n";
      return;
    }

    // write header and open history tag
    outf << XML_HEADER << std::endl;
    outf << "<history>" << std::endl;

    // write the old history lines
    for (std::vector<std::string>::const_iterator history_it = history_lines.begin(); history_it != history_lines.end(); ++history_it) {
      outf << *history_it << std::endl;
    }

    // write the new log lines
    for (std::vector<std::string>::const_iterator log_it = log_lines.begin(); log_it != log_lines.end(); ++log_it) {
      outf << *log_it << std::endl;
    }

    // write closure history tag
    outf << "</history>" << std::endl;

    // close outfile
    outf.close();
  }

  /**
   * Merge old hashdb history to new hashdb history.
   */
  static void merge_history_to_history(const std::string& old_hashdb_dir,
                                       const std::string& new_hashdb_dir) {

    const std::string old_history_filename = old_hashdb_dir + "/history.xml";
    const std::string new_history_filename = new_hashdb_dir + "/history.xml";
    const std::string old_new_history_filename = new_hashdb_dir + "/_old_history.xml";

    // move new_history to old_new_history if new_history exists
    move_onto(new_history_filename.c_str(), old_new_history_filename.c_str());

    // read old history lines
    std::vector<std::string> old_history_lines;
    read_lines(old_history_filename, old_history_lines);

    // strip off header and outer tags from old history lines
    strip_xml_header(old_history_lines);
    strip_outer_tag("history", old_history_lines);

    // embed old history lines inside new "old_history" tag
    embed_in_tag("old_history", old_history_lines);

    // read new history lines
    std::vector<std::string> new_history_lines;
    read_lines(old_new_history_filename, new_history_lines);

    // strip off header and outer tags of new history lines
    strip_xml_header(new_history_lines);
    strip_outer_tag("history", new_history_lines);

    // open new history output stream
    std::fstream outf;
    outf.open(new_history_filename.c_str(),std::ios_base::out);
    if (!outf.is_open()) {
      std::cerr << "Warning: history log failure.\n";
      std::cerr << "merge_history_to_history: unable to open new history file\n";
      return;
    }

    // write header and open history tag
    outf << XML_HEADER << std::endl;
    outf << "<history>" << std::endl;

    // write any new history lines
    for (std::vector<std::string>::const_iterator new_history_it = new_history_lines.begin(); new_history_it != new_history_lines.end(); ++new_history_it) {
      outf << *new_history_it << std::endl;
    }

    // append the old history lines
    for (std::vector<std::string>::const_iterator old_history_it = old_history_lines.begin(); old_history_it != old_history_lines.end(); ++old_history_it) {
      outf << *old_history_it << std::endl;
    }

    // write closure history tag
    outf << "</history>" << std::endl;

    // close outfile
    outf.close();
  }
};

#endif

