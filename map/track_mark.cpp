#include "map/track_mark.hpp"

#include "indexer/scales.hpp"

TrackInfoMark::TrackInfoMark(m2::PointD const & ptOrg)
  : UserMark(ptOrg, UserMark::TRACK_INFO)
{
}

void TrackInfoMark::SetOffset(m2::PointF const & offset)
{
  SetDirty();
  m_offset = offset;
}

void TrackInfoMark::SetPosition(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

void TrackInfoMark::SetIsVisible(bool isVisible)
{
  SetDirty();
  m_isVisible = isVisible;
}

void TrackInfoMark::SetTrackId(kml::TrackId trackId)
{
  m_trackId = trackId;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> TrackInfoMark::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "ic_marker_infosign"));
  return symbol;
}

drape_ptr<df::UserPointMark::SymbolOffsets> TrackInfoMark::GetSymbolOffsets() const
{
  SymbolOffsets offsets(scales::UPPER_STYLE_SCALE, m_offset);
  return make_unique_dp<SymbolOffsets>(offsets);
}

TrackSelectionMark::TrackSelectionMark(m2::PointD const & ptOrg)
  : UserMark(ptOrg, UserMark::TRACK_SELECTION)
{
}

void TrackSelectionMark::SetPosition(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

void TrackSelectionMark::SetTrackId(kml::TrackId trackId)
{
  m_trackId = trackId;
}

void TrackSelectionMark::SetDistance(double distance)
{
  m_distance = distance;
}

void TrackSelectionMark::SetMyPositionDistance(double distance)
{
  m_myPositionDistance = distance;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> TrackSelectionMark::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "ic_marker_ontrack"));
  return symbol;
}
