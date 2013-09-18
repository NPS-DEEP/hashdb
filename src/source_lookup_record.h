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
 * Provides a source location record and accessors for it.
 * This record optimizes for space by allowing more bits for the
 * source lookup index and less bits for the hash block offset value,
 * while still consuming 64 bits total.
 */

#ifndef SOURCE_LOOKUP_RECORD_H
#define SOURCE_LOOKUP_RECORD_H
#include <stdint.h>
#include <cassert>
#include <string>
#include <iostream>
#include <limits>
#include <dfxml/src/hash_t.h>
#include "hashdb_types.h" // for number_of_index_bits_type_t

// note optimizations:
// 1) number_of_index_bits_type_t is used to pack source lookup index
//    and hash block offset value fields.
// 2) count information is encoded in first uint32_t when second uint32_t is max

// ************************************************************
// number of index bits type for packing source lookup record
// ************************************************************
// source lookup record index bits types
enum number_of_index_bits_type_t {NUMBER_OF_INDEX_BITS32,      // 32, 32
                                  NUMBER_OF_INDEX_BITS33,      // 33, 31
                                  NUMBER_OF_INDEX_BITS34,      // 34, 30
                                  NUMBER_OF_INDEX_BITS35,      // 35, 29
                                  NUMBER_OF_INDEX_BITS36,      // 36, 28
                                  NUMBER_OF_INDEX_BITS37,      // 37, 27
                                  NUMBER_OF_INDEX_BITS38,      // 38, 26
                                  NUMBER_OF_INDEX_BITS39,      // 39, 25
                                  NUMBER_OF_INDEX_BITS40};     // 40, 24

inline std::string number_of_index_bits_type_to_string(number_of_index_bits_type_t type) {
  switch(type) {
    case NUMBER_OF_INDEX_BITS32: return "32";
    case NUMBER_OF_INDEX_BITS33: return "33";
    case NUMBER_OF_INDEX_BITS34: return "34";
    case NUMBER_OF_INDEX_BITS35: return "35";
    case NUMBER_OF_INDEX_BITS36: return "36";
    case NUMBER_OF_INDEX_BITS37: return "37";
    case NUMBER_OF_INDEX_BITS38: return "38";
    case NUMBER_OF_INDEX_BITS39: return "39";
    case NUMBER_OF_INDEX_BITS40: return "40";
    default: assert(0); return "";
  }
}

inline bool string_to_number_of_index_bits_type(const std::string& name, number_of_index_bits_type_t& type) {
  if (name == "32") { type = NUMBER_OF_INDEX_BITS32; return true; }
  if (name == "33") { type = NUMBER_OF_INDEX_BITS33; return true; }
  if (name == "34") { type = NUMBER_OF_INDEX_BITS34; return true; }
  if (name == "35") { type = NUMBER_OF_INDEX_BITS35; return true; }
  if (name == "36") { type = NUMBER_OF_INDEX_BITS36; return true; }
  if (name == "37") { type = NUMBER_OF_INDEX_BITS37; return true; }
  if (name == "38") { type = NUMBER_OF_INDEX_BITS38; return true; }
  if (name == "39") { type = NUMBER_OF_INDEX_BITS39; return true; }
  if (name == "40") { type = NUMBER_OF_INDEX_BITS40; return true; }
  type = NUMBER_OF_INDEX_BITS32;
  return false;
}

inline std::ostream& operator<<(std::ostream& os,
            const number_of_index_bits_type_t& t) {
  os <<  number_of_index_bits_type_to_string(t);
  return os;
}

class source_lookup_record_t {
  private:
    // the basic composite value for a source_lookup_record_t
    uint64_t composite_value;

    struct slide32_t {
      uint64_t source_lookup_index:32;
      uint64_t hash_block_offset_value:32;
    };
    struct slide33_t {
      uint64_t source_lookup_index:33;
      uint64_t hash_block_offset_value:31;
    };
    struct slide34_t {
      uint64_t source_lookup_index:34;
      uint64_t hash_block_offset_value:30;
    };
    struct slide35_t {
      uint64_t source_lookup_index:35;
      uint64_t hash_block_offset_value:29;
    };
    struct slide36_t {
      uint64_t source_lookup_index:36;
      uint64_t hash_block_offset_value:28;
    };
    struct slide37_t {
      uint64_t source_lookup_index:37;
      uint64_t hash_block_offset_value:27;
    };
    struct slide38_t {
      uint64_t source_lookup_index:38;
      uint64_t hash_block_offset_value:26;
    };
    struct slide39_t {
      uint64_t source_lookup_index:39;
      uint64_t hash_block_offset_value:25;
    };
    struct slide40_t {
      uint64_t source_lookup_index:40;
      uint64_t hash_block_offset_value:24;
    };
    struct counter_t {
      uint64_t count:32;
      uint64_t overflow_indicator:32;
    };

    void validate_source_lookup_index(
                     number_of_index_bits_type_t number_of_index_bits_type,
                     uint64_t index) {

      bool is_valid = false;
      switch(number_of_index_bits_type) {
        case NUMBER_OF_INDEX_BITS32: is_valid = index < (uint64_t)1<<32; break;
        case NUMBER_OF_INDEX_BITS33: is_valid = index < (uint64_t)1<<33; break;
        case NUMBER_OF_INDEX_BITS34: is_valid = index < (uint64_t)1<<34; break;
        case NUMBER_OF_INDEX_BITS35: is_valid = index < (uint64_t)1<<35; break;
        case NUMBER_OF_INDEX_BITS36: is_valid = index < (uint64_t)1<<36; break;
        case NUMBER_OF_INDEX_BITS37: is_valid = index < (uint64_t)1<<37; break;
        case NUMBER_OF_INDEX_BITS38: is_valid = index < (uint64_t)1<<38; break;
        case NUMBER_OF_INDEX_BITS39: is_valid = index < (uint64_t)1<<39; break;
        case NUMBER_OF_INDEX_BITS40: is_valid = index < (uint64_t)1<<40; break;
      }
      if (!is_valid) {
        std::cerr << "Error: The source lookup index has become too big for the current number of index bits type " << number_of_index_bits_type << ".\n";
        std::cerr << "No more source lookup records can be allocated at this setting.\n";
        std::cerr << "Use a number of index bits type with a higher capacity.\n";
        std::cerr << "Cannot continue.  Aborting.\n";
        exit(1);
      }
    }

    void validate_hash_block_offset_value(
                     number_of_index_bits_type_t number_of_index_bits_type,
                     uint64_t offset) {
      bool is_valid = false;
      switch(number_of_index_bits_type) {
        case NUMBER_OF_INDEX_BITS32: is_valid = offset < (uint64_t)1<<32; break;
        case NUMBER_OF_INDEX_BITS33: is_valid = offset < (uint64_t)1<<31; break;
        case NUMBER_OF_INDEX_BITS34: is_valid = offset < (uint64_t)1<<30; break;
        case NUMBER_OF_INDEX_BITS35: is_valid = offset < (uint64_t)1<<29; break;
        case NUMBER_OF_INDEX_BITS36: is_valid = offset < (uint64_t)1<<28; break;
        case NUMBER_OF_INDEX_BITS37: is_valid = offset < (uint64_t)1<<27; break;
        case NUMBER_OF_INDEX_BITS38: is_valid = offset < (uint64_t)1<<26; break;
        case NUMBER_OF_INDEX_BITS39: is_valid = offset < (uint64_t)1<<25; break;
        case NUMBER_OF_INDEX_BITS40: is_valid = offset < (uint64_t)1<<24; break;
      }
      if (!is_valid) {
        std::cerr << "Error: The hash block offset value is too large for the current source lookup record type " << number_of_index_bits_type << ".\n";
        std::cerr << "Use a number of index bits type with a lower capacity\n";
        std::cerr << "in order to index higher hash block offset values.\n";
        std::cerr << "Cannot continue.  Aborting.\n";
        exit(1);
      }
    }

  public:

    /**
     * Construct with no parameters because std::pair needs it?
     */
    source_lookup_record_t() : composite_value(0UL) {
    }

    /**
     * Construct from source lookup index and hash block offset value.
     */
    source_lookup_record_t(
            number_of_index_bits_type_t number_of_index_bits_type,
            uint64_t _source_lookup_index,
            uint64_t _hash_block_offset_value) :
                  // The composite_value cannot be set from this constructor
                  composite_value(0UL) {

      // validate the inputs
      validate_source_lookup_index(number_of_index_bits_type, _source_lookup_index);
      validate_hash_block_offset_value(number_of_index_bits_type, _hash_block_offset_value);

      // now accept the input values
      switch(number_of_index_bits_type) {
        case NUMBER_OF_INDEX_BITS32: {
          slide32_t* slide32 = reinterpret_cast<slide32_t*>(this);
          slide32->source_lookup_index = _source_lookup_index;
          slide32->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS33: {
          slide33_t* slide33 = reinterpret_cast<slide33_t*>(this);
          slide33->source_lookup_index = _source_lookup_index;
          slide33->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS34: {
          slide34_t* slide34 = reinterpret_cast<slide34_t*>(this);
          slide34->source_lookup_index = _source_lookup_index;
          slide34->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS35: {
          slide35_t* slide35 = reinterpret_cast<slide35_t*>(this);
          slide35->source_lookup_index = _source_lookup_index;
          slide35->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS36: {
          slide36_t* slide36 = reinterpret_cast<slide36_t*>(this);
          slide36->source_lookup_index = _source_lookup_index;
          slide36->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS37: {
          slide37_t* slide37 = reinterpret_cast<slide37_t*>(this);
          slide37->source_lookup_index = _source_lookup_index;
          slide37->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS38: {
          slide38_t* slide38 = reinterpret_cast<slide38_t*>(this);
          slide38->source_lookup_index = _source_lookup_index;
          slide38->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS39: {
          slide39_t* slide39 = reinterpret_cast<slide39_t*>(this);
          slide39->source_lookup_index = _source_lookup_index;
          slide39->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        case NUMBER_OF_INDEX_BITS40: {
          slide40_t* slide40 = reinterpret_cast<slide40_t*>(this);
          slide40->source_lookup_index = _source_lookup_index;
          slide40->hash_block_offset_value = _hash_block_offset_value;
          break;
        }
        default: {
          assert(0);
          break;
        }
      }
    }

    /**
     * Construct with count value, where count >= 2 and count < max.
     */
    source_lookup_record_t(uint32_t count) : composite_value(0UL) {
      counter_t* counter = reinterpret_cast<counter_t*>(this);
      // invalid usage unless 2 <= count < max
      if (count < 2 || count == std::numeric_limits<uint32_t>::max()) {
        assert(0);
      }

      counter->count = count;
      counter->overflow_indicator = std::numeric_limits<uint32_t>::max();
    }

    size_t get_count() {
      counter_t* counter = reinterpret_cast<counter_t*>(this);
      if (counter->overflow_indicator != std::numeric_limits<uint32_t>::max()) {
        // the data structure is using source_lookup_index
        // and hash_block_offset_value rather than count
        return 1;
      } else {
        // the data structure is using overflow_indicator and count
        return counter->count;
      }
    }

    /**
     * Change the value of the source lookup record.
     */
    source_lookup_record_t & operator=(const source_lookup_record_t &theirs) {
      this->composite_value = theirs.composite_value;
      return *this;
    }

    /**
     * Get the value of the source lookup index
     * given the number of index bits type.
     */
    uint64_t source_lookup_index(
                 number_of_index_bits_type_t number_of_index_bits_type) const {
      switch(number_of_index_bits_type) {
        case NUMBER_OF_INDEX_BITS32: {
          slide32_t const* slide32 = reinterpret_cast<slide32_t const*>(this);
          return slide32->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS33: {
          slide33_t const* slide33 = reinterpret_cast<slide33_t const*>(this);
          return slide33->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS34: {
          slide34_t const* slide34 = reinterpret_cast<slide34_t const*>(this);
          return slide34->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS35: {
          slide35_t const* slide35 = reinterpret_cast<slide35_t const*>(this);
          return slide35->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS36: {
          slide36_t const* slide36 = reinterpret_cast<slide36_t const*>(this);
          return slide36->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS37: {
          slide37_t const* slide37 = reinterpret_cast<slide37_t const*>(this);
          return slide37->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS38: {
          slide38_t const* slide38 = reinterpret_cast<slide38_t const*>(this);
          return slide38->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS39: {
          slide39_t const* slide39 = reinterpret_cast<slide39_t const*>(this);
          return slide39->source_lookup_index;
        }
        case NUMBER_OF_INDEX_BITS40: {
          slide40_t const* slide40 = reinterpret_cast<slide40_t const*>(this);
          return slide40->source_lookup_index;
        }
      }
      assert(0); return 0;
    }

    /**
     * Get the hash block offset value given the number of index bits type.
     */
    uint64_t hash_block_offset_value(
                 number_of_index_bits_type_t number_of_index_bits_type) const {
      switch(number_of_index_bits_type) {
        case NUMBER_OF_INDEX_BITS32: {
          slide32_t const* slide32 = reinterpret_cast<slide32_t const*>(this);
          return slide32->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS33: {
          slide33_t const* slide33 = reinterpret_cast<slide33_t const*>(this);
          return slide33->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS34: {
          slide34_t const* slide34 = reinterpret_cast<slide34_t const*>(this);
          return slide34->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS35: {
          slide35_t const* slide35 = reinterpret_cast<slide35_t const*>(this);
          return slide35->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS36: {
          slide36_t const* slide36 = reinterpret_cast<slide36_t const*>(this);
          return slide36->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS37: {
          slide37_t const* slide37 = reinterpret_cast<slide37_t const*>(this);
          return slide37->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS38: {
          slide38_t const* slide38 = reinterpret_cast<slide38_t const*>(this);
          return slide38->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS39: {
          slide39_t const* slide39 = reinterpret_cast<slide39_t const*>(this);
          return slide39->hash_block_offset_value;
        }
        case NUMBER_OF_INDEX_BITS40: {
          slide40_t const* slide40 = reinterpret_cast<slide40_t const*>(this);
          return slide40->hash_block_offset_value;
        }
      }
      assert(0); return 0;
    }

    bool operator==(const class source_lookup_record_t r) const {
      return ((this)->composite_value == r.composite_value);
    }

    bool operator!=(const class source_lookup_record_t r) const {
      return ((this)->composite_value != r.composite_value);
    }

    uint64_t composite_value_exported_for_testing() const {
      return composite_value;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                         const class source_lookup_record_t& lookup) {
  os << "(source_lookup_record value=0x"
     << std::hex << lookup.composite_value_exported_for_testing() << std::dec << ")";
  return os;
}

// hash store element pair
typedef std::pair<md5_t, source_lookup_record_t> hash_store_element_t;
inline std::ostream& operator<<(std::ostream& os,
                         const hash_store_element_t& hash_store_element) {
  os << "(md5=" << hash_store_element.first.hexdigest()
     << ",source_lookup_record=" << hash_store_element.second << ")";
  return os;
}

#endif

