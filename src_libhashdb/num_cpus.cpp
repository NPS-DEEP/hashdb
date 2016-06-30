/**
 * \file
 * Return number of system cores.
 */

#include <iostream>
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

#include <sys/types.h>
#include <unistd.h>

namespace hashdb {

/* Return the number of CPUs we have on various architectures.
 * From http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 */
int numCPU()
{

#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    int numCPU = sysinfo.dwNumberOfProcessors;

#elif defined _SC_NPROCESSORS_ONLN   // linux, unistd.h
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(HW_AVAILCPU) && defined(HW_NCPU) // others, not tested.

    int numCPU=1;			// default
    int mib[4];
    size_t len=sizeof(numCPU);

    // set the mib for hw.ncpu
    memset(mib,0,sizeof(mib));
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    // get the number of CPUs from the system
    if(sysctl(mib, 2, &numCPU, &len, NULL, 0)){
	perror("sysctl");
    }

    if( numCPU <= 1 ) {
	mib[1] = HW_NCPU;
	sysctl( mib, 2, &numCPU, &len, NULL, 0 );
	if( numCPU < 1 ) {
	    numCPU = 1;
	}
    }

#else
#error architecture not identified
#endif

    return numCPU;
}
} // end namespace hashdb

