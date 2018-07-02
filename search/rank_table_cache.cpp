#include "search/rank_table_cache.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/data_source.hpp"
#include "indexer/rank_table.hpp"

namespace search
{
RankTable const & RankTableCache::Get(DataSource & dataSource, TId const & mwmId)
{
  auto const it = m_ranks.find(TKey(mwmId));
  if (it != m_ranks.end())
    return *it->second;

  TKey handle(dataSource.GetMwmHandleById(mwmId));
  auto table = RankTable::Load(handle.GetValue<MwmValue>()->m_cont, SEARCH_RANKS_FILE_TAG);
  if (!table)
    table.reset(new DummyRankTable());

  return *(m_ranks.emplace(move(handle), move(table)).first->second.get());
}

void RankTableCache::Remove(TId const & id) { m_ranks.erase(TKey(id)); }

void RankTableCache::Clear() { m_ranks.clear(); }

}  // namespace search
