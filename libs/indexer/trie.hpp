#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/buffer_vector.hpp"
#include "base/mem_trie.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace trie
{
using TrieChar = char32_t;

// 95 is a good value for the default baseChar, since both small and capital latin letters
// are less than +/- 32 from it and thus can fit into supershort edge.
// However 0 is used because the first byte is actually language id.
TrieChar constexpr kDefaultChar = 0;

template <typename ValueList>
struct Iterator
{
  using List = ValueList;
  using Value = typename List::Value;

  struct Edge
  {
    using EdgeLabel = buffer_vector<TrieChar, 8>;

    Edge() = default;

    template <typename It>
    Edge(It begin, It end) : m_label(begin, end)
    {}

    EdgeLabel m_label;
  };

  virtual ~Iterator() = default;

  virtual std::unique_ptr<Iterator<ValueList>> Clone() const = 0;
  virtual std::unique_ptr<Iterator<ValueList>> GoToEdge(size_t i) const = 0;

  buffer_vector<Edge, 8> m_edges;
  List m_values;
};

template <typename String, typename ValueList>
class MemTrieIterator final : public trie::Iterator<ValueList>
{
public:
  using Base = trie::Iterator<ValueList>;

  using Char = typename String::value_type;
  using InnerIterator = typename base::MemTrie<String, ValueList>::Iterator;

  explicit MemTrieIterator(InnerIterator const & innerIt)
  {
    Base::m_values = innerIt.GetValues();
    innerIt.ForEachMove([&](Char c, InnerIterator it)
    {
      auto const label = it.GetLabel();
      Base::m_edges.emplace_back();
      auto & edge = Base::m_edges.back().m_label;
      edge.push_back(c);
      edge.append(label);
      m_moves.push_back(it);
    });
  }

  ~MemTrieIterator() override = default;

  // Iterator<ValueList> overrides:
  std::unique_ptr<Base> Clone() const override { return std::make_unique<MemTrieIterator>(*this); }

  std::unique_ptr<Base> GoToEdge(size_t i) const override
  {
    ASSERT_LESS(i, m_moves.size(), ());
    return std::make_unique<MemTrieIterator>(m_moves[i]);
  }

private:
  std::vector<InnerIterator> m_moves;
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
