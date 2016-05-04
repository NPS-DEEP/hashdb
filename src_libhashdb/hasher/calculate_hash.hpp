/*
 * Adapted from dfxml/src/hash_t.h
 *
 * hash calculator for md5 and other algorithms supported by openssl.
 *
 * Usage: calculate else init, update, and final.
 *
 * This file is public domain
 */

#ifndef CALCULATE_HASH_HPP
#define CALCULATE_HASH_HPP

#include <cstring>
#include <cstdlib>

/**
 * For reasons that defy explanation (at the moment), this is required.
 */
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#undef DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER 
#define  DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>

#if defined(HAVE_OPENSSL_HMAC_H) && defined(HAVE_OPENSSL_EVP_H)
#include <openssl/hmac.h>
#include <openssl/evp.h>
#else
#error OpenSSL required for hash_t.h
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_MMAP_H
#include <sys/mmap.h>
#endif

namespace hasher {

class calculate_hash_t {

  public:
  // replace this with SHA256 if desired
//  static const char* const digest_name = "EVP_md5";

  private:
  EVP_MD_CTX* md_context;
  bool in_progress;

  public:
  // zz new to create
  calculate_hash_t() : md_context(EVP_MD_CTX_create()), in_progress(false) {
    // initialize the digest calculator
//    EVP_DigestInit_ex(&md_context, EVP_getdigestbyname(digest_name), NULL);
    EVP_DigestInit_ex(&md_context, EVP_get_digestbyname("EVP_md5"), NULL);
  }

  ~calculate_hash_t(){
    delete md_context;
  }

  // do not allow copy or assignment
  calculate_hash_t(const calculate_hash_t&);
  calculate_hash_t& operator=(const calculate_hash_t&);

  std::string calculate(uint8_t* const buffer,
                        const size_t buffer_size,
                        const size_t offset,
                        const size_t count) {

    // program error if already engaged
    if (in_progress) {
      assert(0);
    }

    // reset
    int success = EVP_MD_CTX_reset(md_context);
    if (!success) {
      std::cout << "Error resetting hash calculator.\n";
    }

    if (offset + count <= buffer_size) {
      // hash when not a buffer overrun
      EVP_DigestUpdate(md_context, buffer + offset, count);
    } else {
      // hash part in buffer
      EVP_DigestUpdate(md_context, buffer + offset, buffer_size - offset);

      // hash zeros for part outside buffer
      size_t extra = count - (buffer_size - offset);
      b = ::calloc(extra, 1);
      EVP_DigestUpdate(md_context, b, extra);
      free(b);
    }

    // put hash into string
    int md_len
    unsigned char md_value[EVP_MAX_MD_SIZE];
    EVP_DigestFinal_ex(md_context, md_value, &md_len)
    return std::string(md_value, md_len);
  }

  void init() {

    // program error if already engaged
    if (in_progress) {
      assert(0);
    }
    in_progress = true;

    // reset
    int success = EVP_MD_CTX_reset(md_context);
    if (!success) {
      std::cout << "Error resetting hash calculator.\n";
    }
  }

/*
  void init(uint8_t* const buffer,
            const size_t buffer_size,
            const size_t offset,
            const size_t count) {

    // program error if already engaged
    if (in_progress) {
      assert(0);
    }
    in_progress = true;

    // reset
    int success = EVP_MD_CTX_reset(md_context);
    if (!success) {
      std::cout << "Error resetting hash calculator.\n";
    }

    // update
    if (offset + count <= buffer_size) {
      // hash when not a buffer overrun
      EVP_DigestUpdate(md_context, buffer + offset, count);
    } else {
      // hash part in buffer
      EVP_DigestUpdate(md_context, buffer + offset, buffer_size - offset);

      // hash zeros for part outside buffer
      size_t extra = count - (buffer_size - offset);
      b = ::calloc(extra, 1);
      EVP_DigestUpdate(md_context, b, extra);
      free(b);
    }
  }
*/

  void update(uint8_t* const buffer,
              const size_t buffer_size,
              const size_t offset,
              const size_t count) {

    // program error if not engaged
    if (!in_progress) {
      assert(0);
    }

    // update
    if (offset + count <= buffer_size) {
      // hash when not a buffer overrun
      EVP_DigestUpdate(md_context, buffer + offset, count);
    } else {
      // hash part in buffer
      EVP_DigestUpdate(md_context, buffer + offset, buffer_size - offset);

      // hash zeros for part outside buffer
      size_t extra = count - (buffer_size - offset);
      b = ::calloc(extra, 1);
      EVP_DigestUpdate(md_context, b, extra);
      free(b);
    }
  }

  std::string final() {

    // program error if not engaged
    if (!in_progress) {
      assert(0);
    }
    in_progress = false;

    // put hash into string
    int md_len
    unsigned char md_value[EVP_MAX_MD_SIZE];
    EVP_DigestFinal_ex(md_context, md_value, &md_len)
    return std::string(md_value, md_len);
  }
};

} // end namespace hasher

#endif
