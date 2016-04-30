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
 * Read E01,RAW, and general files, based on filename extension.
 *
 * Adapted heavily from bulk_extractor/src/image_process.cpp.
 */


#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <libewf.h>
#include "file_reader_helper.hpp"

#ifndef PATH_MAX
#define PATH_MAX 65536
#endif

namespace hasher {

static const size_t buffer_size = 16777216; // 2^24=16MiB

enum file_reader_type_t {E01, SERIAL, SINGLE};

class file_reader_t {

  public:
  const filename_t filename;
  const file_reader_type_t file_reader_type;
  std::string error_message;

  // very simple iterator to walk offsets up to filesize.
  // dereferencing simply returns the itherator's current offset.
  class const_iterator {
    private:
    const uint64_t filesize;
    const size_t increment;
    uint64_t current_offset;
    public:
    const_iterator(const uint64_t p_filesize, const uint64_t p_increment, bool at_end) :
              filesize(p_filesize), increment(p_increment), current_offset(0) {
      if (at_end) {
        current_offset = filesize;
      }
    }
    bool operator==(const const_iterator& it) {
      return (filesize == it.filesize &&
              increment == it.increment &&
              current_offset == it.current_offset);
    }
    bool operator!=(const const_iterator& it) {
      return !((*this) != it);
    }
    const_iterator& operator++() {
      if (current_offset == filesize) {
        std::cerr << "Usage error: attempt to increment iterator after end\n";
      }
      current_offset += increment;
      if (current_offset > filesize) {
        current_offset = filesize;
      }
      return *this;
    }
    uint64_t operator*(const const_iterator& it) {
      return current_offset;
    }
  };

  private:
  // E01 data
  libewf_handle_t* libewf_handle;

  // SERIAL 001 data
  uint64_t serial_filesize;
  class serial_file_info_t {
    public:
    filename_t name;
    int64_t offset;
    int64_t length;
    serial_file_info_t(const filename_t& p_name, const int64_t p_offset,
                       const int64_t p_length) : name(p_name),
                                   offset(p_offset), length(p_length) {
    }
  };
  typedef std::vector<serial_file_info_t> serial_file_list_t;
  serial_file_list_t serial_file_list;
  mutable filename_t serial_current_filename;   // which file is currently open
#ifdef WIN32
  mutable HANDLE serial_current_handle;         // currently open file
#else
  mutable int serial_current_fd;                // currently open file
#endif

  // SINGLE file data
  uint64_t single_filesize;
#ifdef WIN32
  mutable HANDLE single_handle;         // currently open file
#else
  mutable int single_fd;                // currently open file
#endif

  public:
  const bool is_open;
  const uint64_t filesize;
  uint8_t* const buffer;

  private:

  // do not allow copy or assignment
  file_reader_t(const file_reader_t&);
  file_reader_t& operator=(const file_reader_t&);

  // get reader type from fname extension
  file_reader_type_t reader_type(const filename_t& fname) {
    if (fname.size() < 4) {
      // no special fname extension
      return file_reader_type_t::SINGLE;
    }
    const filename_t last4 = fname.substr(fname.size() - 4);
    if (last4 == ".E01" || last4 == ".E01") {
      // E01
      return file_reader_type_t::E01;
    }
    if (last4 == ".000" || last4 == ".001") {
      // 001
      return file_reader_type_t::SERIAL;
    }
    if (fname.size() >= 8 && fname.substr(fname.size() - 8) ==
                                           "001.vmdk") {
      // 001.vdmk
      return file_reader_type_t::SERIAL;
    }
    // no special fname extension
    return file_reader_type_t::SINGLE;
  }

  // helper reads error and releases ewf error resource
  std::string consume_libewf_error(libewf_error_t* error) const {
    char e[500];
    libewf_error_sprint(error, e, 500);
    libewf_error_free(&error);
    return std::string(e);
  }

  // open the file for reading
  bool open_reader() {
    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {

#ifdef WIN32
        const wchar_t *fname = filename.c_str();
        char **libewf_filenames = NULL;
        int amount_of_filenames = 0;

        libewf_error_t *error=0;

        if(libewf_glob_wide(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,
                       &libewf_filenames,&amount_of_filenames,&error)<0){
          std::stringstream ss;
          ss << "libewf_glob " << fname << ", " << consume_libewf_error(error);
          error_message = ss.str();
          return false;
        }
 
        libewf_handle = 0;
        if(libewf_handle_initialize(&libewf_handle,NULL)<0){
          error_message ="Cannot initialize EWF handle?";
          return false;
        }
        if(libewf_handle_open_wide(libewf_handle,libewf_filenames,
                          amount_of_filenames, LIBEWF_OPEN_READ,&error)<0){
          std::stringstream ss;
          ss << "cannot open " << fname << ", " << consume_libewf_error(error);
          error_message = ss.str();
          return false;
        }
          
        // Free the allocated filenames
        if(libewf_glob_free_wide(libewf_filenames,amount_of_filenames,&error)<0){
          std::stringstream ss;
          error_message = ss.str();
          ss << "libewf_glob_free failed, " << consume_libewf_error(error);
          return false;
        }
        return true;

#else
        const char *fname = filename.c_str();
        char **libewf_filenames = NULL;
        int amount_of_filenames = 0;

        libewf_error_t *error=0;

        if(libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,
                       &libewf_filenames,&amount_of_filenames,&error)<0){
          std::stringstream ss;
          ss << "libewf_glob " << fname << ", " << consume_libewf_error(error);
          error_message = ss.str();
          return false;
        }
 
        libewf_handle = 0;
        if(libewf_handle_initialize(&libewf_handle,NULL)<0){
          error_message ="Cannot initialize EWF handle?";
          return false;
        }
        if(libewf_handle_open(libewf_handle,libewf_filenames,
                          amount_of_filenames, LIBEWF_OPEN_READ,&error)<0){
          std::stringstream ss;
          ss << "cannot open " << fname << ", " << consume_libewf_error(error);
          error_message = ss.str();
          return false;
        }
          
        // Free the allocated filenames
        if(libewf_glob_free(libewf_filenames,amount_of_filenames,&error)<0){
          std::stringstream ss;
          error_message = ss.str();
          ss << "libewf_glob_free failed, " << consume_libewf_error(error);
          return false;
        }
        return true;
#endif
      }

      // SERIAL 001
      case file_reader_type_t::SERIAL: {
        serial_add_file(filename);

        int num=0;
        const filename_t templ = serial_make_list_template(filename,&num);
        for(;;num++){
          char probename[PATH_MAX];
          snprintf(probename,sizeof(probename),templ.c_str(),num); 
          if(access(probename,R_OK)!=0) break;     // no more files
          serial_add_file(filename_t(probename));  // found another name
        }
        return true;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        single_filesize = getSizeOfFile(filename);
        return true;

        // open the file for reading
#ifdef WIN32
        single_handle = CreateFileA(filename.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				     OPEN_EXISTING, 0, NULL);
        if(single_handle==INVALID_HANDLE_VALUE){
          std::stringstream ss;
          ss << "hashdb WIN32 subsystem: cannot open file " << filename;
          error_message = ss.str();
	  return false;
	}
#else        
	single_fd = ::open(filename.c_str(),O_RDONLY|O_BINARY);
	if(single_fd<=0) {
          std::stringstream ss;
          ss << "hashdb WIN32 subsystem: cannot open file " << filename;
          error_message = ss.str();
	  return false;
	}
#endif
      }

      default: assert(0); std::exit(1);
    }
  }

  uint64_t get_filesize() {
    if (!is_open) {
      return 0;
    }

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        uint64_t ewf_filesize;
        libewf_handle_get_media_size(libewf_handle,
                                     (size64_t *)&ewf_filesize,NULL);
        return ewf_filesize;
      }

      // SERAIL 001
      case file_reader_type_t::SERIAL: {
        return serial_filesize;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        return single_filesize;
      }

      default: assert(0); std::exit(1);
    }
  }

  // SERIAL 001 support
  void serial_add_file(const filename_t &fname) {
    // get the size of this file
    const int64_t file_length = getSizeOfFile(fname);
    serial_file_list.push_back(serial_file_info_t(fname,serial_filesize,file_length));
    serial_filesize += file_length;
  }

  static filename_t serial_make_list_template(filename_t fn,int *start)
  {
    /* First find where the digits are */
    size_t p = fn.rfind("000");
    if(p==filename_t::npos) p = fn.rfind("001");
    assert(p!=filename_t::npos);
    
    *start = atoi(fn.substr(p,3).c_str()) + 1;
    fn.replace(p,3,"%03d");	// make it a format
    return fn;
  }

  const serial_file_info_t* serial_find_file_info_record(int64_t pos) const {
    for(serial_file_list_t::const_iterator it = serial_file_list.begin();
                                   it != serial_file_list.end();it++) {
        if((*it).offset<=pos && pos< ((*it).offset+(*it).length)){
	    return &(*it);
	}
    }
    return 0;
}

  /**
   * Read randomly between a split file.
   * 1. Determine which file to read and how many bytes from that file can be read.
   * 2. Perform the read.
   * 3. If there are additional files to read in the next file, recurse.
   */
  int serial_read(unsigned char *buf,size_t bytes,int64_t offset) const
  {
    const serial_file_info_t *fi = serial_find_file_info_record(offset);
    if(fi==0) return 0;			// nothing to read.

    /* See if the file is the one that's currently opened.
     * If not, close the current one and open the new one.
     */

    if(fi->name != serial_current_filename){
#ifdef WIN32
        if(serial_current_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(serial_current_handle);
#else
	if(serial_current_fd>=0) close(serial_current_fd);
#endif

	serial_current_filename = fi->name;
	fprintf(stderr,"Attempt to open %s\n",fi->name.c_str());
#ifdef WIN32
        serial_current_handle = CreateFileA(fi->name.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				     OPEN_EXISTING, 0, NULL);
        if(serial_current_handle==INVALID_HANDLE_VALUE){
	  fprintf(stderr,"bulk_extractor WIN32 subsystem: cannot open file '%s'\n",fi->name.c_str());
	  return -1;
	}
#else        
	serial_current_fd = ::open(fi->name.c_str(),O_RDONLY|O_BINARY);
	if(serial_current_fd<=0) return -1;	// can't read this data
#endif
    }

#if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buf,size_t nbyte,off_t offset);
#endif

#if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
#define pread64(d,buf,nbyte,offset) pread(d,buf,nbyte,offset)
#endif

    /* we have neither, so just hack it with lseek64 */

    assert(fi->offset <= offset);
#ifdef WIN32
    DWORD bytes_read = 0;
    LARGE_INTEGER li;
    li.QuadPart = offset - fi->offset;
    li.LowPart = SetFilePointer(serial_current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) return -1;
    if (FALSE == ReadFile(serial_current_handle, buf, (DWORD) bytes, &bytes_read, NULL)){
        return -1;
    }
#else
    ssize_t bytes_read = ::pread64(serial_current_fd,buf,bytes,offset - fi->offset);
#endif
    if(bytes_read<0) return -1;		// error???
    if((size_t)bytes_read==bytes) return bytes_read; // read precisely the correct amount!

    /* Need to recurse */
    ssize_t bytes_read2 = serial_read(buf+bytes_read,bytes-bytes_read,offset+bytes_read);
    if(bytes_read2<0) return -1;	// error on second read
    if(bytes_read==0) return 0;		// kind of odd.

    return bytes_read + bytes_read2;
  }

  int single_read(unsigned char *buf,size_t bytes,int64_t offset) const
  {
#if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buf,size_t nbyte,off_t offset);
#endif

#if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
#define pread64(d,buf,nbyte,offset) pread(d,buf,nbyte,offset)
#endif

    /* we have neither, so just hack it with lseek64 */

#ifdef WIN32
    DWORD bytes_read = 0;
    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = SetFilePointer(serial_current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) return -1;
    if (FALSE == ReadFile(serial_current_handle, buf, (DWORD) bytes, &bytes_read, NULL)){
        return -1;
    }
#else
    ssize_t bytes_read = ::pread64(serial_current_fd,buf,bytes,offset);
#endif
    return bytes_read;
  }

  public:
  /**
   * Opens a file reader.  The reader detects file types.
   * Provide the filename or device name to read from.
   * Provide an allocated buffer of size hasher::buffer_size for this
   * reader to read into.
   * Check is_open.  If false, print error_message.
   * To read: read(offset).
   * To iterate through file: use const_iterator.
   * Use these const globals as desired: filename, filesize, file_reader_type.
   */
  file_reader_t(const filename_t& p_filename, uint8_t* p_buffer) :
          filename(p_filename),
          file_reader_type(reader_type(filename)),
          error_message(""),
          libewf_handle(NULL),
          serial_filesize(0),
          serial_file_list(),
          serial_current_filename(""),
#ifdef WIN32
          serial_current_handle(INVALID_HANDLE_VALUE),
#else
          serial_current_fd(-1),
#endif
          single_filesize(0),
#ifdef WIN32
          single_handle(INVALID_HANDLE_VALUE),
#else
          single_fd(-1),
#endif
          is_open(open_reader()),
          filesize(get_filesize()),
          buffer(p_buffer) {
  }

  // destructor closes any open resources
  ~file_reader_t() {

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        if(libewf_handle){
          libewf_handle_close(libewf_handle,NULL);
          libewf_handle_free(&libewf_handle,NULL);
        }
        break;
      }

      // SERAIL 001
      case file_reader_type_t::SERIAL: {
#ifdef WIN32
        if(serial_current_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(serial_current_handle);
#else
        if(serial_current_fd>0) ::close(serial_current_fd);
#endif
        break;
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
#ifdef WIN32
        if(single_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(single_handle);
#else
	if(single_fd>=0) close(single_fd);
#endif
        break;
      }

      default: assert(0); std::exit(1);
    }
  }

  int read(int64_t offset) const {

    switch(file_reader_type) {

      // E01
      case file_reader_type_t::E01: {
        libewf_error_t *error=0;
        int ret = libewf_handle_read_random(libewf_handle,buffer,buffer_size,
                                            offset,&error);
        if(ret<0){
          std::cerr << consume_libewf_error(error);
        }
        return ret;
      }

      // SERAIL 001
      case file_reader_type_t::SERIAL: {
        return serial_read(buffer, buffer_size, offset);
      }

      // SINGLE binary file
      case file_reader_type_t::SINGLE: {
        return single_read(buffer, buffer_size, offset);
      }

      default: assert(0); std::exit(1);
    }
  }

  const_iterator begin() {
    return const_iterator(filesize, buffer_size, false);
  }
  const_iterator end() {
    return const_iterator(filesize, buffer_size, true);
  }

};

} // end namespace hasher

#endif

