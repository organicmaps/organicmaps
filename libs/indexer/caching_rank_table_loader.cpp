#include "indexer/caching_rank_table_loader.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/data_source.hpp"

CachingRankTableLoader::CachingRankTableLoader(DataSource const & dataSource, std::string const & sectionName)
  : m_dataSource(dataSource)
  , m_sectionName(sectionName)
{}

uint8_t CachingRankTableLoader::Get(FeatureID const & featureId) const
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return search::RankTable::kNoRank;

  auto it = m_deserializers.find(featureId.m_mwmId);

  if (it == m_deserializers.end())
  {
    auto rankTable = search::RankTable::Load(handle.GetValue()->m_cont, m_sectionName);

    if (!rankTable)
      rankTable = std::make_unique<search::DummyRankTable>();

    auto const result = m_deserializers.emplace(featureId.m_mwmId, std::move(rankTable));
    it = result.first;
  }

  return it->second->Get(featureId.m_index);
}

void CachingRankTableLoader::OnMwmDeregistered(platform::LocalCountryFile const & localFile)
{
  for (auto it = m_deserializers.begin(); it != m_deserializers.end(); ++it)
  {
    if (it->first.IsDeregistered(localFile))
    {
      m_deserializers.erase(it);
      return;
    }
  }
}
