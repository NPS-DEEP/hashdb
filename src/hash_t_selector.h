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
 * Configures hash type hash_t based on the hash type macro.
 */

#ifndef  HASH_T_SELECTOR_H
#define  HASH_T_SELECTOR_H

#include <dfxml/src/hash_t.h>

#ifdef USE_HASH_TYPE_MD5
typedef md5_t hash_t;
#elif USE_HASH_TYPE_SHA1
typedef sha1_t hash_t;
#elif USE_HASH_TYPE_SHA256
typedef sha256_t hash_t;
#elif USE_HASH_TYPE_SHA512
typedef sha512_t hash_t;

#elif USE_HASH_TYPE_STRAIGHT16
typedef hash__<EVP_md_null,16> hash_t;
template<typename T>
inline std::string digest_name();
template<>
inline std::string digest_name<hash_t>() {
    return "STRAIGHT16";
}

#elif USE_HASH_TYPE_STRAIGHT64
typedef hash__<EVP_md_null,64> hash_t;
template<typename T>
inline std::string digest_name();
template<>
inline std::string digest_name<hash_t>() {
    return "STRAIGHT64";
}

#else
#error A valid hash type macro is required
#endif

#endif
