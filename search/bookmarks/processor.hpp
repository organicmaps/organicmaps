#pragma once

#include "search/base/mem_search_index.hpp"
#include "search/bookmarks/results.hpp"
#include "search/bookmarks/types.hpp"
#include "search/cancel_exception.hpp"
#include "search/doc_vec.hpp"
#include "search/feature_offset_match.hpp"
#include "search/idf_map.hpp"
#include "search/query_params.hpp"
#include "search/utils.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace my
{
class Cancellable;
}

namespace search
{
class Emitter;

namespace bookmarks
{
class Processor : public IdfMap::Delegate
{
public:
  using Index = base::MemSearchIndex<Id>;

  Processor(Emitter & emitter, ::base::Cancellable const & cancellable);
  ~Processor() override = default;

  void Add(Id const & id, Doc const & doc);
  void Erase(Id const & id);

  void Search(QueryParams const & params) const;

  // IdfMap::Delegate overrides:
  uint64_t GetNumDocs(strings::UniString const & token, bool isPrefix) const override;

private:
  void BailIfCancelled() const { ::search::BailIfCancelled(m_cancellable); }

  template <typename DFA, typename Fn>
  void Retrieve(QueryParams::Token const & token, Fn && fn) const
  {
    SearchTrieRequest<DFA> request;
    token.ForEach([&request](strings::UniString const & s)
                  {
                    request.m_names.emplace_back(BuildLevenshteinDFA(s));
                  });
    request.m_langs.insert(StringUtf8Multilang::kDefaultCode);

    MatchFeaturesInTrie(request, m_index.GetRootIterator(),
                        [](Id const & /* id */) { return true; } /* filter */,
                        std::forward<Fn>(fn));
  }

  QueryVec GetQueryVec(IdfMap & idfs, QueryParams const & params) const;

  Emitter & m_emitter;
  ::base::Cancellable const & m_cancellable;

  Index m_index;
  std::unordered_map<Id, DocVec> m_docs;
};
}  // namespace bookmarks
}  // namespace search
