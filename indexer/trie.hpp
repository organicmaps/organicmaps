#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/buffer_vector.hpp"

#include <cstddef>
#include <memory>

namespace trie
{
using TrieChar = uint32_t;

// 95 is a good value for the default baseChar, since both small and capital latin letters
// are less than +/- 32 from it and thus can fit into supershort edge.
// However 0 is used because the first byte is actually language id.
uint32_t constexpr kDefaultChar = 0;

template <typename ValueList>
struct Iterator
{
  using List = ValueList;
  using Value = typename List::Value;

  struct Edge
  {
    using EdgeLabel = buffer_vector<TrieChar, 8>;
    EdgeLabel m_label;
  };

  virtual ~Iterator() = default;

  virtual std::unique_ptr<Iterator<ValueList>> Clone() const = 0;
  virtual std::unique_ptr<Iterator<ValueList>> GoToEdge(size_t i) const = 0;

  buffer_vector<Edge, 8> m_edges;
  List m_values;
};

template <typename ValueList, typename ToDo, typename String>
void ForEachRef(Iterator<ValueList> const & it, ToDo && toDo, String const & s)
{
  it.m_values.ForEach([&toDo, &s](typename ValueList::Value const & value) { toDo(s, value); });

  for (size_t i = 0; i < it.m_edges.size(); ++i)
  {
    String s1(s);
    s1.insert(s1.end(), it.m_edges[i].m_label.begin(), it.m_edges[i].m_label.end());
    auto nextIt = it.GoToEdge(i);
    ForEachRef(*nextIt, toDo, s1);
  }
}
}  // namespace trie
