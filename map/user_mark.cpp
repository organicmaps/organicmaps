#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

UserMark::UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg), m_container(container)
{
}

m2::PointD const & UserMark::GetPivot() const
{
  return m_ptOrg;
}

m2::PointD const & UserMark::GetPixelOffset() const
{
  static m2::PointD const s_centre(0.0, 0.0);
  return s_centre;
}

dp::Anchor UserMark::GetAnchor() const
{
  return dp::Center;
}

float UserMark::GetDepth() const
{
  return GetContainer()->GetPointDepth();
}

bool UserMark::RunCreationAnim() const
{
  return false;
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

SearchMarkPoint::SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
: UserMark(ptOrg, container)
{
}

string SearchMarkPoint::GetSymbolName() const
{
  return "search-result";
}

UserMark::Type SearchMarkPoint::GetMarkType() const
{
  return UserMark::Type::SEARCH;
}

PoiMarkPoint::PoiMarkPoint(UserMarkContainer * container)
  : SearchMarkPoint(m2::PointD::Zero(), container) {}

UserMark::Type PoiMarkPoint::GetMarkType() const
{
  return UserMark::Type::POI;
}

void PoiMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  m_ptOrg = ptOrg;
}

MyPositionMarkPoint::MyPositionMarkPoint(UserMarkContainer * container)
  : PoiMarkPoint(container)
{
}

UserMark::Type MyPositionMarkPoint::GetMarkType() const
{
  return UserMark::Type::MY_POSITION;
}

DebugMarkPoint::DebugMarkPoint(const m2::PointD & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
}

string DebugMarkPoint::GetSymbolName() const
{
  return "api-result";
}
