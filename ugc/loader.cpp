#include "ugc/loader.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "defines.hpp"

namespace ugc
{
Loader::Loader(DataSource const & dataSource) : m_dataSource(dataSource) {}

UGC Loader::GetUGC(FeatureID const & featureId)
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return {};

  auto const & value = *handle.GetValue<MwmValue>();

  if (!value.m_cont.IsExist(UGC_FILE_TAG))
    return {};

  UGC ugc;
  EntryPtr entry;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_deserializers.find(featureId.m_mwmId);

    if (it == m_deserializers.end())
    {
      auto const result = m_deserializers.emplace(featureId.m_mwmId, make_shared<Entry>());
      it = result.first;
    }
    entry = it->second;
  }

  ASSERT(entry, ());

  {
    std::lock_guard<std::mutex> lock(entry->m_mutex);
    auto readerPtr = value.m_cont.GetReader(UGC_FILE_TAG);
    entry->m_deserializer.Deserialize(*readerPtr.GetPtr(), featureId.m_index, ugc);
  }

  return ugc;
}
}  // namespace ugc
