#include "search/bookmarks/processor.hpp"

#include "search/emitter.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

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
  bool operator<(RankingInfo const & rhs) const { return m_cosineSimilarity > rhs.m_cosineSimilarity; }

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

Processor::Processor(Emitter & emitter, base::Cancellable const & cancellable)
  : m_emitter(emitter)
  , m_cancellable(cancellable)
{}

void Processor::Reset()
{
  m_index = {};
  m_docs.clear();
  m_indexDescriptions = false;
  m_indexableGroups.clear();
  m_idToGroup.clear();
  m_bookmarksInGroup.clear();
}

void Processor::EnableIndexingOfDescriptions(bool enable)
{
  m_indexDescriptions = enable;
}

void Processor::EnableIndexingOfBookmarkGroup(GroupId const & groupId, bool enable)
{
  bool const wasIndexable = m_indexableGroups.count(groupId) > 0;
  if (enable)
    m_indexableGroups.insert(groupId);
  else
    m_indexableGroups.erase(groupId);
  bool const nowIndexable = m_indexableGroups.count(groupId) > 0;

  if (wasIndexable == nowIndexable)
    return;

  for (auto const & id : m_bookmarksInGroup[groupId])
    if (nowIndexable)
      AddToIndex(id);
    else
      EraseFromIndex(id);
}

void Processor::Add(Id const & id, Doc const & doc)
{
  ASSERT_EQUAL(m_docs.count(id), 0, ());

  DocVec::Builder builder;
  doc.ForEachNameToken([&](int8_t /* lang */, strings::UniString const & token) { builder.Add(token); });

  if (m_indexDescriptions)
    doc.ForEachDescriptionToken([&](int8_t /* lang */, strings::UniString const & token) { builder.Add(token); });

  DocVec const docVec(builder);

  m_docs[id] = docVec;
}

void Processor::AddToIndex(Id const & id)
{
  ASSERT_EQUAL(m_docs.count(id), 1, ());

  m_index.Add(id, DocVecWrapper(m_docs[id]));
}

void Processor::Update(Id const & id, Doc const & doc)
{
  auto group = kInvalidGroupId;
  auto const groupIt = m_idToGroup.find(id);
  if (groupIt != m_idToGroup.end())
  {
    // A copy to avoid use-after-free.
    group = groupIt->second;
    DetachFromGroup(id, group);
  }

  Erase(id);
  Add(id, doc);

  if (group != kInvalidGroupId)
    AttachToGroup(id, group);
}

void Processor::Erase(Id const & id)
{
  ASSERT_EQUAL(m_docs.count(id), 1, ());

  ASSERT(m_idToGroup.find(id) == m_idToGroup.end(),
         ("A bookmark must be detached from all groups before being deleted."));

  m_docs.erase(id);
}

void Processor::EraseFromIndex(Id const & id)
{
  ASSERT_EQUAL(m_docs.count(id), 1, ());

  auto const & docVec = m_docs[id];
  m_index.Erase(id, DocVecWrapper(docVec));
}

void Processor::AttachToGroup(Id const & id, GroupId const & group)
{
  auto const it = m_idToGroup.find(id);
  if (it != m_idToGroup.end())
    LOG(LWARNING, ("Tried to attach bookmark", id, "to group", group, "but it already belongs to group", it->second));

  m_idToGroup[id] = group;
  m_bookmarksInGroup[group].insert(id);
  if (m_indexableGroups.count(group) > 0)
    AddToIndex(id);
}

void Processor::DetachFromGroup(Id const & id, GroupId const & group)
{
  auto const it = m_idToGroup.find(id);
  if (it == m_idToGroup.end())
  {
    LOG(LWARNING, ("Tried to detach bookmark", id, "from group", group, "but it does not belong to any group"));
    return;
  }

  if (it->second != group)
  {
    LOG(LWARNING, ("Tried to detach bookmark", id, "from group", group, "but it only belongs to group", it->second));
    return;
  }

  m_idToGroup.erase(it);
  m_bookmarksInGroup[group].erase(id);

  if (m_indexableGroups.count(group) > 0)
    EraseFromIndex(id);

  auto const groupIt = m_bookmarksInGroup.find(group);
  CHECK(groupIt != m_bookmarksInGroup.end(), (group, m_bookmarksInGroup));
  if (groupIt->second.size() == 0)
    m_bookmarksInGroup.erase(groupIt);
}

void Processor::Search(Params const & params) const
{
  std::set<Id> ids;
  auto insertId = [&ids](Id const & id, bool /* exactMatch */) { ids.insert(id); };

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

  std::vector<IdInfoPair> idInfos;
  for (auto const & id : ids)
  {
    BailIfCancelled();

    if (params.m_groupId != kInvalidGroupId)
    {
      auto const it = m_idToGroup.find(id);
      if (it == m_idToGroup.end() || it->second != params.m_groupId)
        continue;
    }

    auto it = m_docs.find(id);
    CHECK(it != m_docs.end(), ("Can't find retrieved doc:", id));
    auto const & doc = it->second;

    RankingInfo info;
    FillRankingInfo(qv, idfs, doc, info);

    idInfos.emplace_back(id, info);
  }

  BailIfCancelled();
  sort(idInfos.begin(), idInfos.end());

  size_t numEmitted = 0;
  for (auto const & idInfo : idInfos)
  {
    if (numEmitted >= params.m_maxNumResults)
      break;
    m_emitter.AddBookmarkResult(bookmarks::Result(idInfo.m_id));
    ++numEmitted;
  }
}

void Processor::Finish(bool cancelled)
{
  m_emitter.Finish(cancelled);
}

uint64_t Processor::GetNumDocs(strings::UniString const & token, bool isPrefix) const
{
  return base::asserted_cast<uint64_t>(m_index.GetNumDocs(StringUtf8Multilang::kDefaultCode, token, isPrefix));
}

QueryVec Processor::GetQueryVec(IdfMap & idfs, QueryParams const & params) const
{
  QueryVec::Builder builder;
  for (size_t i = 0; i < params.GetNumTokens(); ++i)
  {
    auto const & token = params.GetToken(i).GetOriginal();
    if (params.IsPrefixToken(i))
      builder.SetPrefix(token);
    else
      builder.AddFull(token);
  }
  return {idfs, builder};
}
}  // namespace bookmarks
}  // namespace search
