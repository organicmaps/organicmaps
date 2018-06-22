#include "search/feature_loader.hpp"

#include "indexer/feature_decl.hpp"

#include "base/stl_add.hpp"

namespace search
{
FeatureLoader::FeatureLoader(DataSourceBase const & dataSource) : m_dataSource(dataSource) {}

bool FeatureLoader::Load(FeatureID const & id, FeatureType & ft)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());

  auto const & mwmId = id.m_mwmId;
  if (!m_guard || m_guard->GetId() != mwmId)
    m_guard = my::make_unique<EditableDataSource::FeaturesLoaderGuard>(m_dataSource, mwmId);
  return m_guard->GetFeatureByIndex(id.m_index, ft);
}

void FeatureLoader::Reset()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  m_guard.reset();
}
}  // namespace search
