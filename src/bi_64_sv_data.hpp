// provide a <uint64_t, string_view> data structure suitable for use
// with a btree forward and reverse lookup indexed set.

#ifndef BI_64_SV_DATA_HPP
#define BI_64_SV_DATA_HPP

#include <boost/btree/index_helpers.hpp>
#include <boost/btree/support/string_view.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;
using boost::string_view;

struct bi_64_s_data_t {
  typedef uint64_t key_type;
  typedef string_view value_type;

  key_type key;
//  string_view value;
  value_type value;

  bi_data_t() {}
  bi_data_t(key_type p_key, std::string p_value) : key(p_key), value(p_value) {}
  bi_data_t(key_type p_key, string_view p_value) : key(p_key), value(p_value) {}
  std::string &value_string() { return value; }

  bool operator < (const bi_data_t& rhs) const { return key < rhs.key; }
};

//  stream inserter  ---------------------------------------------------------//

inline std::ostream& operator<<(std::ostream& os, const bi_data_t& x)
{
  os << x.key << " \"" << x.value << "\"";
  return os;
}

//  function objects for different orderings  --------------------------------//

struct value_ordering
{
  bool operator()(const bi_data_t& x, const bi_data_t& y) const
    {return x.value < y.value;}
};

//  specializations to support btree indexes  --------------------------------//

namespace boost
{
namespace btree
{
  template <> struct index_reference<bi_data_t> { typedef const bi_data_t type; };

  template <>
  inline void index_serialize<bi_data_t>(const bi_data_t& bi_data, flat_file_type& file)
  {
    index_serialize(bi_data.key, file);
    index_serialize(bi_data.value, file);
  }

  template <>
  inline index_reference<bi_data_t>::type index_deserialize<bi_data_t>(const char** flat)
  {
    bi_data_t bi_data;
    bi_data.key = index_deserialize<bi_data_t::key_type>(flat);
//    bi_data.value = index_deserialize<boost::string_view>(flat);
    bi_data.value = index_deserialize<bi_data_t::value_type>(flat);
    return bi_data;
  }
}} // namespaces

#endif

