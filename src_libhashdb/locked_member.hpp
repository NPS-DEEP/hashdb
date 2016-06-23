// Author:  Bruce Allen
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
 * Provide a threadsafe interface for checking membership.
 */

#ifndef LOCKED_MEMBER_HPP
#define LOCKED_MEMBER_HPP

#include <string>
#include <set>

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

class locked_member_t {

  private:
  typedef std::set<std::string> set_t;
  set_t member;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  locked_member_t(const locked_member_t&);
  locked_member_t& operator=(const locked_member_t&);

  public:
  locked_member_t() : member(), M() {
    MUTEX_INIT(&M);
  }

  ~locked_member_t() {
    MUTEX_DESTROY(&M);
  }

  // return true if new else false
  bool locked_insert(const std::string& item) {
    MUTEX_LOCK(&M);
    std::pair<set_t::const_iterator, bool> pair = member.insert(item);
    bool did_insert = pair.second;
    MUTEX_UNLOCK(&M);
    return did_insert;
  }
};

} // end namespace hashdb

#endif

