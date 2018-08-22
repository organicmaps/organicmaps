#include "search/editor_delegate.hpp"

#include "search/reverse_geocoder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source_helpers.hpp"
#include "indexer/feature_decl.hpp"

namespace search
{
EditorDelegate::EditorDelegate(DataSource const & dataSource) : m_dataSource(dataSource) {}

MwmSet::MwmId EditorDelegate::GetMwmIdByMapName(string const & name) const
{
  return m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(name));
}

unique_ptr<FeatureType> EditorDelegate::GetOriginalFeature(FeatureID const & fid) const
{
  FeaturesLoaderGuard guard(m_dataSource, fid.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(fid.m_index);
  if (feature)
    feature->ParseEverything();

  return feature;
}

string EditorDelegate::GetOriginalFeatureStreet(FeatureType & ft) const
{
  search::ReverseGeocoder const coder(m_dataSource);
  auto const streets = coder.GetNearbyOriginalFeatureStreets(ft);
  if (streets.second < streets.first.size())
    return streets.first[streets.second].m_name;
  return {};
}

void EditorDelegate::ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn,
                                           m2::PointD const & point) const
{
  auto const kToleranceMeters = 1e-2;
  indexer::ForEachFeatureAtPoint(m_dataSource, move(fn), point, kToleranceMeters);
}
}  // namespace search
