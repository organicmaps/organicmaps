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

m2::PointD UserMark::GetPixelOffset() const
{
  return m2::PointD(0.0, 0.0);
}

dp::Anchor UserMark::GetAnchor() const
{
  return dp::Center;
}

float UserMark::GetDepth() const
{
  return GetContainer()->GetPointDepth();
}

bool UserMark::HasCreationAnimation() const
{
  return false;
}

UserMarkContainer const * UserMark::GetContainer() const
{
  ASSERT(m_container != nullptr, ());
  return m_container;
}

void UserMark::SetDirty()
{
  m_isDirty = true;
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
  return m_customSymbol.empty() ? "search-result" : m_customSymbol;
}

UserMark::Type SearchMarkPoint::GetMarkType() const
{
  return UserMark::Type::SEARCH;
}

void SearchMarkPoint::SetFoundFeature(FeatureID const & feature)
{
  SetDirty();
  m_foundFeatureID = feature;
}

void SearchMarkPoint::SetMatchedName(string const & name)
{
  SetDirty();
  m_matchedName = name;
}

void SearchMarkPoint::SetCustomSymbol(string const & symbol)
{
  SetDirty();
  m_customSymbol = symbol;
}

PoiMarkPoint::PoiMarkPoint(UserMarkContainer * container)
  : SearchMarkPoint(m2::PointD::Zero(), container) {}

UserMark::Type PoiMarkPoint::GetMarkType() const
{
  return UserMark::Type::POI;
}

void PoiMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  SetDirty();
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

string DebugPrint(UserMark::Type type)
{
  switch (type)
  {
  case UserMark::Type::API: return "API";
  case UserMark::Type::SEARCH: return "SEARCH";
  case UserMark::Type::POI: return "POI";
  case UserMark::Type::BOOKMARK: return "BOOKMARK";
  case UserMark::Type::MY_POSITION: return "MY_POSITION";
  case UserMark::Type::DEBUG_MARK: return "DEBUG_MARK";
  case UserMark::Type::ROUTING: return "ROUTING";
  }
}
