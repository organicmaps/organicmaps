#pragma once

#include "editor/osm_editor.hpp"

class DataSource;

namespace search
{
class EditorDelegate : public osm::Editor::Delegate
{
public:
  EditorDelegate(DataSource const & dataSource);

  // osm::Editor::Delegate overrides:
  MwmSet::MwmId GetMwmIdByMapName(string const & name) const override;
  unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const override;
  string GetOriginalFeatureStreet(FeatureType & ft) const override;
  void ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn,
                             m2::PointD const & point) const override;

private:
  DataSource const & m_dataSource;
};
}  // namespace search
