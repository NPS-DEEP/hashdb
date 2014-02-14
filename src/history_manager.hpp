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

static std::string XML_HEADER = "<?xml version='1.0' encoding='UTF-8'?>";

class history_manager_failure_exception_t: public std::exception {
  public:
  virtual const char* what() const throw() {
    return "Error: history manager failure";
  }
};

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
      std::cerr << "Cannot open " << filename << ": " << strerror(errno) << "\n";
      throw history_manager_failure_exception_t();
    }

    // read log file into vector of lines
    std::string line;
    while(getline(in, line)) {
      lines.push_back(line);
    }

    in.close();
  }

  static void strip_xml_header(std::vector<std::string>& lines) {
    // abort if header is wrong
    if (lines.size() < 1 || lines[0] != XML_HEADER) {
      std::cerr << "strip_xml_header: invalid header\n";
      throw history_manager_failure_exception_t();
    }

    // strip the header
    lines.erase(lines.begin());
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
      std::cerr << "strip_outer_tag: invalid size\n";
      throw history_manager_failure_exception_t();
    }

    // remove open tag
    std::vector<std::string>::iterator it = lines.begin();
    if (*it == XML_HEADER) {
      ++it;
    }
    if (*it != open_tag) {
      std::cerr << "strip_outer_tag: no open tag\n";
      throw history_manager_failure_exception_t();
    }
    lines.erase(it);

    // remove close tag
    if (lines.back() != close_tag) {
      std::cerr << "strip_outer_tag: no close tag,\n";
      std::cerr << "'" << lines.back() << "' is not '" << close_tag << "'\n";
      throw history_manager_failure_exception_t();
    }
    lines.pop_back();
  }

/*
  static void write_lines(const std::string& filename,
                   const std::vector<std::string>& lines) {

    // define scratch file
    std::string scratchfile = filename + ".scratch";

    // open scratch file
    std::fstream outf;
    outf.open(scratchfile.c_str(),std::ios_base::out);
    if (!outf.is_open()) {
      throw history_manager_failure_exception_t();
    }

    // copy lines to scratch file
    for (std::vector<std::string>::const_iterator it = lines.begin(); it != lines.end(); ++it) {
      outf << line << std::endl;
    }

    // close outfile
    outf.close();

    // move scratch file onto file
    std::rename
  }
*/

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

  public:
  /**
   * Append hashdb log to history.
   */
  static void append_log_to_history(const std::string& hashdb_dir) {
    try {

      // read the old history lines, if available
      const std::string history_filename = hashdb_dir + "/history.xml";
      std::vector<std::string> history_lines;
      bool has_history = (access(history_filename.c_str(), F_OK) == 0);
      if (has_history) {
        read_lines(history_filename, history_lines);

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

      // open scratch history output stream
      const std::string scratch = history_filename + ".scratch";
      std::fstream outf;
      outf.open(scratch.c_str(),std::ios_base::out);
      if (!outf.is_open()) {
        std::cerr << "append_log_to_history: unable to open scratch file\n";
        throw history_manager_failure_exception_t();
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

      // rename scratch file to history file, replacing existing history file
      std::rename(scratch.c_str(), history_filename.c_str());
    } catch (history_manager_failure_exception_t &e) {
      std::cerr << "Warning: unable to write hashdb history file.\n";
    }
  }

  /**
   * Merge old hashdb history to new hashdb history.
   */
  static void merge_history_to_history(const std::string& old_hashdb_dir,
                                       const std::string& new_hashdb_dir) {
    try {

      // read the old history lines
      const std::string old_history_filename = old_hashdb_dir + "/history.xml";
      std::vector<std::string> old_history_lines;
      read_lines(old_history_filename, old_history_lines);

      // strip off header and outer tags from old history
      strip_xml_header(old_history_lines);
      strip_outer_tag("history", old_history_lines);

      // embed old history lines inside new "old_history" tag
      embed_in_tag("old_history", old_history_lines);

      // read any new history lines, if available
      const std::string new_history_filename = new_hashdb_dir + "/history.xml";
      std::vector<std::string> new_history_lines;
      bool has_new_history = (access(new_history_filename.c_str(), F_OK) == 0);
      if (has_new_history) {
        read_lines(new_history_filename, new_history_lines);

        // strip off header and outer tags of new history lines
        strip_xml_header(new_history_lines);
        strip_outer_tag("history", new_history_lines);
      }

      // open scratch history output stream
      const std::string scratch = new_history_filename + ".scratch";
      std::fstream outf;
      outf.open(scratch.c_str(),std::ios_base::out);
      if (!outf.is_open()) {
        std::cerr << "merge_history_to_history: unable to open scratch file\n";
        throw history_manager_failure_exception_t();
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

      // rename scratch file to history file, replacing existing history file
      std::rename(scratch.c_str(), new_history_filename.c_str());
    } catch (history_manager_failure_exception_t &e) {
      std::cerr << "Warning: unable to write hashdb history file.\n";
    }
  }
};

#endif

