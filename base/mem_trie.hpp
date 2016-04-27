#pragma once

#include "base/macros.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

namespace my
{
// This class is a simple in-memory trie which allows to add
// key-value pairs and then traverse them in a sorted order.
template <typename TString, typename TValue>
class MemTrie
{
public:
  MemTrie() = default;

  MemTrie(MemTrie && other) : m_root(move(other.m_root))
  {
    m_numNodes = other.m_numNodes;
    other.m_numNodes = 0;
  }

  MemTrie & operator=(MemTrie && other) = default;

  // Adds a key-value pair to the trie.
  void Add(TString const & key, TValue const & value)
  {
    Node * cur = &m_root;
    for (auto const & c : key)
    {
      size_t numNewNodes;
      cur = cur->GetMove(c, numNewNodes);
      m_numNodes += numNewNodes;
    }
    cur->AddValue(value);
  }

  // Traverses all key-value pairs in the trie and calls |toDo| on each of them.
  template <typename ToDo>
  void ForEach(ToDo && toDo)
  {
    TString prefix;
    ForEach(&m_root, prefix, forward<ToDo>(toDo));
  }

  template <typename ToDo>
  void ForEachInSubtree(TString prefix, ToDo && toDo) const
  {
    Node const * node = MoveTo(prefix);
    if (node)
      ForEach(node, prefix, forward<ToDo>(toDo));
  }

  size_t GetNumNodes() const { return m_numNodes; }

private:
  struct Node
  {
    using TChar = typename TString::value_type;

    Node() = default;

    Node(Node && other) = default;

    Node & operator=(Node && other) = default;

    ~Node()
    {
      for (auto const & move : m_moves)
        delete move.second;
    }

    Node * GetMove(TChar const & c, size_t & numNewNodes)
    {
      numNewNodes = 0;
      Node *& node = m_moves[c];
      if (!node)
      {
        node = new Node();
        ++numNewNodes;
      }
      return node;
    }

    void AddValue(TValue const & value) { m_values.push_back(value); }

    map<TChar, Node *> m_moves;
    vector<TValue> m_values;

    DISALLOW_COPY(Node);
  };

  Node const * MoveTo(TString const & key) const
  {
    Node const * cur = &m_root;
    for (auto const & c : key)
    {
      auto const it = cur->m_moves.find(c);
      if (it == cur->m_moves.end())
        return nullptr;
      cur = it->second;
    }
    return cur;
  }

  template <typename ToDo>
  void ForEach(Node const * root, TString & prefix, ToDo && toDo) const
  {
    if (!root->m_values.empty())
    {
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
  size_t m_numNodes = 0;

  DISALLOW_COPY(MemTrie);
};  // class MemTrie
}  // namespace my
