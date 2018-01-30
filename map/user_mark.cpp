#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

UserMark::UserMark(m2::PointD const & ptOrg, UserMarkManager * manager, UserMark::Type type, size_t index)
  : m_ptOrg(ptOrg), m_manager(manager), m_type(type), m_index(index)
{}

m2::PointD const & UserMark::GetPivot() const
{
  return m_ptOrg;
}

m2::PointD UserMark::GetPixelOffset() const
{
  return {};
}

dp::Anchor UserMark::GetAnchor() const
{
  return dp::Center;
}

float UserMark::GetDepth() const
{
  return m_manager->GetPointDepth(m_type, m_index);
}

df::RenderState::DepthLayer UserMark::GetDepthLayer() const
{
  return df::RenderState::UserMarkLayer;
}

ms::LatLon UserMark::GetLatLon() const
{
  return MercatorBounds::ToLatLon(m_ptOrg);
}

StaticMarkPoint::StaticMarkPoint(UserMarkManager * manager)
  : UserMark(m2::PointD{}, manager, UserMark::Type::STATIC, 0)
{}

void StaticMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

MyPositionMarkPoint::MyPositionMarkPoint(UserMarkManager * manager)
  : StaticMarkPoint(manager)
{}

DebugMarkPoint::DebugMarkPoint(const m2::PointD & ptOrg, UserMarkManager * manager)
  : UserMark(ptOrg, manager, UserMark::Type::DEBUG_MARK, 0)
{}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> DebugMarkPoint::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "api-result"));
  return symbol;
}

string DebugPrint(UserMark::Type type)
{
  switch (type)
  {
  case UserMark::Type::API: return "API";
  case UserMark::Type::SEARCH: return "SEARCH";
  case UserMark::Type::STATIC: return "STATIC";
  case UserMark::Type::BOOKMARK: return "BOOKMARK";
  case UserMark::Type::DEBUG_MARK: return "DEBUG_MARK";
  case UserMark::Type::ROUTING: return "ROUTING";
  case UserMark::Type::LOCAL_ADS: return "LOCAL_ADS";
  case UserMark::Type::TRANSIT: return "TRANSIT";
  }
}
