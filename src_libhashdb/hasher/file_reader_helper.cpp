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
 * Raw file accessors.  Mostly copied from bulk_extractor/src/image_process.cpp
 *
 * Provides:
 *   pread64() for Windows in global namespace
 *   get_drive_geometry() for Windows
 *   get_filesize_by_filename()
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

#include <cassert>
#include <string>
#include <sstream>
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "filename_t.hpp"
#include "file_reader_helper.hpp"

// if pread64 is not defined then define it in the global namespace
#ifdef WIN32
int pread64(HANDLE current_handle,char *buf,size_t bytes,uint64_t offset)
{
    DWORD bytes_read = 0;
    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = SetFilePointer(current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) return -1;
    if (FALSE == ReadFile(current_handle, buf, (DWORD) bytes, &bytes_read, NULL)){
        return -1;
    }
    return bytes_read;
}
#else
  #if !defined(HAVE_PREAD64) && !defined(HAVE_PREAD) && defined(HAVE__LSEEKI64)
size_t pread64(int d,void *buf,size_t nbyte,int64_t offset)
{
    if(_lseeki64(d,offset,0)!=offset) return -1;
    return read(d,buf,nbyte);
}
  #endif
#endif

namespace hasher {

#ifdef WIN32
static BOOL get_drive_geometry(const wchar_t *wszPath, DISK_GEOMETRY *pdg)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    BOOL bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    hDevice = CreateFileW(wszPath,          // drive to open
                          0,                // no access to the drive
                          FILE_SHARE_READ | // share mode
                          FILE_SHARE_WRITE, 
                          NULL,             // default security attributes
                          OPEN_EXISTING,    // disposition
                          0,                // file attributes
                          NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
        {
            return (FALSE);
        }

    bResult = DeviceIoControl(hDevice,                       // device to be queried
                              IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
                              NULL, 0,                       // no input buffer
                              pdg, sizeof(*pdg),            // output buffer
                              &junk,                         // # bytes returned
                              (LPOVERLAPPED) NULL);          // synchronous I/O

    CloseHandle(hDevice);

    return (bResult);
}

#endif

/**
 * It's hard to figure out the filesize in an opearting system independent method that works with both
 * files and devices. This seems to work. It only requires a functioning pread64 or pread.
 */

#ifdef WIN32
static int64_t get_filesize(HANDLE fd)
#else
static int64_t get_filesize(int fd)
#endif
{
    char buf[64];
    int64_t raw_filesize = 0;		/* needs to be signed for lseek */
    int bits = 0;
    int i =0;

#if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buf,size_t nbyte,off_t offset);
#endif

#if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
#define pread64(d,buf,nbyte,offset) pread(d,buf,nbyte,offset)
    if(sizeof(off_t)!=8){
	err(1,"Compiled with off_t==%d and no pread64 support.",(int)sizeof(off_t));
    }
#endif

#ifndef WIN32
    /* We can use fstat if sizeof(st_size)==8 and st_size>0 */
    struct stat st;
    memset(&st,0,sizeof(st));
    if(sizeof(st.st_size)==8 && fstat(fd,&st)==0){
	if(st.st_size>0) return st.st_size;
    }
#endif

    /* Phase 1; figure out how far we can seek... */
    for(bits=0;bits<60;bits++){
	raw_filesize = ((int64_t)1<<bits);
	if(::pread64(fd,buf,1,raw_filesize)!=1){
	    break;
	}
    }
    if(bits==60) {
      std::cerr << "filesize seek error: Partition detection not functional.\n";
      return 0;
    }

    /* Phase 2; blank bits as necessary */
    for(i=bits;i>=0;i--){
	int64_t test = (int64_t)1<<i;
	int64_t test_filesize = raw_filesize | ((int64_t)1<<i);
	if(::pread64(fd,buf,1,test_filesize)==1){
	    raw_filesize |= test;
	} else{
	    raw_filesize &= ~test;
	}
    }
    if(raw_filesize>0) raw_filesize+=1;	/* seems to be needed */
    return raw_filesize;
}



// get_filesize_by_filename() for Win or Linux, return error_message or ""
std::string get_filesize_by_filename(const filename_t &fname,
                                     uint64_t* size) {
    int64_t file_length;
#ifdef WIN32
// Win
    HANDLE current_handle = CreateFile(fname.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				     OPEN_EXISTING, 0, NULL);
    if(current_handle!=INVALID_HANDLE_VALUE){
        // good, able to read file
        file_length = get_filesize(current_handle);
        ::CloseHandle(current_handle);
        *size = file_length;
        return "";

    } else {
        std::cout << "# Cannot open file '" << hasher::native_to_utf8(fname)
                  << " to read filesize, checking physical drive.\n";

        // try this
        /* On Windows, see if we can use this */
        // http://msdn.microsoft.com/en-gb/library/windows/desktop/aa363147%28v=vs.85%29.aspx
        DISK_GEOMETRY pdg = { 0 }; // disk drive geometry structure
        get_drive_geometry(fname.c_str(), &pdg);
        file_length = pdg.Cylinders.QuadPart *
                       (ULONG)pdg.TracksPerCylinder *
                       (ULONG)pdg.SectorsPerTrack *
                       (ULONG)pdg.BytesPerSector;
        *size = file_length;
        return "";
    }

#else
// Linux
#ifndef O_BINARY
#define O_BINARY 0
#endif
    int fd = ::open(fname.c_str(),O_RDONLY|O_BINARY);
    if(fd<0){
        *size = 0;
        std::stringstream ss;
        ss << "cannot open " << fname << " to read file size.  "
           << strerror(errno) << "\n";
        return ss.str();
    }
    file_length = get_filesize(fd);
    ::close(fd);
    *size = file_length;
    return "";
#endif
}

} // end namespace hasher

