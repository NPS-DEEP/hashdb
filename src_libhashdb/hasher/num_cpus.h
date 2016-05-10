/**
 * \file
 * Return number of system cores.
 */

#ifndef NUM_CPUS_H
#define NUM_CPUS_H

namespace hasher {

// from bulk_extractor/src/threadpool.cpp
/* Return the number of CPUs we have on various architectures.
 * From http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 */
u_int numCPU()
{
    int numCPU=1;			// default
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    numCPU = sysinfo.dwNumberOfProcessors;
#endif
#if defined(HW_AVAILCPU) && defined(HW_NCPU)
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
#endif
#ifdef _SC_NPROCESSORS_ONLN
    numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return numCPU;
}
} // end namespace hasher

#endif
