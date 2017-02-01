#pragma once

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace my
{
// This class is a simple in-memory trie which allows to add
// key-value pairs and then traverse them in a sorted order.
template <typename String, typename Value>
class MemTrie
{
private:
  struct Node;

public:
  using Char = typename String::value_type;

  MemTrie() = default;
  MemTrie(MemTrie && rhs) { *this = std::move(rhs); }

  MemTrie & operator=(MemTrie && rhs)
  {
    m_root = std::move(rhs.m_root);
    m_numNodes = rhs.m_numNodes;
    rhs.m_numNodes = 1;
    return *this;
  }

  // A read-only iterator. Any modification to the
  // underlying trie is assumed to invalidate the iterator.
  class Iterator
  {
  public:
    Iterator(MemTrie::Node const & node) : m_node(node) {}

    template <typename ToDo>
    void ForEachMove(ToDo && todo) const
    {
      for (auto const & move : m_node.m_moves)
        todo(move.first, Iterator(*move.second));
    }

    template <typename ToDo>
    void ForEachInNode(ToDo && todo) const
    {
      for (auto const & value : m_node.m_values)
        todo(value);
    }

  private:
    MemTrie::Node const & m_node;
  };

  // Adds a key-value pair to the trie.
  void Add(String const & key, Value const & value)
  {
    auto * cur = &m_root;
    for (auto const & c : key)
    {
      bool created;
      cur = &cur->GetMove(c, created);
      if (created)
        ++m_numNodes;
    }
    cur->AddValue(value);
  }

  // Traverses all key-value pairs in the trie and calls |todo| on each of them.
  template <typename ToDo>
  void ForEachInTrie(ToDo && todo) const
  {
    String prefix;
    ForEachInSubtree(m_root, prefix, std::forward<ToDo>(todo));
  }

  // Calls |todo| for each key-value pair in the node that is reachable
  // by |prefix| from the trie root. Does nothing if such node does
  // not exist.
  template <typename ToDo>
  void ForEachInNode(String const & prefix, ToDo && todo) const
  {
    if (auto const * root = MoveTo(prefix))
      ForEachInNode(*root, prefix, std::forward<ToDo>(todo));
  }

  // Calls |todo| for each key-value pair in a subtree that is
  // reachable by |prefix| from the trie root. Does nothing if such
  // subtree does not exist.
  template <typename ToDo>
  void ForEachInSubtree(String prefix, ToDo && todo) const
  {
    if (auto const * root = MoveTo(prefix))
      ForEachInSubtree(*root, prefix, std::forward<ToDo>(todo));
  }

  size_t GetNumNodes() const { return m_numNodes; }
  Iterator GetRootIterator() const { return Iterator(m_root); }
  Node const & GetRoot() const { return m_root; }

private:
  struct Node
  {
    friend class MemTrie<String, Value>::Iterator;

    Node() = default;
    Node(Node && /* rhs */) = default;

    Node & operator=(Node && /* rhs */) = default;

    Node & GetMove(Char const & c, bool & created)
    {
      auto & node = m_moves[c];
      if (!node)
      {
        node = my::make_unique<Node>();
        created = true;
      }
      else
      {
        created = false;
      }
      return *node;
    }

    void AddValue(Value const & value) { m_values.push_back(value); }

    std::map<Char, std::unique_ptr<Node>> m_moves;
    std::vector<Value> m_values;

    DISALLOW_COPY(Node);
  };

  Node const * MoveTo(String const & key) const
  {
    auto const * cur = &m_root;
    for (auto const & c : key)
    {
      auto const it = cur->m_moves.find(c);
      if (it == cur->m_moves.end())
        return nullptr;
      cur = it->second.get();
    }
    return cur;
  }

  // Calls |todo| for each key-value pair in a |node| that is
  // reachable by |prefix| from the trie root.
  template <typename ToDo>
  void ForEachInNode(Node const & node, String const & prefix, ToDo && todo) const
  {
    for (auto const & value : node.m_values)
      todo(prefix, value);
  }

  // Calls |todo| for each key-value pair in subtree where |node| is a
  // root of the subtree. |prefix| is a path from the trie root to the
  // |node|.
  template <typename ToDo>
  void ForEachInSubtree(Node const & node, String & prefix, ToDo && todo) const
  {
    ForEachInNode(node, prefix, todo);

    for (auto const & move : node.m_moves)
    {
      prefix.push_back(move.first);
      ForEachInSubtree(*move.second, prefix, todo);
      prefix.pop_back();
    }
  }

  Node m_root;
  size_t m_numNodes = 1;

  DISALLOW_COPY(MemTrie);
};
}  // namespace my
