#pragma once

#include "editor/osm_editor.hpp"

#include "indexer/editable_map_object.hpp"

#include <memory>
#include <string>

class DataSource;

namespace search
{
class EditorDelegate : public osm::Editor::Delegate
{
public:
  EditorDelegate(DataSource const & dataSource);

  // osm::Editor::Delegate overrides:
  MwmSet::MwmId GetMwmIdByMapName(std::string const & name) const override;
  std::unique_ptr<osm::EditableMapObject> GetOriginalMapObject(FeatureID const & fid) const override;
  std::string GetOriginalFeatureStreet(FeatureID const & fid) const override;
  void ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn, m2::PointD const & point) const override;

private:
  DataSource const & m_dataSource;
};
}  // namespace search
