#include "descriptions/loader.hpp"

#include "indexer/data_source.hpp"

#include "defines.hpp"

namespace descriptions
{
std::string Loader::GetWikiDescription(FeatureID const & featureId, std::vector<int8_t> const & langPriority)
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return {};

  auto const & value = *handle.GetValue();

  if (!value.m_cont.IsExist(DESCRIPTIONS_FILE_TAG))
    return {};

  // No need to have separate mutexes for each MWM since there is no concurrent Wiki pages reading.
  // Pros: lock is called once and a simple logic with OnMwmDeregistered synchronization.
  /// @todo Consider removing mutex at all or make wiki loading async (PlacePage info).

  std::lock_guard lock(m_mutex);
  Deserializer & deserializer = m_deserializers[featureId.m_mwmId];

  auto readerPtr = value.m_cont.GetReader(DESCRIPTIONS_FILE_TAG);
  return deserializer.Deserialize(*readerPtr.GetPtr(), featureId.m_index, langPriority);
}

void Loader::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  std::lock_guard lock(m_mutex);
  for (auto it = m_deserializers.begin(); it != m_deserializers.end(); ++it)
  {
    if (it->first.IsDeregistered(countryFile))
    {
      m_deserializers.erase(it);
      break;
    }
  }
}

}  // namespace descriptions
