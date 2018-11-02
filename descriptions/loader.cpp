#include "descriptions/loader.hpp"

#include "indexer/data_source.hpp"

#include "base/assert.hpp"

#include "defines.hpp"

namespace descriptions
{
bool Loader::GetDescription(FeatureID const & featureId, std::vector<int8_t> const & langPriority,
                            std::string & description)
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return false;

  auto const & value = *handle.GetValue<MwmValue>();

  if (!value.m_cont.IsExist(DESCRIPTIONS_FILE_TAG))
    return false;

  EntryPtr entry;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_deserializers.find(featureId.m_mwmId);

    if (it == m_deserializers.end())
    {
      auto const result = m_deserializers.emplace(featureId.m_mwmId, std::make_shared<Entry>());
      it = result.first;
    }
    entry = it->second;
  }

  ASSERT(entry, ());

  auto readerPtr = value.m_cont.GetReader(DESCRIPTIONS_FILE_TAG);

  std::lock_guard<std::mutex> lock(entry->m_mutex);
  return entry->m_deserializer.Deserialize(*readerPtr.GetPtr(), featureId.m_index, langPriority, description);
}
}  // namespace descriptions
