#include "search/editor_delegate.hpp"

#include "search/reverse_geocoder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source_helpers.hpp"
#include "indexer/feature_decl.hpp"

namespace search
{
EditorDelegate::EditorDelegate(DataSourceBase const & index) : m_index(index) {}

MwmSet::MwmId EditorDelegate::GetMwmIdByMapName(string const & name) const
{
  return m_index.GetMwmIdByCountryFile(platform::CountryFile(name));
}

unique_ptr<FeatureType> EditorDelegate::GetOriginalFeature(FeatureID const & fid) const
{
  EditableDataSource::FeaturesLoaderGuard guard(m_index, fid.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(fid.m_index);
  if (feature)
    feature->ParseEverything();

  return feature;
}

string EditorDelegate::GetOriginalFeatureStreet(FeatureType & ft) const
{
  search::ReverseGeocoder const coder(m_index);
  auto const streets = coder.GetNearbyFeatureStreets(ft);
  if (streets.second < streets.first.size())
    return streets.first[streets.second].m_name;
  return {};
}

void EditorDelegate::ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn,
                                           m2::PointD const & point) const
{
  auto const kToleranceMeters = 1e-2;
  indexer::ForEachFeatureAtPoint(m_index, move(fn), point, kToleranceMeters);
}
}  // namespace search
