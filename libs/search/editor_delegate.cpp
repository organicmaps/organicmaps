#include "search/editor_delegate.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/data_source.hpp"
#include "indexer/data_source_helpers.hpp"
#include "indexer/feature_decl.hpp"

namespace search
{
EditorDelegate::EditorDelegate(DataSource const & dataSource) : m_dataSource(dataSource) {}

MwmSet::MwmId EditorDelegate::GetMwmIdByMapName(std::string const & name) const
{
  return m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(name));
}

std::unique_ptr<osm::EditableMapObject> EditorDelegate::GetOriginalMapObject(FeatureID const & fid) const
{
  FeaturesLoaderGuard guard(m_dataSource, fid.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(fid.m_index);
  if (!feature)
    return {};

  auto object = std::make_unique<osm::EditableMapObject>();
  object->SetFromFeatureType(*feature);
  return object;
}

std::string EditorDelegate::GetOriginalFeatureStreet(FeatureID const & fid) const
{
  search::ReverseGeocoder const coder(m_dataSource);
  return coder.GetOriginalFeatureStreetName(fid);
}

void EditorDelegate::ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn, m2::PointD const & point) const
{
  indexer::ForEachFeatureAtPoint(m_dataSource, std::move(fn), point);
}
}  // namespace search
