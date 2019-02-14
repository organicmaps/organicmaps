#include "search/rank_table_cache.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/data_source.hpp"
#include "indexer/rank_table.hpp"

namespace search
{
RankTable const & RankTableCache::Get(DataSource & dataSource, Id const & mwmId)
{
  auto const it = m_ranks.find(Key(mwmId));
  if (it != m_ranks.end())
    return *it->second;

  Key handle(dataSource.GetMwmHandleById(mwmId));
  auto table = RankTable::Load(handle.GetValue<MwmValue>()->m_cont, SEARCH_RANKS_FILE_TAG);
  if (!table)
    table.reset(new DummyRankTable());

  return *(m_ranks.emplace(std::move(handle), std::move(table)).first->second.get());
}

void RankTableCache::Remove(Id const & id) { m_ranks.erase(Key(id)); }

void RankTableCache::Clear() { m_ranks.clear(); }
}  // namespace search
