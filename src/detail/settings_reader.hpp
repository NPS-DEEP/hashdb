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
 * Provides the service of reading settings used by the hashdb.
 *
 * Throws parse_error_t if the settings xml file is invalid.
 */

/**
 * NOTE: libxml2 uses type xmlChar which is unsigned char, which is
 * incompatible with string conversions.
 * All data in settings are and must remain simple ascii text.
 * This stiplulation is easy to change, see dfxml_hashdigest_reader.hpp.
 */

#ifndef SETTINGS_READER_HPP
#define SETTINGS_READER_HPP
#ifdef WIN32
  #include "io.h"
#endif
#include <libxml/parser.h>
#include "hashdb_types.h"
#include "hashdb_settings.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

/**
 * \file
 * This file reads hashdb settings.
 */

namespace hashdb_settings {

class parse_error_t: public std::runtime_error {
public:
    final std::string text;
    parse_error_t(std::string p_text) : text(p_text) {
    }
    virtual const char *what() const throw() {
        return text;
    }
};


// a class is used just to keep members private
class settings_reader_t {

  private:

  // do not allow these
  settings_reader_t();
  settings_reader_t(const settings_reader_t&);
  settings_reader_t& operator=(const settings_reader_t&);

  // ************************************************************
  // nodes
  // ************************************************************
  // parent nodes
  enum parent_node_type_t {NO_PARENT_NODE,
                           HASH_STORE_SETTINGS,
                           HASH_DUPLICATES_STORE_SETTINGS,
                           SOURCE_LOOKUP_SETTINGS,
                           BLOOM_FILTER_SETTINGS};

  // nodes
  // NOTE: use REGULAR_MAP_TYPE instead of MAP_TYPE because of a typedef
  // conflict, likely in usr/include/asm-generic/mman-common.h
  // or usr/include/bits/mman.h
  enum node_type_t {NO_NODE,
                    // hashdb
                    HASHDB_VERSION,
                    HASH_BLOCK_SIZE,
                    HASHDIGEST_TYPE,
                    MAXIMUM_HASH_DUPLICATES,
                    // hash store, hash duplicates store
                    REGULAR_MAP_TYPE,
                    DUPLICATES_MAP_TYPE,
                    SHARD_COUNT,
                    // source lookup settings
                    NUMBER_OF_INDEX_BITS_TYPE,
                    MULTI_INDEX_CONTAINER_TYPE,
                    // bloom filters
                    STATUS,
                    K_HASH_FUNCTIONS,
                    M_HASH_SIZE};

  // ************************************************************
  // user data type for sax
  // ************************************************************
  struct user_data_t {

    // parser state
    hashdb_settings_t* settings;
    parent_node_type_t active_parent_node;
    node_type_t active_node;
    size_t index;

    user_data_t(hashdb_settings_t* p_settings) : settings(p_settings),
                    active_parent_node(NO_PARENT_NODE),
                    active_node(NO_NODE),
                    index(0) {
    }

    // don't allow these
    private:
    user_data_t(const user_data_t&);
    user_data_t& operator=(const user_data_t&);
  };

  // ************************************************************
  // static sax handler helper functions
  // ************************************************************
  // xmlChar to string
  static void xmlChar_to_string(const xmlChar* characters, int len, std::string& s) {
    char temp_chars[len+1];
    strncpy(temp_chars, (const char*)characters, len);
    temp_chars[len] = (char)NULL;
    s = std::string(temp_chars);
  }

  // convert node name to parent node type
  static parent_node_type_t xmlChar_to_parent_node_type(const xmlChar* name) {
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hash_store_settings"))) return HASH_STORE_SETTINGS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hash_duplicates_store_settings"))) return HASH_DUPLICATES_STORE_SETTINGS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("source_lookup_settings"))) return SOURCE_LOOKUP_SETTINGS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("bloom_filter_settings"))) return BLOOM_FILTER_SETTINGS;
    return NO_PARENT_NODE;
  }

  // convert node name to node type
  static node_type_t xmlChar_to_node_type(const xmlChar* name) {
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hashdb_version"))) return HASHDB_VERSION;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hash_block_size"))) return HASH_BLOCK_SIZE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hashdigest_type"))) return HASHDIGEST_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("maximum_hash_duplicates"))) return MAXIMUM_HASH_DUPLICATES;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("map_type"))) return REGULAR_MAP_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("duplicates_map_type"))) return DUPLICATES_MAP_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("shard_count"))) return SHARD_COUNT;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("number_of_index_bits_type"))) return NUMBER_OF_INDEX_BITS_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("multi_index_container_type"))) return MULTI_INDEX_CONTAINER_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("status"))) return STATUS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("k_hash_functions"))) return K_HASH_FUNCTIONS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("M_hash_size"))) return M_HASH_SIZE;
    return NO_NODE;
  }

  // parse a mandatory bloom filter index value
  static size_t parse_bloom_filter_index(const xmlChar** attributes) {

    if (attributes == NULL) {
      // no attributes
      exit_invalid_state("the tag requires an 'index' attribute but no attributes were provided");
    }

    // parse attributes
    int i=0;
    while (true) {
      if (attributes[i] == NULL || attributes[i+1] == NULL) {
      exit_invalid_state("the tag requires an 'index' attribute but it was not provided");
      }

      if (xmlStrEqual(attributes[i], reinterpret_cast<const xmlChar*>("index"))) {

        std::string index_string((const char*)attributes[i+1]);
        try {
          size_t index = boost::lexical_cast<size_t>(index_string);
          return index;
        } catch(...) {
          // abort
          std::ostringstream s;
          s << "settings_reader_t(): invalid index value: '"
            << index_string << "'\n" << "Cannot continue.\n";
          throw hashdb_settings::parse_error_t(s.str());
        }
      }

      i += 2;
    }
  }

  // convert string to number or fail with exit
  template <class T>
  static void xmlChar_to_number(const xmlChar* c, int len, T& number) {
    std::string number_string;
    xmlChar_to_string(c, len, number_string);
    try {
      number = boost::lexical_cast<T>(number_string);
      return;
    } catch(...) {
      // abort
      std::ostringstream s;
      s << "settings_reader_t(): invalid number: '"
        << number_string << "'\n" << "Cannot continue.\n";
      throw parse_error_t(s.str());
    }
  }

  static void exit_invalid_state(std::string message) {
    std::ostringstream s;
    s << "Error: " << message << "\n" << "Cannot continue.\n";
    throw parse_error_t(s.str());
  }

  static void exit_invalid_text(std::string message, std::string text) {
    std::ostringstream s;
    s << "Error: " << message << ":'" << text << "'\n" << "Cannot continue.\n";
    throw parse_error_t(s.str());
  }

  static void exit_invalid_index(size_t index) {
    std::ostringstream s;
    s << "Error: invalid bloom filter index "
      << index << "\n" << "Cannot continue.\n";
    throw parse_error_t(s.str());
  }

  // ************************************************************
  // static sax handlers
  // ************************************************************
  // these on_* methods implement SAX parsing
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

    // identify active node type
    user_data.active_node = xmlChar_to_node_type(name);

    // identify active parent node type
    parent_node_type_t temp = xmlChar_to_parent_node_type(name);
    if (temp != NO_PARENT_NODE) {
      user_data.active_parent_node = temp;
    }

    // parse any relavent attributes for this node
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("bloom_filter_settings"))) {
      user_data.index = parse_bloom_filter_index(attributes);
    }
  }

  static void on_end_element(void* p_user_data,
                             const xmlChar* name) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    user_data.active_node = NO_NODE;
    if (xmlChar_to_parent_node_type(name) != NO_PARENT_NODE) {
      // close parent node type too
      user_data.active_parent_node = NO_PARENT_NODE;
    }
  }

  static void on_characters(void* p_user_data,
                            const xmlChar* characters,
                            int len) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    bool is_valid;
    switch(user_data.active_parent_node) {
      case NO_PARENT_NODE:
        if (user_data.active_node == HASHDB_VERSION) {
          xmlChar_to_number(characters, len, user_data->settings.hashdb_version);
        } else if (user_data.active_node == HASH_BLOCK_SIZE) {
          xmlChar_to_number(characters, len, user_data->settings.hash_block_size);
        } else if (user_data.active_node == HASHDIGEST_TYPE) {
          // get hashdigest type
          std::string hashdigest_type_string;
          xmlChar_to_string(characters, len, hashdigest_type_string);
          is_valid = string_to_hashdigest_type(hashdigest_type_string, user_data->settings.hashdigest_type);
          if (!is_valid) {
            exit_invalid_text("invalid hashdigest type", hashdigest_type_string);
          }
        } else if (user_data.active_node == MAXIMUM_HASH_DUPLICATES) {
          xmlChar_to_number(characters, len, user_data->settings.maximum_hash_duplicates);
        }
        break;

      case HASH_STORE_SETTINGS:
        if (user_data.active_node == REGULAR_MAP_TYPE) {
          // get map type
          std::string map_type_string;
          xmlChar_to_string(characters, len, map_type_string);
          is_valid = string_to_map_type(map_type_string, user_data->settings.hash_store_settings.map_type);
          if (!is_valid) {
            exit_invalid_text("invalid hash store map type", map_type_string);
          }
        } else if (user_data.active_node == SHARD_COUNT) {
          xmlChar_to_number(characters, len, user_data->settings.hash_store_settings.shard_count);
        }
        break;

      case HASH_DUPLICATES_STORE_SETTINGS:
        if (user_data.active_node == DUPLICATES_MAP_TYPE) {
          // get duplicates map type
          std::string duplicates_map_type_string;
          xmlChar_to_string(characters, len, duplicates_map_type_string);
          is_valid = string_to_multimap_type(duplicates_map_type_string, user_data->settings.hash_duplicates_store_settings.multimap_type);
          if (!is_valid) {
            exit_invalid_text("invalid hash duplicates store", duplicates_map_type_string);
          }
        } else if (user_data.active_node == SHARD_COUNT) {
          xmlChar_to_number(characters, len, user_data->settings.hash_duplicates_store_settings.shard_count);
        }
        break;

      case SOURCE_LOOKUP_SETTINGS:
        if (user_data.active_node == NUMBER_OF_INDEX_BITS_TYPE) {
          // get number of index bits type
          std::string number_of_index_bits_type_string;
          xmlChar_to_string(characters, len, number_of_index_bits_type_string);
          is_valid = string_to_number_of_index_bits_type(number_of_index_bits_type_string, user_data-->settings.source_lookup_settings.number_of_index_bits_type);
          if (!is_valid) {
            exit_invalid_text("invalid source lookup record type", number_of_index_bits_type_string);
          }
        } else if (user_data.active_node == MULTI_INDEX_CONTAINER_TYPE) {
          // get multi_index container type
          std::string multi_index_container_type_string;
          xmlChar_to_string(characters, len, multi_index_container_type_string);
          is_valid = string_to_multi_index_container_type(multi_index_container_type_string , user_data->settings.source_lookup_settings.multi_index_container_type);
          if (!is_valid) {
            exit_invalid_text("invalid source lookup multi index container type", multi_index_container_type_string);
          }
        }
        break;

      case BLOOM_FILTER_SETTINGS:
        if (user_data.index != 1 && user_data.index != 2) {
          // bad
          exit_invalid_index(user_data.index);
        }
        if (user_data.active_node == STATUS) {
          if (user_data.index == 1) {
            std::string bloom1_state_string;
            xmlChar_to_string(characters, len, bloom1_state_string);
            is_valid = string_to_bloom_state(bloom1_state_string, user_data->settings.bloom1_settings.is_used);
            if (!is_valid) {
              exit_invalid_state("Error: invalid bloom 1 state\n";
            }
          } else if (user_data.index == 2) {
            std::string bloom2_state_string;
            xmlChar_to_string(characters, len, bloom2_state_string);
            is_valid = string_to_bloom_state(bloom2_state_string, user_data->settings.bloom2_settings.is_used);
            if (!is_valid) {
              exit_invalid_state("Error: invalid bloom 2 state\n";
            }
          } else {
            exit_invalid_index(user_data.index);
          }
        } else if (user_data.active_node == K_HASH_FUNCTIONS) {
          uint32_t k;
          xmlChar_to_number(characters, len, k);
          if (user_data.index == 1) {
            user_data->settings.bloom1_settings.k_hash_functions = k;
          } else if (user_data.index == 2) {
            user_data->settings.bloom2_settings.k_hash_functions = k;
          } else {
            exit_invalid_index(user_data.index);
          }
        } else if (user_data.active_node == M_HASH_SIZE) {
          uint32_t M;
          xmlChar_to_number(characters, len, M);
          if (user_data.index == 1) {
            user_data->settings.bloom1_settings.M_hash_size = M;
          } else if (user_data.index == 2) {
            user_data->settings.bloom2_settings.M_hash_size = M;
          } else {
            exit_invalid_index(user_data.index);
          }
        }
        break;
    }
  }

  static void on_comment(void* p_user_data ATTRIBUTE_UNUSED) {
  //    std::cout << "settings_reader_t::on_comment " << text << "\n";
  }

  static void on_warning(void* p_user_data ATTRIBUTE_UNUSED,
                         const char* msg,
                         ...) {
    // adapted from http://stackoverflow.com/questions/5977326/call-printf-using-va-list
    va_list arglist;
    printf("settings_reader on_warning(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_warning(): " << msg << std::endl;

  }

  static void on_error(void* p_user_data ATTRIBUTE_UNUSED,
                       const char* msg,
                       ...) {
    va_list arglist;
    printf("settings_reader on_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_error(): " << msg << std::endl;
  }

  static void on_fatal_error(void* p_user_data ATTRIBUTE_UNUSED,
                             const char* msg,
                             ...) {
    va_list arglist;
    printf("settings_reader on_fatal_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_fatal_error(): " << msg << std::endl;
  }

  public:
  /**
   * read onto default hashdb settings or throw parse_error_t.
   */
  static void read_settings(const std::string hashdb_dir,
                            hashdb_settings_t& settings) {

    // avoid obvious xml trouble by verifying that hashdb_dir exists
    bool dir_is_present = (access(hashdb_dir.c_str(),F_OK) == 0);
    if (!dir_is_present) {
      std::ostringstream s;
      s << "Error:\nHash database directory '"
        << hashdb_dir << "' does not exist.\n"
        << "Is the path to the hash database correct?\n"
        << "Cannot continue.\n";
      throw parse_error_t(s.str());
    }

    // also make sure hashdb_dir is a directory
    struct stat s;
    bool is_dir = false;
    if (stat(hashdb_dir.c_str(), &s) == 0) {
      if (s.st_mode & S_IFDIR) {
        // good
        is_dir = true;
      }
    }
    if (!is_dir) {
      std::ostringstream s;
      s << "Error:\nHash database directory '"
        << hashdb_dir << "' is not a directory.\n"
        << "Is the path to the hash database correct?\n"
        << "Cannot continue.\n";
      throw parse_error_t(s.str());
    }

    // look up the settings filename
    std::string filename(hashdb_filenames_t::settings_filename(hashdb_dir));

    // also verify that the settings file exists
    bool file_is_present = (access(filename.c_str(),F_OK) == 0);
    if (!file_is_present) {
      std::ostringstream s;
      s << "Error:\nSettings file '"
        << filename << "' does not exist.\n"
        << "Is the path to the hash database correct?\n"
        << "Cannot continue.\n";
      throw parse_error_t(s.str());
    }

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
    user_data_t user_data(&settings);

    // perform the sax parse on the file
    int sax_parser_resource = xmlSAXUserParseFile(
                               &sax_handlers, &user_data, filename.c_str());
    if (sax_parser_resource == 0) {
      // good, no failure
      return;
    } else {
      // something went wrong
      std::ostringstream s;
      s << "malformed settings in file '" << filename << "'.  Unable to continue.\n";
      throw parse_error_t(s.str());
    }
  }
};

} // namespace hashdb_settings

#endif

