#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace base
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

  template <typename ToDo>
  void ForEach(ToDo && toDo)
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
      node = std::make_unique<Subtree>();
      created = true;
    }
    else
    {
      created = false;
    }
    return *node;
  }

  void AddSubtree(Char const & c, std::unique_ptr<Subtree> subtree)
  {
    ASSERT(!GetSubtree(c), ());
    m_subtrees.emplace(c, std::move(subtree));
  }

  void EraseSubtree(Char const & c) { m_subtrees.erase(c); }

  size_t Size() const { return m_subtrees.size(); }
  bool Empty() const { return Size() == 0; }

  void Clear() { m_subtrees.clear(); }

  void Swap(MapMoves & rhs) { m_subtrees.swap(rhs.m_subtrees); }

private:
  std::map<Char, std::unique_ptr<Subtree>> m_subtrees;
};

template <typename Char, typename Subtree>
class VectorMoves
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
    auto const it = Find(c);
    return it != m_subtrees.cend() ? it->second.get() : nullptr;
  }

  Subtree & GetOrCreateSubtree(Char const & c, bool & created)
  {
    if (auto * const subtree = GetSubtree(c))
    {
      created = false;
      return *subtree;
    }

    created = true;
    m_subtrees.emplace_back(c, std::make_unique<Subtree>());
    return *m_subtrees.back().second;
  }

  void AddSubtree(Char const & c, std::unique_ptr<Subtree> subtree)
  {
    ASSERT(!GetSubtree(c), ());
    m_subtrees.emplace_back(c, std::move(subtree));
  }

  void EraseSubtree(Char const & c)
  {
    auto const it = Find(c);
    if (it != m_subtrees.end())
      m_subtrees.erase(it);
  }

  size_t Size() const { return m_subtrees.size(); }
  bool Empty() const { return Size() == 0; }

  void Clear() { m_subtrees.clear(); }

  void Swap(VectorMoves & rhs) { m_subtrees.swap(rhs.m_subtrees); }

private:
  using Entry = std::pair<Char, std::unique_ptr<Subtree>>;
  using Entries = std::vector<Entry>;

  typename Entries::iterator Find(Char const & c)
  {
    return std::find_if(m_subtrees.begin(), m_subtrees.end(), [&c](Entry const & entry) { return entry.first == c; });
  }

  typename Entries::const_iterator Find(Char const & c) const
  {
    return std::find_if(m_subtrees.cbegin(), m_subtrees.cend(), [&c](Entry const & entry) { return entry.first == c; });
  }

  Entries m_subtrees;
};

template <typename V>
struct VectorValues
{
  using value_type = V;
  using Value = V;

  template <typename... Args>
  void Add(Args &&... args)
  {
    m_values.emplace_back(std::forward<Args>(args)...);
  }

  void Erase(Value const & v)
  {
    auto const it = std::find(m_values.begin(), m_values.end(), v);
    if (it != m_values.end())
      m_values.erase(it);
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (auto const & value : m_values)
      toDo(value);
  }

  bool Empty() const { return m_values.empty(); }

  void Clear() { m_values.clear(); }

  void Swap(VectorValues & rhs) { m_values.swap(rhs.m_values); }

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
    rhs.Clear();
    return *this;
  }

  // A read-only iterator wrapping a Node. Any modification to the
  // underlying trie is assumed to invalidate the iterator.
  class Iterator
  {
  public:
    using Char = MemTrie::Char;

    Iterator(Node const & node) : m_node(node) {}

    // Iterates over all possible moves from this Iterator's node
    // and calls |toDo| with two arguments:
    // (Char of the move, Iterator wrapping the node of the move).
    template <typename ToDo>
    void ForEachMove(ToDo && toDo) const
    {
      m_node.m_moves.ForEach([&](Char const & c, Node const & node) { toDo(c, Iterator(node)); });
    }

    // Calls |toDo| for every value in this Iterator's node.
    template <typename ToDo>
    void ForEachInNode(ToDo && toDo) const
    {
      m_node.m_values.ForEach(toDo);
    }

    String GetLabel() const { return m_node.m_edge.template As<String>(); }
    ValuesHolder const & GetValues() const { return m_node.m_values; }

  private:
    Node const & m_node;
  };

  // Adds a key-value pair to the trie.
  template <typename... Args>
  void Add(String const & key, Args &&... args)
  {
    auto * curr = &m_root;

    auto it = key.begin();
    while (it != key.end())
    {
      bool created;
      curr = &curr->GetOrCreateMove(*it++, created);

      auto & edge = curr->m_edge;

      if (created)
      {
        edge.Assign(it, key.end());
        it = key.end();
        continue;
      }

      size_t i = 0;
      SkipEqual(edge, key.end(), i, it);

      if (i == edge.Size())
      {
        // We may directly add value to the |curr| values, when edge
        // equals to the rest of the |key|.  Otherwise we need to jump
        // to the next iteration of the loop and continue traversal of
        // the trie.
        continue;
      }

      // We need to split the edge to |curr|.
      auto node = std::make_unique<Node>();

      ASSERT_LESS(i, edge.Size(), ());
      node->m_edge = edge.DropFirst(i);

      ASSERT(!edge.Empty(), ());
      auto const next = edge[0];
      edge.DropFirst(1);

      node->Swap(*curr);
      curr->AddChild(next, std::move(node));
    }

    curr->AddValue(std::forward<Args>(args)...);
  }

  void Erase(String const & key, Value const & value) { Erase(m_root, key.begin(), key.end(), value); }

  // Traverses all key-value pairs in the trie and calls |toDo| on each of them.
  template <typename ToDo>
  void ForEachInTrie(ToDo && toDo) const
  {
    String prefix;
    ForEachInSubtree(m_root, prefix, toDo);
  }

  // Calls |toDo| for each key-value pair in the node that is reachable
  // by |prefix| from the trie root. Does nothing if such node does
  // not exist.
  template <typename ToDo>
  void ForEachInNode(String const & prefix, ToDo && toDo) const
  {
    MoveTo(prefix, true /* fullMatch */,
           [&](Node const & node, Edge const & /* edge */, size_t /* offset */) { node.m_values.ForEach(toDo); });
  }

  template <typename ToDo>
  void WithValuesHolder(String const & prefix, ToDo && toDo) const
  {
    MoveTo(prefix, true /* fullMatch */,
           [&toDo](Node const & node, Edge const & /* edge */, size_t /* offset */) { toDo(node.m_values); });
  }

  // Calls |toDo| for each key-value pair in a subtree that is
  // reachable by |prefix| from the trie root. Does nothing if such
  // subtree does not exist.
  template <typename ToDo>
  void ForEachInSubtree(String const & prefix, ToDo && toDo) const
  {
    MoveTo(prefix, false /* fullMatch */, [&](Node const & node, Edge const & edge, size_t offset)
    {
      String p = prefix;
      for (; offset < edge.Size(); ++offset)
        p.push_back(edge[offset]);
      ForEachInSubtree(node, p, toDo);
    });
  }

  bool HasKey(String const & key) const
  {
    bool exists = false;
    MoveTo(key, true /* fullMatch */,
           [&](Node const & node, Edge const & /* edge */, size_t /* offset */) { exists = !node.m_values.Empty(); });
    return exists;
  }

  bool HasPrefix(String const & prefix) const
  {
    bool exists = false;
    MoveTo(prefix, false /* fullMatch */,
           [&](Node const & node, Edge const & /* edge */, size_t /* offset */) { exists = !node.Empty(); });
    return exists;
  }

  void Clear() { m_root.Clear(); }

  size_t GetNumNodes() const { return m_root.GetNumNodes(); }

  Iterator GetRootIterator() const { return Iterator(m_root); }

  Node const & GetRoot() const { return m_root; }

private:
  // Stores tail (all characters except the first one) of an edge from
  // a parent node to a child node. Characters are stored in reverse
  // order, because we need to efficiently add and cut prefixes.
  class Edge
  {
  public:
    Edge() = default;

    template <typename It>
    Edge(It begin, It end)
    {
      Assign(begin, end);
    }

    template <typename It>
    void Assign(It begin, It end)
    {
      m_label.assign(begin, end);
      std::reverse(m_label.begin(), m_label.end());
    }

    // Drops first |n| characters from the edge.
    //
    // Complexity: amortized O(n).
    Edge DropFirst(size_t n)
    {
      ASSERT_LESS_OR_EQUAL(n, Size(), ());

      Edge prefix(m_label.rbegin(), m_label.rbegin() + n);
      m_label.erase(m_label.begin() + Size() - n, m_label.end());
      return prefix;
    }

    // Prepends |c| to the edge.
    //
    // Complexity: amortized O(1).
    void Prepend(Char const & c) { m_label.push_back(c); }

    // Prepends |prefix| to the edge.
    //
    // Complexity: amortized O(|prefix|)
    void Prepend(Edge const & prefix) { m_label.insert(m_label.end(), prefix.m_label.cbegin(), prefix.m_label.cend()); }

    Char operator[](size_t i) const
    {
      ASSERT_LESS(i, Size(), ());
      return *(m_label.rbegin() + i);
    }

    size_t Size() const { return m_label.size(); }

    bool Empty() const { return Size() == 0; }

    void Swap(Edge & rhs) { m_label.swap(rhs.m_label); }

    template <typename Sequence>
    Sequence As() const
    {
      return {m_label.rbegin(), m_label.rend()};
    }

    friend std::string DebugPrint(Edge const & edge) { return edge.template As<std::string>(); }

  private:
    std::vector<Char> m_label;
  };

  struct Node
  {
    Node() = default;
    Node(Node && /* rhs */) = default;

    Node & operator=(Node && /* rhs */) = default;

    Node & GetOrCreateMove(Char const & c, bool & created) { return m_moves.GetOrCreateSubtree(c, created); }

    Node * GetMove(Char const & c) const { return m_moves.GetSubtree(c); }

    void AddChild(Char const & c, std::unique_ptr<Node> node) { m_moves.AddSubtree(c, std::move(node)); }

    void EraseMove(Char const & c) { m_moves.EraseSubtree(c); }

    template <typename... Args>
    void AddValue(Args &&... args)
    {
      m_values.Add(std::forward<Args>(args)...);
    }

    void EraseValue(Value const & value) { m_values.Erase(value); }

    bool Empty() const { return m_moves.Empty() && m_values.Empty(); }

    void Clear()
    {
      m_moves.Clear();
      m_values.Clear();
    }

    size_t GetNumNodes() const
    {
      size_t size = 1;
      m_moves.ForEach([&size](Char const & /* c */, Node const & child) { size += child.GetNumNodes(); });
      return size;
    }

    void Swap(Node & rhs)
    {
      m_moves.Swap(rhs.m_moves);
      m_edge.Swap(rhs.m_edge);
      m_values.Swap(rhs.m_values);
    }

    Moves<Char, Node> m_moves;
    Edge m_edge;
    ValuesHolder m_values;

    DISALLOW_COPY(Node);
  };

  template <typename Fn>
  void MoveTo(String const & prefix, bool fullMatch, Fn && fn) const
  {
    auto const * cur = &m_root;

    auto it = prefix.begin();

    if (it == prefix.end())
    {
      fn(*cur, cur->m_edge, 0 /* offset */);
      return;
    }

    while (true)
    {
      ASSERT(it != prefix.end(), ());

      cur = cur->GetMove(*it++);
      if (!cur)
        return;

      auto const & edge = cur->m_edge;
      size_t i = 0;
      SkipEqual(edge, prefix.end(), i, it);

      if (i < edge.Size())
      {
        if (it != prefix.end() || fullMatch)
          return;
      }

      ASSERT(i == edge.Size() || (it == prefix.end() && !fullMatch), ());

      if (it == prefix.end())
      {
        fn(*cur, edge, i /* offset */);
        return;
      }

      ASSERT(it != prefix.end() && i == edge.Size(), ());
    }
  }

  template <typename It>
  void Erase(Node & root, It cur, It end, Value const & value)
  {
    if (cur == end)
    {
      root.EraseValue(value);
      if (root.m_values.Empty() && root.m_moves.Size() == 1)
      {
        Node child;
        Char c;
        root.m_moves.ForEach([&](Char const & mc, Node & mn)
        {
          c = mc;
          child.Swap(mn);
        });
        child.m_edge.Prepend(c);
        child.m_edge.Prepend(root.m_edge);
        root.Swap(child);
      }
      return;
    }

    auto const symbol = *cur++;

    auto * child = root.GetMove(symbol);
    if (!child)
      return;

    auto const & edge = child->m_edge;
    size_t i = 0;
    SkipEqual(edge, end, i, cur);

    if (i == edge.Size())
    {
      Erase(*child, cur, end, value);
      if (child->Empty())
        root.EraseMove(symbol);
    }
  }

  // Calls |toDo| for each key-value pair in subtree where |node| is a
  // root of the subtree. |prefix| is a path from the trie root to the
  // |node|. Because we need to accumulate labels from the root to the
  // current |node| somewhere, |prefix| is passed by reference here,
  // but after the call the contents of the |prefix| will be the same
  // as before the call.
  template <typename ToDo>
  void ForEachInSubtree(Node const & node, String & prefix, ToDo && toDo) const
  {
    node.m_values.ForEach([&prefix, &toDo](Value const & value) { toDo(static_cast<String const &>(prefix), value); });

    node.m_moves.ForEach([&](Char const & c, Node const & node)
    {
      auto const size = prefix.size();
      auto const edge = node.m_edge.template As<String>();
      prefix.push_back(c);
      prefix.insert(prefix.end(), edge.begin(), edge.end());
      ForEachInSubtree(node, prefix, toDo);
      prefix.resize(size);
    });
  }

  template <typename It>
  void SkipEqual(Edge const & edge, It end, size_t & i, It & cur) const
  {
    while (i < edge.Size() && cur != end && edge[i] == *cur)
    {
      ++i;
      ++cur;
    }
  }

  Node m_root;

  DISALLOW_COPY(MemTrie);
};
}  // namespace base
