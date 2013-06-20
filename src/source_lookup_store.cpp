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
 * Provides interfaces to the source lookup store.
 */

#include <config.h>
#include "hashdb_types.h"
#include "source_location_record.hpp"
#include "source_lookup_store.hpp"
#include <string>
#include <cassert>

// map name
const std::string MULTI_INDEX_CONTAINER_NAME = "multi_index_container";

    source_lookup_store_t::source_lookup_store_t (const std::string _filename,
                           file_mode_type_t _file_mode_type,
                           source_lookup_settings_t _source_lookup_settings) :
                                multi_index_container(0),
                                filename(_filename),
                                file_mode_type(_file_mode_type),
                                source_lookup_settings(_source_lookup_settings),
                                multi_index_container_type(source_lookup_settings.multi_index_container_type),
                                next_source_lookup_index(1) {
//for testing, start with high index: next_source_lookup_index((1UL<<40) -1) 
//std::cout << "source_lookup_store_t next index " << next_source_lookup_index << "\n";
//std::cout << "source_lookup_store_t\n";

      // instantiate the map type being used
      switch(multi_index_container_type) {
        case MULTI_INDEX_CONTAINER:
          multi_index_container = new multi_index_container_t(
                MULTI_INDEX_CONTAINER_NAME,
                filename,
                size,
                file_mode_type);

          // set next index higher if in append mode
          if (file_mode_type == RW_MODIFY) {
            next_source_lookup_index = multi_index_container->get_highest_key() + 1L;
          }
        break;
        default:
          assert(0);
      }
    }

    /**
     * Close and release resources.
     */
    source_lookup_store_t::~source_lookup_store_t() {
//std::cout << "~source_lookup_store_t\n";
      switch(multi_index_container_type) {
        case MULTI_INDEX_CONTAINER: {
          delete multi_index_container;
        }
        break;
        default:
        assert(0);
//        std::cout << "invalid configuration\n";
      }
    }

    /**
     * Determine if a source location record exists.
     */
    bool source_lookup_store_t::has_source_location_record(
                    const source_location_record_t& source_location_record) {
      bool has;
      fixed_size_source_location_record_t fixed(source_location_record);

      switch(multi_index_container_type) {
      case MULTI_INDEX_CONTAINER: {
        has = multi_index_container->has_pay(fixed);
        break;
      }
      default:
        has = false; // WIN32 compiler wants this
        assert(0);
      }
//std::cout << "source_lookup_store_t::has_source_location_record: " << fixed << " " << has << "\n";
      return has;
    }

    /**
     * Insert else fail if left or right already exist.
     */
    void source_lookup_store_t::insert_source_lookup_element(
                       uint64_t source_lookup_index,
                       const source_location_record_t& source_location_record) {
//std::cout << "source_lookup_store_t::insert_source_location_record: source_lookup_index=" << source_lookup_index << ",source_location_record=" << source_location_record << "\n";
      switch(multi_index_container_type) {
        case MULTI_INDEX_CONTAINER: {
          multi_index_container->insert_element(
               source_lookup_index,
               fixed_size_source_location_record_t(source_location_record));
        }
        break;
        default:
          assert(0);
      }
    }

    /**
     * Get the source location record from the source lookup index else fail.
     */
    void source_lookup_store_t::get_source_location_record(
                       uint64_t source_lookup_index,
                       source_location_record_t& source_location_record) {
//std::cout << "source_lookup_store_t::get_source_location_record source_lookup_index=" << source_lookup_index << "\n";
      switch(multi_index_container_type) {
      case MULTI_INDEX_CONTAINER: {
        fixed_size_source_location_record_t r;
        multi_index_container->get_pay(source_lookup_index, r);
        source_location_record = r.to_string();
        break;
      }
      default:
        assert(0);
      }
//std::cout << "source_lookup_store_t::get_source_location_record source_location_record=" << source_location_record << "\n";
    }

    /**
     * Get the source lookup index from the source location record else fail.
     */
    void source_lookup_store_t::get_source_lookup_index(
                  const source_location_record_t& source_location_record,
                  uint64_t& source_lookup_index) {
//std::cout << "source_lookup_store_t::get_source_lookup_index source_location_record=" << source_location_record << "\n";

      switch(multi_index_container_type) {
      case MULTI_INDEX_CONTAINER: {
        multi_index_container->get_key(fixed_size_source_location_record_t(source_location_record), source_lookup_index);
        break;
      }
      default:
        assert(0);
      }
//std::cout << "source_lookup_store_t::get_source_lookup_index source_lookup_index=" << source_lookup_index << "\n";
    }
 
    /**
     * Get the next unallocated source lookup index to use.
     */
    uint64_t source_lookup_store_t::new_source_lookup_index() {
//std::cout << "source_lookup_store_t::new_source_lookup_index " << (next_source_lookup_index) << "\n";

      return next_source_lookup_index++;
    }

    /**
     * Report status to consumer.
     */
    template void source_lookup_store_t::report_status(std::ostream&) const;
    template void source_lookup_store_t::report_status(dfxml_writer&) const;
    template <class T>
    void source_lookup_store_t::report_status(T& consumer) const {
      switch(multi_index_container_type) {
        case MULTI_INDEX_CONTAINER:
          multi_index_container->report_status(consumer);
          break;
        default:
          assert(0);
      }
    }


