#include "map/user_mark.hpp"
#include "map/user_mark_id_storage.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

UserMark::UserMark(kml::MarkId id, m2::PointD const & ptOrg, UserMark::Type type)
  : df::UserPointMark(id == kml::kInvalidMarkId ? UserMarkIdStorage::Instance().GetNextUserMarkId(type) : id)
  , m_ptOrg(ptOrg)
{
  ASSERT_EQUAL(GetMarkType(), type, ());
}

UserMark::UserMark(m2::PointD const & ptOrg, UserMark::Type type)
  : df::UserPointMark(UserMarkIdStorage::Instance().GetNextUserMarkId(type))
  , m_ptOrg(ptOrg)
{}

// static
UserMark::Type UserMark::GetMarkType(kml::MarkId id)
{
  return UserMarkIdStorage::GetMarkType(id);
}

m2::PointD const & UserMark::GetPivot() const
{
  return m_ptOrg;
}

ms::LatLon UserMark::GetLatLon() const
{
  return mercator::ToLatLon(m_ptOrg);
}

StaticMarkPoint::StaticMarkPoint(m2::PointD const & ptOrg) : UserMark(ptOrg, UserMark::Type::STATIC) {}

void StaticMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

MyPositionMarkPoint::MyPositionMarkPoint(m2::PointD const & ptOrg) : StaticMarkPoint(ptOrg) {}

DebugMarkPoint::DebugMarkPoint(m2::PointD const & ptOrg) : UserMark(ptOrg, UserMark::Type::DEBUG_MARK) {}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> DebugMarkPoint::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "search-result-non-found"));
  return symbol;
}

ColoredMarkPoint::ColoredMarkPoint(m2::PointD const & ptOrg) : UserMark(ptOrg, UserMark::Type::COLORED)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  df::ColoredSymbolViewParams params;
  params.m_outlineColor = dp::Color::White();
  params.m_outlineWidth = 1.5f * vs;
  params.m_radiusInPixels = 7.0f * vs;
  params.m_color = dp::Color::Green();
  m_coloredSymbols.m_needOverlay = false;
  m_coloredSymbols.m_zoomInfo.insert(std::make_pair(1, params));
}

void ColoredMarkPoint::SetColor(dp::Color const & color)
{
  SetDirty();
  m_coloredSymbols.m_zoomInfo.begin()->second.m_color = color;
}

void ColoredMarkPoint::SetRadius(float radius)
{
  SetDirty();

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  m_coloredSymbols.m_zoomInfo.begin()->second.m_radiusInPixels = radius * vs;
}

drape_ptr<df::UserPointMark::ColoredSymbolZoomInfo> ColoredMarkPoint::GetColoredSymbols() const
{
  return make_unique_dp<ColoredSymbolZoomInfo>(m_coloredSymbols);
}

std::string DebugPrint(UserMark::Type type)
{
  switch (type)
  {
  case UserMark::Type::API: return "API";
  case UserMark::Type::SEARCH: return "SEARCH";
  case UserMark::Type::STATIC: return "STATIC";
  case UserMark::Type::BOOKMARK: return "BOOKMARK";
  case UserMark::Type::DEBUG_MARK: return "DEBUG_MARK";
  case UserMark::Type::ROUTING: return "ROUTING";
  case UserMark::Type::ROAD_WARNING: return "ROAD_WARNING";
  case UserMark::Type::SPEED_CAM: return "SPEED_CAM";
  case UserMark::Type::LOCAL_ADS: return "LOCAL_ADS";
  case UserMark::Type::TRANSIT: return "TRANSIT";
  case UserMark::Type::TRACK_INFO: return "TRACK_INFO";
  case UserMark::Type::TRACK_SELECTION: return "TRACK_SELECTION";
  case UserMark::Type::COLORED: return "COLORED";
  case UserMark::Type::USER_MARK_TYPES_COUNT: return "USER_MARK_TYPES_COUNT";
  case UserMark::Type::USER_MARK_TYPES_COUNT_MAX: return "USER_MARK_TYPES_COUNT_MAX";
  }
  UNREACHABLE();
}
