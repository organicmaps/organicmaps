#include "search/editor_delegate.hpp"

#include "search/reverse_geocoder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source_helpers.hpp"
#include "indexer/feature_decl.hpp"

using namespace std;

namespace search
{
EditorDelegate::EditorDelegate(DataSource const & dataSource) : m_dataSource(dataSource) {}

MwmSet::MwmId EditorDelegate::GetMwmIdByMapName(string const & name) const
{
  return m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(name));
}

unique_ptr<osm::EditableMapObject> EditorDelegate::GetOriginalMapObject(FeatureID const & fid) const
{
  FeaturesLoaderGuard guard(m_dataSource, fid.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(fid.m_index);
  if (!feature)
    return {};

  auto object = make_unique<osm::EditableMapObject>();
  object->SetFromFeatureType(*feature);
  return object;
}

string EditorDelegate::GetOriginalFeatureStreet(FeatureID const & fid) const
{
  search::ReverseGeocoder const coder(m_dataSource);
  return coder.GetOriginalFeatureStreetName(fid);
}

void EditorDelegate::ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn, m2::PointD const & point) const
{
  auto const kToleranceMeters = 1e-2;
  indexer::ForEachFeatureAtPoint(m_dataSource, std::move(fn), point, kToleranceMeters);
}
}  // namespace search
