#include "search/feature_loader.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/feature_decl.hpp"

#include <memory>

namespace search
{
FeatureLoader::FeatureLoader(DataSource const & dataSource) : m_dataSource(dataSource) {}

bool FeatureLoader::Load(FeatureID const & id, FeatureType & ft)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());

  auto const & mwmId = id.m_mwmId;
  if (!m_guard || m_guard->GetId() != mwmId)
    m_guard = std::make_unique<FeaturesLoaderGuard>(m_dataSource, mwmId);
  return m_guard->GetFeatureByIndex(id.m_index, ft);
}

void FeatureLoader::Reset()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  m_guard.reset();
}
}  // namespace search
