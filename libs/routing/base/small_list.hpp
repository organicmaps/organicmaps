#pragma once
#include "base/buffer_vector.hpp"

#include <map>

namespace routing
{
namespace impl
{

class Statistics
{
  struct Info
  {
    size_t m_sum = 0;
    size_t m_count = 0;
    size_t m_max = 0;
  };

  static std::map<char const *, Info> s_map;

public:
  static void Add(char const * name, size_t val);
  static void Dump();
};

}  // namespace impl

template <class T>
class SmallList : public buffer_vector<T, 8>
{
  using BaseT = buffer_vector<T, 8>;

public:
  using BaseT::BaseT;

  /// @todo Use in Generator only.
  /*
  void clear()
  {
    impl::Statistics::Add(typeid(T).name(), BaseT::size());
    BaseT::clear();
  }

  ~SmallList()
  {
    impl::Statistics::Add(typeid(T).name(), BaseT::size());
  }
  */
};

template <typename T>
inline std::string DebugPrint(SmallList<T> const & v)
{
  return DebugPrint(static_cast<buffer_vector<T, 8> const &>(v));
}

}  // namespace routing
