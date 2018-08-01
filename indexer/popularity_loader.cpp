#include "indexer/popularity_loader.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "defines.hpp"

namespace
{
uint8_t const kNoPopularity = 0;
}  // namespace

CachingPopularityLoader::CachingPopularityLoader(DataSource const & dataSource)
  : m_dataSource(dataSource)
{
}

uint8_t CachingPopularityLoader::Get(FeatureID const & featureId) const
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return kNoPopularity;

  auto it = m_deserializers.find(featureId.m_mwmId);

  if (it == m_deserializers.end())
  {
    auto rankTable =
        search::RankTable::Load(handle.GetValue<MwmValue>()->m_cont, POPULARITY_RANKS_FILE_TAG);

    if (!rankTable)
      rankTable = std::make_unique<search::DummyRankTable>();

    auto const result = m_deserializers.emplace(featureId.m_mwmId, std::move(rankTable));
    it = result.first;
  }

  return it->second->Get(featureId.m_index);
}

void CachingPopularityLoader::OnMwmDeregistered(platform::LocalCountryFile const & localFile)
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
