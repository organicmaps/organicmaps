#include "search/bookmarks/processor.hpp"

#include "search/emitter.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstddef>

using namespace std;

namespace search
{
namespace bookmarks
{
namespace
{
struct DocVecWrapper
{
  explicit DocVecWrapper(DocVec const & dv) : m_dv(dv) {}

  template <typename Fn>
  void ForEachToken(Fn && fn) const
  {
    for (size_t i = 0; i < m_dv.GetNumTokens(); ++i)
      fn(StringUtf8Multilang::kDefaultCode, m_dv.GetToken(i));
  }

  DocVec const & m_dv;
};

struct RankingInfo
{
  bool operator<(RankingInfo const & rhs) const
  {
    return m_cosineSimilarity > rhs.m_cosineSimilarity;
  }

  bool operator>(RankingInfo const & rhs) const { return rhs < *this; }

  bool operator==(RankingInfo const & rhs) const { return !(*this < rhs) && !(*this > rhs); }
  bool operator!=(RankingInfo const & rhs) const { return !(*this == rhs); }

  double m_cosineSimilarity = 0.0;
};

struct IdInfoPair
{
  IdInfoPair(Id const & id, RankingInfo const & info) : m_id(id), m_info(info) {}

  bool operator<(IdInfoPair const & rhs) const
  {
    if (m_info != rhs.m_info)
      return m_info < rhs.m_info;
    return m_id < rhs.m_id;
  }

  Id m_id;
  RankingInfo m_info;
};

void FillRankingInfo(QueryVec & qv, IdfMap & idfs, DocVec const & dv, RankingInfo & info)
{
  info.m_cosineSimilarity = qv.Similarity(idfs, dv);
}
}  // namespace

Processor::Processor(Emitter & emitter, ::base::Cancellable const & cancellable)
  : m_emitter(emitter), m_cancellable(cancellable)
{
}

void Processor::Add(Id const & id, Doc const & doc)
{
  ASSERT_EQUAL(m_docs.count(id), 0, ());

  DocVec::Builder builder;
  doc.ForEachToken(
      [&](int8_t /* lang */, strings::UniString const & token) { builder.Add(token); });

  DocVec const docVec(builder);

  m_index.Add(id, DocVecWrapper(docVec));
  m_docs[id] = docVec;
}

void Processor::Erase(Id const & id)
{
  ASSERT_EQUAL(m_docs.count(id), 1, ());

  auto const & docVec = m_docs[id];
  m_index.Erase(id, DocVecWrapper(docVec));
  m_docs.erase(id);
}

void Processor::Search(QueryParams const & params) const
{
  set<Id> ids;
  auto insertId = MakeInsertFunctor(ids);

  for (size_t i = 0; i < params.GetNumTokens(); ++i)
  {
    BailIfCancelled();

    auto const & token = params.GetToken(i);
    if (params.IsPrefixToken(i))
      Retrieve<strings::PrefixDFAModifier<strings::LevenshteinDFA>>(token, insertId);
    else
      Retrieve<strings::LevenshteinDFA>(token, insertId);
  }

  IdfMap idfs(*this, 1.0 /* unknownIdf */);
  auto qv = GetQueryVec(idfs, params);

  vector<IdInfoPair> idInfos;
  for (auto const & id : ids)
  {
    BailIfCancelled();

    auto it = m_docs.find(id);
    ASSERT(it != m_docs.end(), ("Can't find retrieved doc:", id));
    auto const & doc = it->second;

    RankingInfo info;
    FillRankingInfo(qv, idfs, doc, info);

    idInfos.emplace_back(id, info);
  }

  BailIfCancelled();
  sort(idInfos.begin(), idInfos.end());

  for (auto const & idInfo : idInfos)
    m_emitter.AddBookmarkResult(bookmarks::Result(idInfo.m_id));
}

uint64_t Processor::GetNumDocs(strings::UniString const & token, bool isPrefix) const
{
  return ::base::asserted_cast<uint64_t>(
      m_index.GetNumDocs(StringUtf8Multilang::kDefaultCode, token, isPrefix));
}

QueryVec Processor::GetQueryVec(IdfMap & idfs, QueryParams const & params) const
{
  QueryVec::Builder builder;
  for (size_t i = 0; i < params.GetNumTokens(); ++i)
  {
    auto const & token = params.GetToken(i).m_original;
    if (params.IsPrefixToken(i))
      builder.SetPrefix(token);
    else
      builder.AddFull(token);
  }
  return {idfs, builder};
}
}  // namespace bookmarks
}  // namespace search
