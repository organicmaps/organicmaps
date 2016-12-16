#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/buffer_vector.hpp"

#include "std/unique_ptr.hpp"

namespace trie
{
using TrieChar = uint32_t;

// 95 is a good value for the default baseChar, since both small and capital latin letters
// are less than +/- 32 from it and thus can fit into supershort edge.
// However 0 is used because the first byte is actually language id.
uint32_t constexpr kDefaultChar = 0;

template <typename TValueList>
class Iterator
{

public:
  using TValue = typename TValueList::TValue;

  struct Edge
  {
    using TEdgeLabel = buffer_vector<TrieChar, 8>;
    TEdgeLabel m_label;
  };

  buffer_vector<Edge, 8> m_edge;
  TValueList m_valueList;

  virtual ~Iterator() = default;

  virtual unique_ptr<Iterator<TValueList>> Clone() const = 0;
  virtual unique_ptr<Iterator<TValueList>> GoToEdge(size_t i) const = 0;
};

template <typename TValueList, typename TF, typename TString>
void ForEachRef(Iterator<TValueList> const & it, TF && f, TString const & s)
{
  it.m_valueList.ForEach([&f, &s](typename TValueList::TValue const & value)
                         {
                           f(s, value);
                         });
  for (size_t i = 0; i < it.m_edge.size(); ++i)
  {
    TString s1(s);
    s1.insert(s1.end(), it.m_edge[i].m_label.begin(), it.m_edge[i].m_label.end());
    auto nextIt = it.GoToEdge(i);
    ForEachRef(*nextIt, f, s1);
  }
}
}  // namespace trie
