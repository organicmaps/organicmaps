#pragma once

#include "macros.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"

namespace my
{
/// This class is a simple in-memory trie which allows to add
/// key-value pairs and then traverse them in a sorted order.
template <typename StringT, typename ValueT>
class MemTrie
{
public:
  MemTrie() = default;

  /// Adds a key-value pair to the trie.
  void Add(StringT const & key, ValueT const & value)
  {
    Node * cur = &m_root;
    for (auto const & c : key)
      cur = cur->GetMove(c);
    cur->AddValue(value);
  }

  /// Traverses all key-value pairs in the trie in a sorted order.
  ///
  /// \param toDo A callable object that will be called on an each
  ///             key-value pair.
  template <typename ToDo>
  void ForEach(ToDo const & toDo)
  {
    StringT prefix;
    ForEach(&m_root, prefix, toDo);
  }

private:
  struct Node
  {
    using CharT = typename StringT::value_type;
    using MovesMap = map<CharT, Node *>;

    Node() = default;

    ~Node()
    {
      for (auto const & move : m_moves)
        delete move.second;
    }

    Node * GetMove(CharT const & c)
    {
      Node *& node = m_moves[c];
      if (!node)
        node = new Node();
      return node;
    }

    void AddValue(const ValueT & value) { m_values.push_back(value); }

    MovesMap m_moves;
    vector<ValueT> m_values;

    DISALLOW_COPY_AND_MOVE(Node);
  };

  template <typename ToDo>
  void ForEach(Node * root, StringT & prefix, ToDo const & toDo)
  {
    if (!root->m_values.empty())
    {
      sort(root->m_values.begin(), root->m_values.end());
      for (auto const & value : root->m_values)
        toDo(prefix, value);
    }

    for (auto const & move : root->m_moves)
    {
      prefix.push_back(move.first);
      ForEach(move.second, prefix, toDo);
      prefix.pop_back();
    }
  }

  Node m_root;

  DISALLOW_COPY_AND_MOVE(MemTrie);
};  // class MemTrie
}  // namespace my
