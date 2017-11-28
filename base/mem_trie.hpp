#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace my
{
template <typename Char, typename Subtree>
class MapMoves
{
public:
  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (auto const & subtree : m_subtrees)
      toDo(subtree.first, *subtree.second);
  }

  Subtree * GetSubtree(Char const & c) const
  {
    auto const it = m_subtrees.find(c);
    if (it == m_subtrees.end())
      return nullptr;
    return it->second.get();
  }

  Subtree & GetOrCreateSubtree(Char const & c, bool & created)
  {
    auto & node = m_subtrees[c];
    if (!node)
    {
      node = my::make_unique<Subtree>();
      created = true;
    }
    else
    {
      created = false;
    }
    return *node;
  }

  void EraseSubtree(Char const & c) { m_subtrees.erase(c); }

  bool Empty() const { return m_subtrees.empty(); }

  void Clear() { m_subtrees.clear(); }

private:
  std::map<Char, std::unique_ptr<Subtree>> m_subtrees;
};

template <typename Value>
struct VectorValues
{
  using value_type = Value;

  template <typename... Args>
  void Add(Args &&... args)
  {
    m_values.emplace_back(std::forward<Args>(args)...);
  }

  void Erase(Value const & v)
  {
    auto const it = find(m_values.begin(), m_values.end(), v);
    if (it != m_values.end())
      m_values.erase(it);
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    std::for_each(m_values.begin(), m_values.end(), std::forward<ToDo>(toDo));
  }

  bool Empty() const { return m_values.empty(); }

  void Clear() { m_values.clear(); }

  std::vector<Value> m_values;
};

// This class is a simple in-memory trie which allows to add
// key-value pairs and then traverse them in a sorted order.
template <typename String, class ValuesHolder, template <typename...> class Moves = MapMoves>
class MemTrie
{
private:
  struct Node;

public:
  using Char = typename String::value_type;
  using Value = typename ValuesHolder::value_type;

  MemTrie() = default;
  MemTrie(MemTrie && rhs) { *this = std::move(rhs); }

  MemTrie & operator=(MemTrie && rhs)
  {
    m_root = std::move(rhs.m_root);
    m_numNodes = rhs.m_numNodes;
    rhs.Clear();
    return *this;
  }

  // A read-only iterator wrapping a Node. Any modification to the
  // underlying trie is assumed to invalidate the iterator.
  class Iterator
  {
  public:
    using Char = MemTrie::Char;

    Iterator(MemTrie::Node const & node) : m_node(node) {}

    // Iterates over all possible moves from this Iterator's node
    // and calls |toDo| with two arguments:
    // (Char of the move, Iterator wrapping the node of the move).
    template <typename ToDo>
    void ForEachMove(ToDo && toDo) const
    {
      m_node.m_moves.ForEach([&](Char c, Node const & node) { toDo(c, Iterator(node)); });
    }

    // Calls |toDo| for every value in this Iterator's node.
    template <typename ToDo>
    void ForEachInNode(ToDo && toDo) const
    {
      m_node.m_values.ForEach(std::forward<ToDo>(toDo));
    }

    ValuesHolder const & GetValues() const { return m_node.m_values; }

  private:
    MemTrie::Node const & m_node;
  };

  // Adds a key-value pair to the trie.
  template <typename... Args>
  void Add(String const & key, Args &&... args)
  {
    auto * cur = &m_root;
    for (auto const & c : key)
    {
      bool created;
      cur = &cur->GetMove(c, created);
      if (created)
        ++m_numNodes;
    }
    cur->Add(std::forward<Args>(args)...);
  }

  void Erase(String const & key, Value const & value)
  {
    return Erase(m_root, 0 /* level */, key, value);
  }

  // Traverses all key-value pairs in the trie and calls |toDo| on each of them.
  template <typename ToDo>
  void ForEachInTrie(ToDo && toDo) const
  {
    String prefix;
    ForEachInSubtree(m_root, prefix, std::forward<ToDo>(toDo));
  }

  // Calls |toDo| for each key-value pair in the node that is reachable
  // by |prefix| from the trie root. Does nothing if such node does
  // not exist.
  template <typename ToDo>
  void ForEachInNode(String const & prefix, ToDo && toDo) const
  {
    if (auto const * root = MoveTo(prefix))
      ForEachInNode(*root, prefix, std::forward<ToDo>(toDo));
  }

  // Calls |toDo| for each key-value pair in a subtree that is
  // reachable by |prefix| from the trie root. Does nothing if such
  // subtree does not exist.
  template <typename ToDo>
  void ForEachInSubtree(String prefix, ToDo && toDo) const
  {
    if (auto const * root = MoveTo(prefix))
      ForEachInSubtree(*root, prefix, std::forward<ToDo>(toDo));
  }

  void Clear()
  {
    m_root.Clear();
    m_numNodes = 1;
  }

  size_t GetNumNodes() const { return m_numNodes; }
  Iterator GetRootIterator() const { return Iterator(m_root); }
  Node const & GetRoot() const { return m_root; }

private:
  struct Node
  {
    Node() = default;
    Node(Node && /* rhs */) = default;

    Node & operator=(Node && /* rhs */) = default;

    Node & GetMove(Char const & c, bool & created)
    {
      return m_moves.GetOrCreateSubtree(c, created);
    }

    Node * GetMove(Char const & c) const { return m_moves.GetSubtree(c); }

    void EraseMove(Char const & c) { m_moves.EraseSubtree(c); }

    template <typename... Args>
    void Add(Args &&... args)
    {
      m_values.Add(std::forward<Args>(args)...);
    }

    void EraseValue(Value const & value)
    {
      m_values.Erase(value);
    }

    bool Empty() const { return m_moves.Empty() && m_values.Empty(); }

    void Clear()
    {
      m_moves.Clear();
      m_values.Clear();
    }

    Moves<Char, Node> m_moves;
    ValuesHolder m_values;

    DISALLOW_COPY(Node);
  };

  Node const * MoveTo(String const & key) const
  {
    auto const * cur = &m_root;
    for (auto const & c : key)
    {
      cur = cur->GetMove(c);
      if (!cur)
        break;
    }
    return cur;
  }

  void Erase(Node & root, size_t level, String const & key, Value const & value)
  {
    if (level == key.size())
    {
      root.EraseValue(value);
      return;
    }

    ASSERT_LESS(level, key.size(), ());
    auto * child = root.GetMove(key[level]);
    if (!child)
      return;
    Erase(*child, level + 1, key, value);
    if (child->Empty())
    {
      root.EraseMove(key[level]);
      --m_numNodes;
    }
  }

  // Calls |toDo| for each key-value pair in a |node| that is
  // reachable by |prefix| from the trie root.
  template <typename ToDo>
  void ForEachInNode(Node const & node, String const & prefix, ToDo && toDo) const
  {
    node.m_values.ForEach(
        std::bind(std::forward<ToDo>(toDo), std::ref(prefix), std::placeholders::_1));
  }

  // Calls |toDo| for each key-value pair in subtree where |node| is a
  // root of the subtree. |prefix| is a path from the trie root to the
  // |node|.
  template <typename ToDo>
  void ForEachInSubtree(Node const & node, String & prefix, ToDo && toDo) const
  {
    ForEachInNode(node, prefix, toDo);

    node.m_moves.ForEach([&](Char c, Node const & node)
                         {
                           prefix.push_back(c);
                           ForEachInSubtree(node, prefix, toDo);
                           prefix.pop_back();
                         });
  }

  Node m_root;
  size_t m_numNodes = 1;

  DISALLOW_COPY(MemTrie);
};
}  // namespace my
