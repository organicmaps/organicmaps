#pragma once

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
  using PolylineD = m2::PolylineD;

  struct TrackOutline
  {
    float m_lineWidth;
    dp::Color m_color;
  };

  struct Params
  {
    buffer_vector<TrackOutline, 2> m_colors;
    string m_name;
  };

  explicit Track(PolylineD const & polyline, Params const & p);

  bool IsDirty() const override { return m_isDirty; }
  void AcceptChanges() const override { m_isDirty = false; }

  string const & GetName() const;
  PolylineD const & GetPolyline() const { return m_polyline; }
  m2::RectD GetLimitRect() const;
  double GetLengthMeters() const;

  int GetMinZoom() const override { return 1; }
  df::RenderState::DepthLayer GetDepthLayer() const override;
  size_t GetLayerCount() const override;
  dp::Color const & GetColor(size_t layerIndex) const override;
  float GetWidth(size_t layerIndex) const override;
  float GetDepth(size_t layerIndex) const override;
  std::vector<m2::PointD> const & GetPoints() const override;

  df::MarkGroupID GetGroupId() const { return m_groupID; }
  void Attach(df::MarkGroupID groupID);
  void Detach();

private:
  PolylineD m_polyline;
  Params m_params;
  df::MarkGroupID m_groupID;
  mutable bool m_isDirty = true;
};
