#include "search/v2/rank_table_cache.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/index.hpp"
#include "indexer/rank_table.hpp"

namespace search
{
namespace v2
{
RankTableCache::RankTableCache() {}

RankTableCache::~RankTableCache() {}

RankTable const & RankTableCache::Get(MwmValue & value, MwmSet::MwmId const & mwmId)
{
  auto const it = m_ranks.find(mwmId);
  if (it != m_ranks.end())
    return *it->second;
  auto table = RankTable::Load(value.m_cont);
  if (!table)
    table.reset(new DummyRankTable());
  auto const * result = table.get();
  m_ranks[mwmId] = move(table);
  return *result;
}

void RankTableCache::Clear() { m_ranks.clear(); }
}  // namespace v2
}  // namespace search
