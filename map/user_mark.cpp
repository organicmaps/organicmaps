#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "indexer/classificator.hpp"

namespace
{

string ToString(UserMark::Type t)
{
  switch (t)
  {
  case UserMark::Type::BOOKMARK: return "BOOKMARK";
  case UserMark::Type::API: return "API";
  case UserMark::Type::MY_POSITION: return "MY_POSITION";
  case UserMark::Type::POI: return "POI";
  case UserMark::Type::SEARCH: return "SEARCH";
  case UserMark::Type::DEBUG_MARK: return "DEBUG";
  }
}

}

UserMark::UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg), m_container(container)
{
}

FeatureType * UserMark::GetFeature() const
{
  return m_feature.get();
}

void UserMark::SetFeature(unique_ptr<FeatureType> feature)
{
  m_feature = move(feature);
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

void UserMark::FillLogEvent(UserMark::TEventContainer & details) const
{
  ms::LatLon const ll = GetLatLon();
  details.emplace("lat", strings::to_string(ll.lat));
  details.emplace("lon", strings::to_string(ll.lon));
  details.emplace("markType", ToString(GetMarkType()));

  if (m_feature)
  {
    string name;
    m_feature->GetReadableName(name);
    details.emplace("name", move(name));
    string types;
    m_feature->ForEachType([&types](uint32_t type)
    {
      if (!types.empty())
        types += ',';
      types += classif().GetReadableObjectName(type);
    });
    // Older version of statistics used "type" key with AddressInfo::GetPinType() value.
    details.emplace("types", move(types));
    details.emplace("metaData", m_feature->GetMetadata().Empty() ? "0" : "1");
  }
}

UserMarkCopy::UserMarkCopy(UserMark const * srcMark, bool needDestroy)
  : m_srcMark(srcMark)
  , m_needDestroy(needDestroy)
{
}

UserMarkCopy::~UserMarkCopy()
{
  if (m_needDestroy)
    delete m_srcMark;
}

UserMark const * UserMarkCopy::GetUserMark() const
{
  return m_srcMark;
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

unique_ptr<UserMarkCopy> SearchMarkPoint::Copy() const
{
  // TODO(AlexZ): Remove this code after UserMark refactoring.
  UserMark * mark = new SearchMarkPoint(m_ptOrg, m_container);
  if (m_feature)
    mark->SetFeature(unique_ptr<FeatureType>(new FeatureType(*m_feature)));
  return unique_ptr<UserMarkCopy>(new UserMarkCopy(mark));
}

PoiMarkPoint::PoiMarkPoint(UserMarkContainer * container)
  : SearchMarkPoint(m2::PointD::Zero(), container) {}

UserMark::Type PoiMarkPoint::GetMarkType() const
{
  return UserMark::Type::POI;
}

unique_ptr<UserMarkCopy> PoiMarkPoint::Copy() const
{
  return unique_ptr<UserMarkCopy>(new UserMarkCopy(this, false));
}
void PoiMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  m_ptOrg = ptOrg;
}

void PoiMarkPoint::SetCustomName(string const & customName)
{
  m_customName = customName;
}

string const & PoiMarkPoint::GetCustomName() const
{
  return m_customName;
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

unique_ptr<UserMarkCopy> DebugMarkPoint::Copy() const
{
  return unique_ptr<UserMarkCopy>(new UserMarkCopy(new DebugMarkPoint(m_ptOrg, m_container)));
}
