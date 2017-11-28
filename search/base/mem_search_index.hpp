#pragma once

#include "search/base/inverted_list.hpp"

#include "indexer/trie.hpp"

#include "base/assert.hpp"
#include "base/mem_trie.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace search
{
namespace base
{
template <typename Id, typename Doc>
class MemSearchIndex
{
public:
  using Token = strings::UniString;
  using Char = Token::value_type;
  using List = InvertedList<Id>;
  using Trie = my::MemTrie<Token, List>;

  class Iterator : public trie::Iterator<List>
  {
  public:
    using Base = trie::Iterator<List>;
    using InnerIterator = typename Trie::Iterator;

    explicit Iterator(InnerIterator const & innerIt)
    {
      Base::m_values = innerIt.GetValues();
      innerIt.ForEachMove([&](Char c, InnerIterator it) {
        Base::m_edges.emplace_back();
        Base::m_edges.back().m_label.push_back(c);
        m_moves.push_back(it);
      });
    }

    ~Iterator() override = default;

    // trie::Iterator<List> overrides:
    std::unique_ptr<Base> Clone() const override { return my::make_unique<Iterator>(*this); }

    std::unique_ptr<Base> GoToEdge(size_t i) const override
    {
      ASSERT_LESS(i, m_moves.size(), ());
      return my::make_unique<Iterator>(m_moves[i]);
    }

  private:
    std::vector<InnerIterator> m_moves;
  };

  void Add(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Add(token, id); });
  }

  void Erase(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Erase(token, id); });
  }

  Iterator GetRootIterator() const { return Iterator(m_trie.GetRootIterator()); }

  std::vector<Id> GetAllIds() const
  {
    std::vector<Id> ids;
    m_trie.ForEachInTrie([&](Token const & /* token */, Id const & id) { ids.push_back(id); });
    my::SortUnique(ids);
    return ids;
  }

private:
  static Token AddLang(int8_t lang, Token const & token)
  {
    Token r(1 + token.size());
    r[0] = static_cast<Char>(lang);
    std::copy(token.begin(), token.end(), r.begin() + 1);
    return r;
  }

  template <typename Fn>
  void ForEachToken(Id const & id, Doc const & doc, Fn && fn)
  {
    doc.ForEachToken([&](int8_t lang, Token const & token) {
      if (lang >= 0)
        fn(AddLang(lang, token));
    });
  }

  Trie m_trie;
};
}  // namespace base
}  // namespace search
