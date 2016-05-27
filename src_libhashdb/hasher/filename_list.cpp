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
 * provide a filename list from a given path.
 * The path is utf8.  The list is string for POSIX or wstring for Win.
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
#ifdef WIN32
#include <strsafe.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stack>
#include <set>
#include <algorithm>  // for std::sort
#include "filename_t.hpp"

namespace hasher {

#ifdef WIN32 // Windows implementation

// adapted from stackoverflow.com/questions/67273/how-do-you-iterate-through-every-file-directory-recursively-in-standard-c

// get files, return error_message or ""
std::string filename_list(const std::string& utf8_filename,
                          filenames_t* files) {

  // get native filename
  const std::wstring native_filename = hasher::utf8_to_native(utf8_filename);

  // clear files
  files->clear();

  // first make sure the filename is a directory
  DWORD file_attributes = 0;
  file_attributes = GetFileAttributes(native_filename.c_str());
  if (file_attributes == INVALID_FILE_ATTRIBUTES) {
    return "invalid file attributes for file";
  }
  if (!(file_attributes & FILE_ATTRIBUTE_DIRECTORY)) {
    // not directory so just use filename
    files->push_back(native_filename);
    return "";
  }

  // stack for processing found directories
  std::stack<std::wstring> directories;

  // push the first directory for processing
  directories.push(native_filename);

  // process directories until empty
  std::set<uint64_t> seen_file_indexes;
  while (!directories.empty()) {
    const std::wstring path = directories.top();
    directories.pop();

    // prepare filename with '\*' appended
    const std::wstring filename_star = path + L"\\*";
    HANDLE filehandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA file_data;
    filehandle = FindFirstFile(filename_star.c_str(), &file_data);

    do {

      // make sure the file handle is valid
      if (filehandle == INVALID_HANDLE_VALUE)  {
        return "invalid file path from invalid handle";
      } 

      // skip file if "." or ".."
      if (wcscmp(file_data.cFileName, L".") == 0 ||
                              wcscmp(file_data.cFileName, L"..") == 0) {
        continue;
      }

      // prepare absolute_filename
      std::wstring absolute_filename = path + L"\\" +
                                         std::wstring(file_data.cFileName);
  
      // skip file if seen before
      HANDLE opened_filehandle = CreateFile(absolute_filename.c_str(),
             0,   // desired access
             FILE_SHARE_READ,
             NULL,  
             OPEN_EXISTING,
             (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS),
             NULL);
      if (opened_filehandle == INVALID_HANDLE_VALUE) {
        return "invalid file handle";
      }
      BY_HANDLE_FILE_INFORMATION fileinfo;
      bool got_info = GetFileInformationByHandle(opened_filehandle, &fileinfo);
      CloseHandle(opened_filehandle);
      if (!got_info) {
        return "invalid information by file handle";
      }
      uint64_t file_index = (((uint64_t)fileinfo.nFileIndexHigh)<<32) |
                            (fileinfo.nFileIndexLow);
      if (seen_file_indexes.find(file_index) == seen_file_indexes.end()) {
        // new
        seen_file_indexes.insert(file_index);
      } else {
        // seen so skip
        continue;
      }

      if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        directories.push(absolute_filename);

      } else {
        files->push_back(absolute_filename);
      }

    // next
    } while (FindNextFile(filehandle, &file_data) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
      FindClose(filehandle);
      return "invalid file path from invalid last error";
    }

    FindClose(filehandle);
    filehandle = INVALID_HANDLE_VALUE;
  }

  // sort the files
  std::sort(files->begin(), files->end());

  // strip out non-first recursive filenames such as *.E02, etc.
  // zz TBD

  // done
  return "";
}
#else // POSIX implementation

// Provide a (device, inode) encoding so we can see if the file
// has been seen before.  From bulk_extractor/src/dig.cpp.
class dev_inode_t {
public:
    dev_inode_t(dev_t dev_,ino_t ino_):dev(dev_),ino(ino_){}
    dev_inode_t(const dev_inode_t &di):dev(di.dev),ino(di.ino){}
    dev_t dev;
    ino_t ino;
    bool operator<(const dev_inode_t t2) const{
        return this->dev < t2.dev || (this->dev==t2.dev && this->ino < t2.ino);
    }
};

// get files, return error_message or ""
std::string filename_list(const std::string& filename, filenames_t* files) {

  // clear files
  files->clear();

  // first make sure the filename is a directory
  DIR *d = opendir(filename.c_str());
  if (d == NULL) {
    // filename is not a directory
    files->push_back(filename);
    return "";
  } else {
    // close resource
    closedir(d);
  }

  // stack for processing found directories
  std::stack<std::string> directories;

  // push the first directory for processing
  directories.push(filename);

  // process directories until empty
  std::set<dev_inode_t> seen_dev_inodes;

  while (!directories.empty()) {
    const std::string path = directories.top();
    directories.pop();

    // read POSIX directory entry
    DIR *dir= opendir(path.c_str());
    if (dir == NULL) {
      std::stringstream ss;
      ss << "falure in opendir reading path " << path;
      return ss.str();
    }

    // read files in directory
    while (true) {
      struct dirent *entry = readdir(dir);
      if (entry == NULL) {
        // done with readdir stream
        break;
      }

      // skip files "." and ".."
      const std::string file_suffix(entry->d_name);
      if (file_suffix == "." || file_suffix == "..") {
        continue;
      }

      // get next filename
      std::stringstream ss;
      ss << path << "/" << std::string(entry->d_name);
      const std::string next_filename = ss.str();

      // stat the file and maybe skip it
      struct stat st;
      if (stat(next_filename.c_str(), &st)) {
        // can't stat
        continue;
      }
      if(S_ISFIFO(st.st_mode)) continue; // FIFO
      if(S_ISSOCK(st.st_mode)) continue; // socket
      if(S_ISBLK(st.st_mode)) continue;  // block device
      if(S_ISCHR(st.st_mode)) continue;  // character device
      dev_inode_t dev_inode(st.st_dev, st.st_ino);
      if (seen_dev_inodes.find(dev_inode) == seen_dev_inodes.end()) {
        // new
        seen_dev_inodes.insert(dev_inode);
      } else {
        // seen
        continue;
      }

      // send filename to the directories stack or to the filenames vector
      DIR *name = opendir(next_filename.c_str());
      if (name == NULL) {
        // filename is not a directory
        files->push_back(next_filename);
      } else {
        // filename is a directory
        directories.push(next_filename);
      }
    }
  }

  // sort the files
  std::sort(files->begin(), files->end());

  // strip out non-first recursive filenames such as *.E02, etc.
  // zz TBD

  // done
  return "";
}

#endif

} // end namespace hasher
