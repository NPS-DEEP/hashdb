// provide a <uint64_t, uint64_t> data structure suitable for use
// with a btree forward and reverse lookup indexed set.

#ifndef BI_64_64_DATA_HPP
#define BI_64_64_DATA_HPP

#include <boost/btree/index_helpers.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;

struct bi_64_64_data_t {
  typedef uint64_t key_type;

  key_type key;
  uint64_t value;

  bi_data_t() {}
  bi_data_t(key_type p_key, uint64_t p_value) : key(p_key), value(p_value) {}
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
    bi_data.value = index_deserialize<boost::string_view>(flat);
    return bi_data;
  }
}} // namespaces

#endif

