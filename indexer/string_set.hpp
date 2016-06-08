#pragma once

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

#include "std/cstdint.hpp"
#include "std/unique_ptr.hpp"

namespace search
{
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

    Node const * Move(TChar c) const
    {
      for (auto const & p : m_moves)
      {
        if (p.first == c)
          return p.second.get();
      }
      return nullptr;
    }

    template <typename TIt>
    Node const * Move(TIt begin, TIt end) const
    {
      Node const * cur = this;
      for (; begin != end && cur; ++begin)
        cur = cur->Move(*begin);
      return cur;
    }

    Node & MakeMove(TChar c)
    {
      for (auto const & p : m_moves)
      {
        if (p.first == c)
          return *p.second;
      }
      m_moves.emplace_back(c, make_unique<Node>());
      return *m_moves.back().second;
    }

    template <typename TIt>
    Node & MakeMove(TIt begin, TIt end)
    {
      Node * cur = this;
      for (; begin != end; ++begin)
        cur = &cur->MakeMove(*begin);
      return *cur;
    }

    buffer_vector<pair<TChar, unique_ptr<Node>>, OutDegree> m_moves;
    bool m_isLeaf;

    DISALLOW_COPY(Node);
  };

  Node m_root;

  DISALLOW_COPY(StringSet);
};
}  // namespace search
