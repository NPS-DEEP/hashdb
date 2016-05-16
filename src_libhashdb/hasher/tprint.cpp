/**
 * \file
 * Provide threadsafe locked print.
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include <string>
#include <iostream>
#include <cassert>
#include "pthread.h"

namespace hasher {

static pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;

void tprint(const std::string& text) {
  // lock must work
  if(pthread_mutex_lock(&M)) {
    assert(0);
  }
  std::cout << text << std::flush;
  pthread_mutex_unlock(&M);
}

} // end namespace hasher

