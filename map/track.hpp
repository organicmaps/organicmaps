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
  DISALLOW_COPY_AND_MOVE(Track)

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

  string const & GetName() const;
  PolylineD const & GetPolyline() const { return m_polyline; }
  m2::RectD const & GetLimitRect() const;
  double GetLengthMeters() const;

  size_t GetLayerCount() const override;
  dp::Color const & GetColor(size_t layerIndex) const override;
  float GetWidth(size_t layerIndex) const override;
  float GetLayerDepth(size_t layerIndex) const override;

  /// Line geometry enumeration
  size_t GetPointCount() const override;
  m2::PointD const & GetPoint(size_t pointIndex) const override;

private:
  PolylineD m_polyline;
  Params m_params;
};
