#include "indexer/popularity_loader.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "defines.hpp"

CachingPopularityLoader::CachingPopularityLoader(DataSource const & dataSource)
  : m_dataSource(dataSource)
{
}

uint8_t CachingPopularityLoader::Get(FeatureID const & featureId) const
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return {};

  auto it = m_deserializers.find(featureId.m_mwmId);

  if (it == m_deserializers.end())
  {
    auto rankTable =
        search::RankTable::Load(handle.GetValue<MwmValue>()->m_cont, POPULARITY_RANKS_FILE_TAG);

    auto const result = m_deserializers.emplace(featureId.m_mwmId, std::move(rankTable));
    it = result.first;
  }

  return it->second->Get(featureId.m_index);
}
