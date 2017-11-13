#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

UserMark::UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg), m_container(container)
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
  return GetContainer()->GetPointDepth();
}

df::RenderState::DepthLayer UserMark::GetDepthLayer() const
{
  return df::RenderState::UserMarkLayer;
}

UserMarkContainer const * UserMark::GetContainer() const
{
  ASSERT(m_container != nullptr, ());
  return m_container;
}

ms::LatLon UserMark::GetLatLon() const
{
  return MercatorBounds::ToLatLon(m_ptOrg);
}

StaticMarkPoint::StaticMarkPoint(UserMarkContainer * container)
  : UserMark(m2::PointD{}, container)
{}

UserMark::Type StaticMarkPoint::GetMarkType() const
{
  return UserMark::Type::STATIC;
}

void StaticMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

MyPositionMarkPoint::MyPositionMarkPoint(UserMarkContainer * container)
  : StaticMarkPoint(container)
{}

DebugMarkPoint::DebugMarkPoint(const m2::PointD & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{}

string DebugMarkPoint::GetSymbolName() const
{
  return "api-result";
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
