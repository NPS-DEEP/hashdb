// provide a <uint64_t, pair<uint64_t,uint64_t>> data structure suitable for use
// with a btree forward and reverse lookup indexed set.

#ifndef BI_64_PAIR_DATA_HPP
#define BI_64_PAIR_DATA_HPP

#include <boost/btree/index_helpers.hpp>
#include <iosfwd>

using std::cout;
using namespace boost::btree;

struct bi_64_pair_data_t {
  typedef uint64_t key_type;
  typedef std::pair<uint64_t,uint64_t> value_type;

  key_type key;
  value_type value;

//  bi_64_pair_data_t() {}
  bi_64_pair_data_t();
  bi_64_pair_data_t(key_type p_key, value_type p_value) : key(p_key), value(p_value) {}

  bool operator < (const bi_64_pair_data_t& rhs) const { return key < rhs.key; }

  //  function objects for different orderings  ------------------------------//
  struct value_ordering {
    bool operator()(const bi_64_pair_data_t& x, const bi_64_pair_data_t& y) const
      {return x.value < y.value;}
  };
};

//  stream inserter  ---------------------------------------------------------//

inline std::ostream& operator<<(std::ostream& os, const bi_64_pair_data_t& x)
{
  os << x.key << ", pair(" << x.value.first << ", " << x.value.second << ")";
  return os;
}

////  function objects for different orderings  --------------------------------//
//
//struct value_ordering
//{
//  bool operator()(const bi_64_pair_data_t& x, const bi_64_pair_data_t& y) const
//    {return x.value < y.value;}
//};

//  specializations to support btree indexes  --------------------------------//

namespace boost
{
namespace btree
{
  template <> struct index_reference<bi_64_pair_data_t> { typedef const bi_64_pair_data_t type; };

  template <>
  inline void index_serialize<bi_64_pair_data_t>(const bi_64_pair_data_t& bi_data, flat_file_type& file)
  {
    index_serialize(bi_data.key, file);
    index_serialize(bi_data.value, file);
  }

  template <>
  inline index_reference<bi_64_pair_data_t>::type index_deserialize<bi_64_pair_data_t>(const char** flat)
  {
    bi_64_pair_data_t bi_data;
    bi_data.key = index_deserialize<bi_64_pair_data_t::key_type>(flat);
    bi_data.value = index_deserialize<bi_64_pair_data_t::value_type>(flat);
    return bi_data;
  }
}} // namespaces

#endif

