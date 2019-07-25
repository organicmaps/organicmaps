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

#include "indexer/search_string_utils.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace base
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
  using Index = search_base::MemSearchIndex<Id>;

  struct Params : public QueryParams
  {
    // If valid, only show results with bookmarks attached to |m_groupId|.
    GroupId m_groupId = kInvalidGroupId;
  };

  Processor(Emitter & emitter, base::Cancellable const & cancellable);
  ~Processor() override = default;

  // By default, only bookmark names are indexed. This method
  // should be used to enable or disable indexing bookmarks
  // by their descriptions.
  void EnableIndexingOfDescriptions(bool enable);

  void Add(Id const & id, Doc const & doc);
  void Erase(Id const & id);

  void AttachToGroup(Id const & id, GroupId const & group);
  void DetachFromGroup(Id const & id, GroupId const & group);

  void Search(Params const & params) const;

  // IdfMap::Delegate overrides:
  uint64_t GetNumDocs(strings::UniString const & token, bool isPrefix) const override;

private:
  void BailIfCancelled() const { ::search::BailIfCancelled(m_cancellable); }

  template <typename DFA, typename Fn>
  void Retrieve(QueryParams::Token const & token, Fn && fn) const
  {
    SearchTrieRequest<DFA> request;
    FillRequestFromToken(token, request);
    request.m_langs.insert(StringUtf8Multilang::kDefaultCode);

    MatchFeaturesInTrie(
        request, m_index.GetRootIterator(), [](Id const & /* id */) { return true; } /* filter */,
        std::forward<Fn>(fn));
  }

  QueryVec GetQueryVec(IdfMap & idfs, QueryParams const & params) const;

  Emitter & m_emitter;
  base::Cancellable const & m_cancellable;

  Index m_index;
  std::unordered_map<Id, DocVec> m_docs;

  bool m_indexDescriptions = false;

  // Currently a bookmark can belong to at most one group
  // but in the future it is possible for a single bookmark to be
  // attached to multiple groups.
  std::unordered_map<Id, GroupId> m_idToGroup;
};
}  // namespace bookmarks
}  // namespace search
