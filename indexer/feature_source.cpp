#include "indexer/feature_source.hpp"

using namespace std;

string ToString(FeatureStatus fs)
{
  switch (fs)
  {
  case FeatureStatus::Untouched: return "Untouched";
  case FeatureStatus::Deleted: return "Deleted";
  case FeatureStatus::Obsolete: return "Obsolete";
  case FeatureStatus::Modified: return "Modified";
  case FeatureStatus::Created: return "Created";
  };
  return "Undefined";
}

FeatureSource::FeatureSource(MwmSet::MwmHandle const & handle) : m_handle(handle)
{
  if (!m_handle.IsAlive())
    return;

  auto const & value = *m_handle.GetValue<MwmValue>();
  m_vector = make_unique<FeaturesVector>(value.m_cont, value.GetHeader(), value.m_table.get());
}

size_t FeatureSource::GetNumFeatures() const
{
  if (!m_handle.IsAlive())
    return 0;

  ASSERT(m_vector.get(), ());
  return m_vector->GetNumFeatures();
}

bool FeatureSource::GetOriginalFeature(uint32_t index, FeatureType & feature) const
{
  ASSERT(m_handle.IsAlive(), ());
  ASSERT(m_vector != nullptr, ());
  m_vector->GetByIndex(index, feature);
  feature.SetID(FeatureID(m_handle.GetId(), index));
  return true;
}

FeatureStatus FeatureSource::GetFeatureStatus(uint32_t index) const
{
  return FeatureStatus::Untouched;
}

bool FeatureSource::GetModifiedFeature(uint32_t index, FeatureType & feature) const
{
  return false;
}

void FeatureSource::ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                             function<void(uint32_t)> const & fn) const
{
}
