#pragma once

#include "kml/types.hpp"

#include "drape_frontend/user_marks_provider.hpp"
#include "drape/color.hpp"

#include "geometry/polyline2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

namespace location
{
  class RouteMatchingInfo;
}

class Track : public df::UserLineMark
{
  DISALLOW_COPY_AND_MOVE(Track);

public:
  explicit Track(kml::TrackData const & data);

  static void ResetLastId();

  bool IsDirty() const override { return m_isDirty; }
  void ResetChanges() const override { m_isDirty = false; }

  kml::TrackData const & GetData() const { return m_data; }

  std::string GetName() const;
  m2::RectD GetLimitRect() const;
  double GetLengthMeters() const;

  int GetMinZoom() const override { return 1; }
  df::RenderState::DepthLayer GetDepthLayer() const override;
  size_t GetLayerCount() const override;
  dp::Color GetColor(size_t layerIndex) const override;
  float GetWidth(size_t layerIndex) const override;
  float GetDepth(size_t layerIndex) const override;
  std::vector<m2::PointD> const & GetPoints() const override;

  df::MarkGroupID GetGroupId() const { return m_groupID; }
  void Attach(df::MarkGroupID groupId);
  void Detach();

private:
  kml::TrackData m_data;

  df::MarkGroupID m_groupID;
  mutable bool m_isDirty = true;
};
