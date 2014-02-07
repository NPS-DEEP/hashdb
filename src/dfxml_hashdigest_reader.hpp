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
 * Provides the service of reading hash data from a DFXML file
 * typically created by md5deep or by a hashdb_manager export operation
 * and calling a consumer to process the hash data.
 */

/**
 * NOTE: libxml2 uses type xmlChar which is unsigned char, which is
 * typecast to char.
 */

#ifndef DFXML_HASHDIGEST_READER_HPP
#define DFXML_HASHDIGEST_READER_HPP
//#include "hashdb_types.h"

#include <libxml/parser.h>
#include <iostream>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

// a class is used just to keep members private
template <class T>
class dfxml_hashdigest_reader_t {
  private:

  // do not allow these
  dfxml_hashdigest_reader_t();
  dfxml_hashdigest_reader_t(const dfxml_hashdigest_reader_t&);
  dfxml_hashdigest_reader_t& operator=(const dfxml_hashdigest_reader_t&);

  // ************************************************************
  // user data type for sax
  // ************************************************************
  struct user_data_t {

    // input values provided by do_read()
    std::string default_repository_name;
    T* consumer;

    // state variables
    bool is_at_repository_name;
    bool is_at_filename;
    bool is_at_hashdigest;
    bool has_filename;
    bool has_repository_name;
    bool has_byte_run_file_offset_attribute;
    bool has_byte_run_len_attribute;
    bool has_hashdigest_type_attribute;

    // parsed xml values
    std::string repository_name;
    std::string filename;
    uint64_t byte_run_file_offset_attribute;
    uint64_t byte_run_len_attribute;
    std::string hashdigest_type_attribute;
    std::string hashdigest;

    user_data_t(const std::string& p_default_repository_name, T* p_consumer) :
                         default_repository_name(p_default_repository_name),
                         consumer(p_consumer),
                         is_at_repository_name(false),
                         is_at_filename(false),
                         is_at_hashdigest(false),
                         has_filename(false),
                         has_repository_name(false),
                         has_byte_run_file_offset_attribute(false),
                         has_byte_run_len_attribute(false),
                         has_hashdigest_type_attribute(false),
                         repository_name(""),
                         filename(""),
                         byte_run_file_offset_attribute(0),
                         byte_run_len_attribute(0),
                         hashdigest_type_attribute(""),
                         hashdigest("") {
    }
    // don't allow these
    private:
    user_data_t(const user_data_t&);
    user_data_t& operator=(const user_data_t&);
  };

  // ************************************************************
  // static sax handler helpers
  // ************************************************************

  static void save_hashdigest(user_data_t& user_data) {

    // create hash source record with available attribute information in it

    // get the repository name from xml or use default
    std::string selected_repository_name = (user_data.has_repository_name) ?
                user_data.repository_name : user_data.default_repository_name;

    // create hash source record
    hash_source_record_t hash_source_record(
               user_data.byte_run_file_offset_attribute,
               user_data.byte_run_len_attribute,
               user_data.hashdigest_type_attribute,
               selected_repository_name,
               user_data.filename);

    // retype hashdigest to md5_t
    md5_t md5=md5_t(md5_t::fromhex(user_data.hashdigest));

//std::cout << "dfxml_hashdigest_reader_t::save_hashdigest hash source record " << md5 << ", " << hash_source_record << "\n";

    // create hash element to be submitted to consumer
    hashdb_element_t hashdb_element(md5, hash_source_record);

    // call the consumer
    user_data.consumer->consume(hashdb_element);
  }

  // parse byte_run tag for possible file_offset or len attributes
  static void parse_byte_run_attributes(user_data_t& user_data,
                                        const xmlChar** attributes) {
    user_data.has_byte_run_file_offset_attribute = false;
    user_data.has_byte_run_len_attribute = false;

    if (attributes == NULL) {
      // no attributes
      return;
    }

    // parse attributes
    int i=0;
    while (true) {
      if (attributes[i] == NULL || attributes[i+1] == NULL) {
        // done
        return;
      }

      if (xmlStrEqual(attributes[i], reinterpret_cast<const xmlChar*>("file_offset"))) {
        std::string file_offset_string((const char*)attributes[i+1]);
        try {
          user_data.byte_run_file_offset_attribute
                     = boost::lexical_cast<uint64_t>(file_offset_string);
          user_data.has_byte_run_file_offset_attribute = true;
        } catch(...) {
          // abort
          std::cerr << "dfxml_hashdigest_reader_t::on_start_element(): invalid byte_run file_offset attribute: '" << file_offset_string << "'\n";
          return;
        }
      } else if (xmlStrEqual(attributes[i], reinterpret_cast<const xmlChar*>("len"))) {
        std::string len_string((const char*)attributes[i+1]);
        try {
          user_data.byte_run_len_attribute = boost::lexical_cast<size_t>(len_string);
          user_data.has_byte_run_len_attribute = true;
        } catch(...) {
          // abort
          std::cerr << "dfxml_hashdigest_reader_t::on_start_element(): invalid byte_run len attribute: '" << len_string << "'\n";
          return;
        }
      }

      i += 2;
    }
  }

  // parse "hashdigest" tag for possible "type" attribute
  static void parse_hashdigest_attributes(user_data_t& user_data,
                                          const xmlChar** attributes) {
    user_data.has_hashdigest_type_attribute = false;

    if (attributes == NULL) {
      // no attributes
      return;
    }

    // parse attributes
    int i=0;
    while (true) {
      if (attributes[i] == NULL || attributes[i+1] == NULL) {
        return;
      }

      if (xmlStrEqual(attributes[i], reinterpret_cast<const xmlChar*>("type"))) {
        user_data.has_hashdigest_type_attribute = true;
        user_data.hashdigest_type_attribute = std::string((const char*)attributes[i+1]);
      }

      i += 2;
    }
  }

  // ************************************************************
  // static sax handlers
  // ************************************************************
  static void on_start_document(void* p_user_data ATTRIBUTE_UNUSED) {
    // no action
  }
  static void on_end_document(void* p_user_data ATTRIBUTE_UNUSED) {
    // no action
  }

  static void on_start_element(void* p_user_data,
                               const xmlChar* name,
                               const xmlChar** attributes) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));
    user_data.is_at_repository_name = false;
    user_data.is_at_filename = false;

    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("fileobject"))) {
      // a filename tag is required after a fileobject tag
      user_data.has_filename = false;
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("repository_name"))) {
      // repository_name establishes the repository name for future hashdigest values
      user_data.is_at_repository_name = true;
      user_data.has_repository_name = false;
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("filename"))) {
      // a filename tag establishes the filename for future hashdigest values
      user_data.is_at_filename = true;
      user_data.has_filename = false;
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("byte_run"))) {
      parse_byte_run_attributes(user_data, attributes);
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hashdigest"))) {
      user_data.is_at_hashdigest = true;
      parse_hashdigest_attributes(user_data, attributes);
    }
  }

  static void on_end_element(void* p_user_data,
                             const xmlChar* name) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));
//      std::cout << "dfxml_hashdigest_reader_t::on_end_name " << name << "\n";
    // no action
    user_data.is_at_repository_name = false;
    user_data.is_at_filename = false;
    user_data.is_at_hashdigest = false;

    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("fileobject"))) {
      user_data.has_filename = false;
      user_data.has_repository_name = false;
    }
  }

  static void on_characters(void* p_user_data,
                            const xmlChar* characters,
                            int len) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

//      std::cout << "dfxml_hashdigest_reader_t::on_characters\n";
    if (user_data.is_at_repository_name) {
      // get repository name to be used for the hashdigest
      char repository_name_chars[len+1];
      strncpy(repository_name_chars, (const char *)characters, len);
      repository_name_chars[len] = (char)NULL;
      user_data.repository_name = std::string(repository_name_chars);
      user_data.has_repository_name = true;

    } else if (user_data.is_at_filename) {
      // get filename to be used for the hashdigest
      char filename_chars[len+1];
      strncpy(filename_chars, (const char *)characters, len);
      filename_chars[len] = (char)NULL;
      user_data.filename = std::string(filename_chars);
      user_data.has_filename = true;

    } else if (user_data.is_at_hashdigest
               && user_data.has_byte_run_file_offset_attribute
               && user_data.has_byte_run_len_attribute) {

      // get hashdigest
      char hashdigest_chars[len+1];
      strncpy(hashdigest_chars, (const char *)characters, len);
      hashdigest_chars[len] = (char)NULL;
      user_data.hashdigest = std::string(hashdigest_chars);

      // save hashdigest
      save_hashdigest(user_data);

      // hashdigest consumed the byte_run information
      user_data.has_byte_run_file_offset_attribute = false;
      user_data.has_byte_run_len_attribute = false;
    }
  }

  static void on_comment(void* p_user_data ATTRIBUTE_UNUSED) {
  //    std::cout << "dfxml_hashdigest_reader_t::on_comment " << text << "\n";
  }

  static void on_warning(void* p_user_data ATTRIBUTE_UNUSED,
                         const char* msg,
                         ...) {
    // adapted from http://stackoverflow.com/questions/5977326/call-printf-using-va-list
    va_list arglist;
    printf("dfxml_hashdigest_reader on_warning(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "dfxml_hashdigest_reader on_warning(): " << msg << std::endl;
  }

  static void on_error(void* p_user_data ATTRIBUTE_UNUSED,
                       const char* msg,
                       ...) {
    va_list arglist;
    printf("dfxml_hashdigest_reader on_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "dfxml_hashdigest_reader on_error(): " << msg << std::endl;
  }

  static void on_fatal_error(void* p_user_data ATTRIBUTE_UNUSED,
                             const char* msg,
                             ...) {
    va_list arglist;
    printf("dfxml_hashdigest_reader on_fatal_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "dfxml_hashdigest_reader on_fatal_error(): " << msg << std::endl;
  }

  // ************************************************************
  // class dfxml_hashdigest_reader_t
  // ************************************************************
  /**
   * Provides the service of reading hash data from DFXML format into a hashdb.
   */

  public:
  static bool do_read(
                const std::string& dfxml_file,
                const std::string& default_repository_name,
                T* consumer) {

    // set up the sax callback data structure with context-relavent handlers
    xmlSAXHandler sax_handlers = {
      NULL,			// internalSubset
      NULL,			// isStandalone
      NULL,			// hasInternalSubset
      NULL,			// hasExternalSubset
      NULL,			// resolveEntity
      NULL,			// getEntity
      NULL,			// entityDecl
      NULL,			// notationDecl
      NULL,			// attributeDecl
      NULL,			// elementDecl
      NULL,			// unparsedEntityDecl
      NULL,			// setDocumentLocator
      on_start_document,	// startDocument
      on_end_document,		// endDocument
      on_start_element,		// startElement
      on_end_element,		// endElement
      NULL,			// reference
      on_characters,		// characters
      NULL,			// ignorableWhitespace
      NULL,			// processingInstruction
      NULL,			// comment
      on_warning,		// xmlParserWarning
      on_error,			// xmlParserError
      on_fatal_error,		// xmlParserFatalError
      NULL,			// getParameterEntity
      NULL,			// cdataBlock
      NULL,			// externalSubset
      1     			// is initialized
    };

    // set up the data structure for the sax handlers to use
    user_data_t user_data(default_repository_name, consumer);

    // perform the sax parse on the file
    int sax_parser_resource = xmlSAXUserParseFile(
                              &sax_handlers, &user_data, dfxml_file.c_str());

    // clean up libxml2 malloc resources to help memory leak analysis using
    // tools such as valgrind.  Call only in non-multithreaded environment.
    xmlCleanupParser();

    if (sax_parser_resource == 0) {
      // good, no failure
      return true;
    } else {
      // something went wrong
      return false;
    }
  }
};
#endif

