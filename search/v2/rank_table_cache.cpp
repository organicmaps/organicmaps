#include "search/v2/rank_table_cache.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/rank_table.hpp"

namespace search
{
namespace v2
{

RankTable const & RankTableCache::Get(Index & index, TId const & mwmId)
{
  auto const it = m_ranks.find(TKey(mwmId));
  if (it != m_ranks.end())
    return *it->second;

  TKey handle(index.GetMwmHandleById(mwmId));
  auto table = RankTable::Load(handle.GetValue<MwmValue>()->m_cont);
  if (!table)
    table.reset(new DummyRankTable());

  return *(m_ranks.emplace(move(handle), move(table)).first->second.get());
}

void RankTableCache::Remove(TId const & id)
{
  m_ranks.erase(TKey(id));
}

void RankTableCache::Clear()
{
  m_ranks.clear();
}

}  // namespace v2
}  // namespace search
