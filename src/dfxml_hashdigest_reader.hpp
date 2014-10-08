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
 * and calling a hash consumer to process the hash data.
 */

/**
 * NOTE: libxml2 uses type xmlChar which is unsigned char, which is
 * typecast to char.
 */

#ifndef DFXML_HASHDIGEST_READER_HPP
#define DFXML_HASHDIGEST_READER_HPP

#include <libxml/parser.h>
#include <iostream>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include "hash_t_selector.h"

// a class is used just to keep members private
// Note that HC is the hash consumer.
// Note that SC is the source metadata consumer.
template <class HC, class SC>
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
    const std::string default_repository_name;
    HC* hash_consumer;
    SC* source_metadata_consumer;

    // state variables
    bool under_fileobject;
    bool under_repository_name;
    bool under_filename;
    bool under_filesize;
    bool under_fileobject_hashdigest;
    bool under_byte_run;
    bool under_byte_run_hashdigest;

    // parsed byte run values
    std::string byte_run_file_offset;
    std::string byte_run_len;
    std::string byte_run_hashdigest_type;
    std::string byte_run_hashdigest;

    // parsed fileobject values
    std::string fileobject_repository_name;
    std::string fileobject_filename;
    std::string fileobject_filesize;
    std::string fileobject_hashdigest_type;
    std::string fileobject_hashdigest;

    user_data_t(const std::string& p_default_repository_name,
                                             HC* p_hash_consumer,
                                             SC* p_source_metadata_consumer) :
                         default_repository_name(p_default_repository_name),
                         hash_consumer(p_hash_consumer),
                         source_metadata_consumer(p_source_metadata_consumer),
                         under_fileobject(false),
                         under_repository_name(false),
                         under_filename(false),
                         under_filesize(false),
                         under_fileobject_hashdigest(false),
                         under_byte_run(false),
                         under_byte_run_hashdigest(false),
                         byte_run_file_offset(""),
                         byte_run_len(""),
                         byte_run_hashdigest_type(""),
                         byte_run_hashdigest(""),
                         fileobject_repository_name(""),
                         fileobject_filename(""),
                         fileobject_filesize(""),
                         fileobject_hashdigest_type(""),
                         fileobject_hashdigest("") {
    }
    // don't allow these
    private:
    user_data_t(const user_data_t&);
    user_data_t& operator=(const user_data_t&);
  };

  // ************************************************************
  // static sax handler helpers
  // ************************************************************

  static void consume_byte_run_hash(user_data_t& user_data) {

    // pull together byte_run fields for the hashdb element

    // validate hash
    std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(user_data.byte_run_hashdigest);
    if (hash_pair.first == false) {
      std::cerr << "Invalid hashdigest: '"
                << user_data.byte_run_hashdigest << "', entry ignored.\n";
      return;
    }

    // get file_offset
    uint64_t file_offset;
    try {
      file_offset = boost::lexical_cast<uint64_t>(user_data.byte_run_file_offset);
    } catch(...) {
      std::cerr << "Invalid file_offset value: '"
                << user_data.byte_run_file_offset << "', entry ignored.\n";
      return;
    }

    // get hash_block_size
      uint32_t hash_block_size;
    try {
      hash_block_size = boost::lexical_cast<uint64_t>(user_data.byte_run_len);
    } catch(...) {
      std::cerr << "Invalid byte_run value: '"
                << user_data.byte_run_len << "', entry ignored.\n";
      return;
    }

    // validate hashdigest type
    if (user_data.byte_run_hashdigest_type != digest_name<hash_t>()) {
      std::cerr << "dfxml_hashdigest_reader: Wrong hashdigest type for byte_run: "
                << user_data.byte_run_hashdigest_type << "', entry ignored.\n";
      return;
    }
 
    // create the hashdb element
    hashdb_element_t hashdb_element(
               hash_pair.second,
               hash_block_size,
               user_data.fileobject_repository_name,
               user_data.fileobject_filename,
               file_offset);

    // call the hash consumer
    user_data.hash_consumer->consume(hashdb_element);
  }

  static void consume_source_metadata(user_data_t& user_data) {
    // do not consume unless all metadata fields are there
    if (user_data.fileobject_hashdigest_type == "" ||
        user_data.fileobject_hashdigest == "" ||
        user_data.fileobject_filesize == "") {
      return;
    }

    // validate hashdigest type
    if (user_data.fileobject_hashdigest_type != digest_name<hash_t>()) {
      std::cerr << "dfxml_hashdigest_reader: Wrong hashdigest type for fileobject: "
                << user_data.fileobject_hashdigest_type << "', entry ignored.\n";
      return;
    }

    // validate hash
    std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(user_data.fileobject_hashdigest);
    if (hash_pair.first == false) {
      std::cerr << "Invalid hashdigest: '"
                << user_data.fileobject_hashdigest << "', entry ignored.\n";
      return;
    }

    // get file size
    uint64_t file_size;
    try {
      file_size = boost::lexical_cast<uint64_t>(user_data.fileobject_filesize);
    } catch(...) {
      std::cerr << "Invalid filesize value: '"
                << user_data.fileobject_filesize << "', entry ignored.\n";
      return;
    }


    // create the source metadata element
    source_metadata_element_t source_metadata_element(
               user_data.fileobject_repository_name,
               user_data.fileobject_filename,
               file_size,
               hash_pair.second);
 
    // call the consumer
    user_data.source_metadata_consumer->consume(source_metadata_element);
  }

  // parse byte_run tag for possible file_offset or len attributes
  static void parse_byte_run_attributes(user_data_t& user_data,
                                    const xmlChar** attributes) {

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
        user_data.byte_run_file_offset = std::string ((const char*)attributes[i+1]);
      } else if (xmlStrEqual(attributes[i], reinterpret_cast<const xmlChar*>("len"))) {
        user_data.byte_run_len = std::string((const char*)attributes[i+1]);
      }

      i += 2;
    }
  }

  // parse byte_run hashdigest tag for possible "type" attribute
  static void parse_byte_run_hashdigest_attributes(user_data_t& user_data,
                                          const xmlChar** attributes) {

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
        user_data.byte_run_hashdigest_type = std::string((const char*)attributes[i+1]);
      }
      i += 2;
    }
  }

  // parse fileobject hashdigest tag for possible "type" attribute
  static void parse_fileobject_hashdigest_attributes(user_data_t& user_data,
                                          const xmlChar** attributes) {

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
        user_data.fileobject_hashdigest_type = std::string((const char*)attributes[i+1]);
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

  // example syntax:
  // <fileobject>
  //   <repository_name>repo</repository_name>
  //   <filename>/home/bdallen/demo/demo_video.mp4</filename>
  //   <filesize>10630146</filesize>
  //   <byte_run file_offset='0' len='4096'>   
  //     <hashdigest type='MD5'>63641a3c008a3d26a192c778dd088868</hashdigest>
  //   </byte_run>
  //   <hashdigest type='MD5'>a003483521c181d26e66dc09740e939d</hashdigest>
  // </fileobject>
 
  static void on_start_element(void* p_user_data,
                               const xmlChar* name,
                               const xmlChar** attributes) {

    // set up convenient reference to user data
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    // set state based on tag name

    // fileobject
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("fileobject"))) {
      user_data.under_fileobject = true;

      // clear fields under fileobject
      user_data.fileobject_repository_name = user_data.default_repository_name;
      user_data.fileobject_filename = "";
      user_data.fileobject_filesize = "";
      user_data.fileobject_hashdigest_type = "";
      user_data.fileobject_hashdigest = "";

    // repository_name
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("repository_name"))) {
      user_data.under_repository_name = true;

    // filename
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("filename"))) {
      user_data.under_filename = true;

    // file size
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("filesize"))) {
      user_data.under_filesize = true;

    // byte_run
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("byte_run"))) {
      user_data.under_byte_run = true;

      // clear fields under byte_run
      user_data.byte_run_file_offset = "";
      user_data.byte_run_len = "";
      user_data.byte_run_hashdigest_type = "";
      user_data.byte_run_hashdigest = "";

      parse_byte_run_attributes(user_data, attributes);

    // hashdigest
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hashdigest"))) {
      if (user_data.under_byte_run) {
        user_data.under_byte_run_hashdigest = true;
        parse_byte_run_hashdigest_attributes(user_data, attributes);
      } else {
        user_data.under_fileobject_hashdigest = true;
        parse_fileobject_hashdigest_attributes(user_data, attributes);
      }

    } else {
      // no action for other tag names
    }
  }

  static void on_end_element(void* p_user_data,
                             const xmlChar* name) {

    // set up convenient reference to user data
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    // fileobject
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("fileobject"))) {
      consume_source_metadata(user_data);
      user_data.under_fileobject = false;

    // repository_name
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("repository_name"))) {
      user_data.under_repository_name = false;

    // filename
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("filename"))) {
      user_data.under_filename = false;

    // file size
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("filesize"))) {
      user_data.under_filesize = false;

    // byte_run
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("byte_run"))) {
      consume_byte_run_hash(user_data);
      user_data.under_byte_run = false;

    // hashdigest
    } else if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hashdigest"))) {
      if (user_data.under_byte_run) {
        user_data.under_byte_run_hashdigest = false;
      } else {
        user_data.under_fileobject_hashdigest = false;
      }

    } else {
      // no action for other tag names
    }
  }

  static void on_characters(void* p_user_data,
                            const xmlChar* characters,
                            int len) {

  // NOTE: under* booleans are treated flat so they don't hide state.

  // example syntax:
  // <fileobject>
  //   <repository_name>repo</repository_name>
  //   <filename>/home/bdallen/demo/demo_video.mp4</filename>
  //   <filesize>10630146</filesize>
  //   <byte_run file_offset='0' len='4096'>   
  //     <hashdigest type='MD5'>63641a3c008a3d26a192c778dd088868</hashdigest>
  //   </byte_run>
  //   <hashdigest type='MD5'>a003483521c181d26e66dc09740e939d</hashdigest>
  // </fileobject>
 
    // set up convenient reference to user data
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    // repository name
    if (user_data.under_repository_name) {
      char repository_name_chars[len+1];
      strncpy(repository_name_chars, (const char *)characters, len);
      repository_name_chars[len] = (char)NULL;
      user_data.fileobject_repository_name = std::string(repository_name_chars);

    // filename
    } else if (user_data.under_filename) {
      char filename_chars[len+1];
      strncpy(filename_chars, (const char *)characters, len);
      filename_chars[len] = (char)NULL;
      user_data.fileobject_filename = std::string(filename_chars);

    // filesize
    } else if (user_data.under_filesize) {
      char filesize_chars[len+1];
      strncpy(filesize_chars, (const char *)characters, len);
      filesize_chars[len] = (char)NULL;
      user_data.fileobject_filesize = std::string(filesize_chars);

    // fileobject hashdigest
    } else if (user_data.under_fileobject_hashdigest) {
      char fileobject_hashdigest_chars[len+1];
      strncpy(fileobject_hashdigest_chars, (const char *)characters, len);
      fileobject_hashdigest_chars[len] = (char)NULL;
      user_data.fileobject_hashdigest = std::string(fileobject_hashdigest_chars);

    // byte_run hashdigest
    } else if (user_data.under_byte_run_hashdigest) {
      char byte_run_hashdigest_chars[len+1];
      strncpy(byte_run_hashdigest_chars, (const char *)characters, len);
      byte_run_hashdigest_chars[len] = (char)NULL;
      user_data.byte_run_hashdigest = std::string(byte_run_hashdigest_chars);

    } else {
      // no action for other tag names
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
    printf("Error reading DFXML file: ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "dfxml_hashdigest_reader on_error(): " << msg << std::endl;
  }

  static void on_fatal_error(void* p_user_data ATTRIBUTE_UNUSED,
                             const char* msg,
                             ...) {
    va_list arglist;
    printf("Fatal error reading DFXML file: ");
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
  static void do_read(
                const std::string& dfxml_file,
                const std::string& default_repository_name,
                HC* hash_consumer,
                SC* source_metadata_consumer) {

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
    user_data_t user_data(default_repository_name,
                          hash_consumer, source_metadata_consumer);

    // perform the sax parse on the file
    int sax_parser_resource = xmlSAXUserParseFile(
                              &sax_handlers, &user_data, dfxml_file.c_str());

    // clean up libxml2 malloc resources to help memory leak analysis using
    // tools such as valgrind.  Call only in non-multithreaded environment.
    xmlCleanupParser();

    if (sax_parser_resource == 0) {
      // good, no failure
      return;
    } else {
      // something went wrong
      std::ostringstream ss3;
      ss3 << "malformed DFXML file '" << dfxml_file << "'";
      throw std::runtime_error(ss3.str());
    }
  }
};
#endif

