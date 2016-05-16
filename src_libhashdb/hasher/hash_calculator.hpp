/*
 * Adapted from dfxml/src/hash_t.h
 *
 * hash calculator for md5 and other algorithms supported by openssl.
 *
 * Usage: calculate else init, update, and final.
 *
 * This file is public domain
 */

#ifndef HASH_CALCULATOR_HPP
#define HASH_CALCULATOR_HPP

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

//#include <openssl/hmac.h>
#include <openssl/evp.h>

//#ifdef HAVE_SYS_MMAN_H
//#include <sys/mman.h>
//#endif
//
//#ifdef HAVE_SYS_MMAP_H
//#include <sys/mmap.h>
//#endif

namespace hasher {

class hash_calculator_t {

  private:
  EVP_MD_CTX* const md_context;
  const EVP_MD* md;
  bool in_progress;

  public:
  hash_calculator_t() : md_context(EVP_MD_CTX_create()),
                        md(EVP_md5()),
                        in_progress(false) {
  }

  ~hash_calculator_t(){
    EVP_MD_CTX_destroy(md_context);
  }

  // do not allow copy or assignment
  hash_calculator_t(const hash_calculator_t&);
  hash_calculator_t& operator=(const hash_calculator_t&);

  std::string calculate(const uint8_t* const buffer,
                        const size_t buffer_size,
                        const size_t offset,
                        const size_t count) {

    // program error if already engaged
    if (in_progress) {
      assert(0);
    }

    // reset
    EVP_MD_CTX_init(md_context);
    EVP_DigestInit_ex(md_context, md, NULL);

    if (offset + count <= buffer_size) {
      // hash when not a buffer overrun
      EVP_DigestUpdate(md_context, buffer + offset, count);
    } else {
      // hash part in buffer
      EVP_DigestUpdate(md_context, buffer + offset, buffer_size - offset);

      // hash zeros for part outside buffer
      size_t extra = count - (buffer_size - offset);
      uint8_t* b = new uint8_t[extra]();
      EVP_DigestUpdate(md_context, b, extra);
      delete[] b;
    }

    // put hash into string
    unsigned int md_len;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int success = EVP_DigestFinal(md_context, md_value, &md_len);
    if (success == 0) {
      std::cout << "error calculating hash\n";
      assert(0);
    }
    return std::string(reinterpret_cast<char*>(md_value),
                       static_cast<size_t>(md_len));
  }

  void init() {

    // program error if already engaged
    if (in_progress) {
      assert(0);
    }
    in_progress = true;

    // reset
    EVP_MD_CTX_init(md_context);
    EVP_DigestInit_ex(md_context, md, NULL);
  }

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
//std::cerr << "update.ba.buffer_size " << buffer_size << " offset: " << offset << " count: " << count << "\n";
//std::cerr << "update.bb " << buffer[0] << ", " << buffer[1] << "\n";
      EVP_DigestUpdate(md_context, buffer + offset, count);
    } else {
      // hash part in buffer
      EVP_DigestUpdate(md_context, buffer + offset, buffer_size - offset);

      // hash zeros for part outside buffer
      const size_t extra = count - (buffer_size - offset);
//std::cerr << "buffer_size " << buffer_size << " offset: " << offset << " count: " << count << " extra " << extra << "\n";
      const uint8_t* const b = new uint8_t[extra]();
      EVP_DigestUpdate(md_context, b, extra);
      delete[] b;
    }
  }

  std::string final() {

    // program error if not engaged
    if (!in_progress) {
      assert(0);
    }
    in_progress = false;

    // put hash into string
    unsigned int md_len;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int success = EVP_DigestFinal(md_context, md_value, &md_len);
    if (success == 0) {
      std::cout << "error calculating hash\n";
      assert(0);
    }
    return std::string(reinterpret_cast<char*>(md_value),
                       static_cast<size_t>(md_len));
  }
};

} // end namespace hasher

#endif
