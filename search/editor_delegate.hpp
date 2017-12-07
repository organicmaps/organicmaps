#pragma once

#include "indexer/osm_editor.hpp"

class Index;

namespace search
{
class EditorDelegate : public osm::Editor::Delegate
{
public:
  EditorDelegate(Index const & index);

  // osm::Editor::Delegate overrides:
  MwmSet::MwmId GetMwmIdByMapName(string const & name) const override;
  unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const override;
  string GetOriginalFeatureStreet(FeatureType & ft) const override;
  void ForEachFeatureAtPoint(osm::Editor::FeatureTypeFn && fn,
                             m2::PointD const & point) const override;

private:
  Index const & m_index;
};
}  // namespace search
