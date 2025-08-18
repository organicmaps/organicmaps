#pragma once

#include "map/user_mark.hpp"

class TrackInfoMark : public UserMark
{
public:
  explicit TrackInfoMark(m2::PointD const & ptOrg);

  void SetOffset(m2::PointF const & offset);
  void SetPosition(m2::PointD const & ptOrg);
  void SetIsVisible(bool isVisible);

  void SetTrackId(kml::TrackId trackId);
  kml::TrackId GetTrackId() const { return m_trackId; }

  // df::UserPointMark overrides.
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  dp::Anchor GetAnchor() const override { return dp::Bottom; }
  bool IsVisible() const override { return m_isVisible; }
  bool SymbolIsPOI() const override { return true; }

  df::SpecialDisplacement GetDisplacement() const override { return df::SpecialDisplacement::SpecialModeUserMark; }

  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::RoutingMarkLayer; }

private:
  m2::PointF m_offset;
  bool m_isVisible = false;
  kml::TrackId m_trackId = kml::kInvalidTrackId;
};

class TrackSelectionMark : public UserMark
{
public:
  static double constexpr kInvalidDistance = -1.0;

  explicit TrackSelectionMark(m2::PointD const & ptOrg);

  void SetPosition(m2::PointD const & ptOrg);
  void SetIsVisible(bool isVisible);
  void SetMinVisibleZoom(int zoom);

  void SetTrackId(kml::TrackId trackId);
  kml::TrackId GetTrackId() const { return m_trackId; }

  void SetDistance(double distance);
  double GetDistance() const { return m_distance; }

  void SetMyPositionDistance(double distance);
  double GetMyPositionDistance() const { return m_myPositionDistance; }

  // df::UserPointMark overrides.
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  int GetMinZoom() const override { return m_minVisibleZoom; }
  bool IsVisible() const override { return m_isVisible; }

  static std::string GetInitialSymbolName();

private:
  int m_minVisibleZoom = 1;
  double m_distance = 0.0;
  double m_myPositionDistance = kInvalidDistance;
  kml::TrackId m_trackId = kml::kInvalidTrackId;
  bool m_isVisible = false;
};
