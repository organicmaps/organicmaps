#pragma once

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace search
{
// A trie.  TChar is a type of string characters, OutDegree is an
// expected branching factor. Therefore, both Add(s) and Has(s) have
// expected complexity O(OutDegree * Length(s)).  For small values of
// OutDegree this is quite fast, but if OutDegree can be large and you
// need guaranteed O(Log(AlphabetSize) * Length(s)), refactor this
// class.
//
// TODO (@y): unify this with base::MemTrie.
template <typename TChar, size_t OutDegree>
class StringSet
{
public:
  enum class Status
  {
    Absent,
    Prefix,
    Full,
  };

  StringSet() = default;

  template <typename TIt>
  void Add(TIt begin, TIt end)
  {
    auto & cur = m_root.MakeMove(begin, end);
    cur.m_isLeaf = true;
  }

  template <typename TIt>
  Status Has(TIt begin, TIt end) const
  {
    auto const * cur = m_root.Move(begin, end);
    if (!cur)
      return Status::Absent;

    return cur->m_isLeaf ? Status::Full : Status::Prefix;
  }

private:
  struct Node
  {
    Node() : m_isLeaf(false) {}

    // Tries to move from the current node by |c|. If the move can't
    // be made, returns nullptr.
    Node const * Move(TChar c) const
    {
      for (auto const & p : m_moves)
        if (p.first == c)
          return p.second.get();
      return nullptr;
    }

    // Tries to move from the current node by [|begin|, |end|). If the
    // move can't be made, returns nullptr.
    template <typename TIt>
    Node const * Move(TIt begin, TIt end) const
    {
      Node const * cur = this;
      for (; begin != end && cur; ++begin)
        cur = cur->Move(*begin);
      return cur;
    }

    // Moves from the current node by |c|. If the move can't be made,
    // creates a new sub-tree and moves to it.
    Node & MakeMove(TChar c)
    {
      for (auto const & p : m_moves)
        if (p.first == c)
          return *p.second;
      m_moves.emplace_back(c, std::make_unique<Node>());
      return *m_moves.back().second;
    }

    // Moves from the current node by [|begin|, |end|). If there were
    // no path from the current node labeled by [|begin|, |end|), this
    // method will create it.
    template <typename TIt>
    Node & MakeMove(TIt begin, TIt end)
    {
      Node * cur = this;
      for (; begin != end; ++begin)
        cur = &cur->MakeMove(*begin);
      return *cur;
    }

    buffer_vector<std::pair<TChar, std::unique_ptr<Node>>, OutDegree> m_moves;
    bool m_isLeaf;

    DISALLOW_COPY(Node);
  };

  Node m_root;

  DISALLOW_COPY(StringSet);
};
}  // namespace search
