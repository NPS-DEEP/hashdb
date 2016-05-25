/**
 * \file
 * provide system-specific filename type and conversion function for Windows.
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
#include "filename_t.hpp"

namespace hasher {

// get filename in native string type
filename_t utf8_to_native(const std::string &utf8_string)
#ifdef WIN32
// adapted from http://stackoverflow.com/questions/6693010/problem-using-multibytetowidechar
// MultiByteToWideChar needs Windows.h
{
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, NULL ,0 );
    if (wchars_num == 0 || wchars_num == 0xfffd) {
        std::cerr << "Invalid UTF8 code in filename.\n";
        return std::wstring(L"");
    }

    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, wstr, wchars_num);
    std::wstring fn16(wstr, wchars_num);
    delete[] wstr;
    return fn16;
}
#else
{
return utf8_string;
}
#endif

// get filename in utf8 string type
  std::string native_to_utf8(const filename_t& native_string) {
#ifdef WIN32
// from http://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
// Convert a wide Unicode string to an UTF8 string
    if( native_string.empty() ) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &native_string[0], (int)native_string.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &native_string[0], (int)native_string.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    return native_string;
#endif
  }

} // end namespace hasher

