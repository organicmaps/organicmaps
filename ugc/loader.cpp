#include "ugc/loader.hpp"

#include "ugc/types.hpp"

#include "indexer/feature.hpp"
#include "indexer/index.hpp"

#include "defines.hpp"

namespace ugc
{
Loader::Loader(Index const & index) : m_index(index) {}

void Loader::GetUGC(FeatureID const & featureId, UGC & result)
{
  UGC ugc;
  auto const & handle = m_index.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return;

  auto const & value = *handle.GetValue<MwmValue>();

  if (!value.m_cont.IsExist(UGC_FILE_TAG))
    return;

  auto readerPtr = value.m_cont.GetReader(UGC_FILE_TAG);

  if (!m_d.Deserialize(*readerPtr.GetPtr(), featureId.m_index, ugc))
    return;

  result = std::move(ugc);
}
}  // namespace ugc
