#pragma once

#include "editor/osm_editor.hpp"

class DataSourceBase;

namespace search
{
class EditorDelegate : public osm::Editor::Delegate
{
public:
  EditorDelegate(DataSourceBase const & dataSource);

  // osm::Editor::Delegate overrides:
  MwmSet::MwmId GetMwmIdByMapName(string const & name) const override;
  unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const override;
  string GetOriginalFeatureStreet(FeatureType & ft) const override;
  void ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn,
                             m2::PointD const & point) const override;

private:
  DataSourceBase const & m_dataSource;
};
}  // namespace search
