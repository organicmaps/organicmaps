#include "user_mark.hpp"
#include "user_mark_container.hpp"


UserMark::UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg), m_container(container)
{
}

m2::PointD const & UserMark::GetPivot() const
{
  return m_ptOrg;
}

dp::Anchor UserMark::GetAnchor() const
{
  return dp::Center;
}

float UserMark::GetDepth() const
{
  return GetContainer()->GetPointDepth();
}

UserMarkContainer const * UserMark::GetContainer() const
{
  ASSERT(m_container != nullptr, ());
  return m_container;
}

void UserMark::GetLatLon(double & lat, double & lon) const
{
  lon = MercatorBounds::XToLon(m_ptOrg.x);
  lat = MercatorBounds::YToLat(m_ptOrg.y);
}

void UserMark::FillLogEvent(UserMark::TEventContainer & details) const
{
  double lat, lon;
  GetLatLon(lat, lon);
  details.emplace("lat", strings::to_string(lat));
  details.emplace("lon", strings::to_string(lon));
}

////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////

ApiMarkPoint::ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
}

ApiMarkPoint::ApiMarkPoint(string const & name, string const & id, m2::PointD const & ptOrg,
                           UserMarkContainer * container)
  : UserMark(ptOrg, container)
  , m_name(name)
  , m_id(id)
{
}

string ApiMarkPoint::GetSymbolName() const
{
  return "api-result";
}

UserMark::Type ApiMarkPoint::GetMarkType() const
{
  return UserMark::Type::API;
}

string const & ApiMarkPoint::GetName() const
{
  return m_name;
}

void ApiMarkPoint::SetName(string const & name)
{
  m_name = name;
}

string const & ApiMarkPoint::GetID() const
{
  return m_id;
}

void ApiMarkPoint::SetID(string const & id)
{
  m_id = id;
}

unique_ptr<UserMarkCopy> ApiMarkPoint::Copy() const
{
  return unique_ptr<UserMarkCopy>(
        new UserMarkCopy(new ApiMarkPoint(m_name, m_id, m_ptOrg, m_container)));
}

void ApiMarkPoint::FillLogEvent(UserMark::TEventContainer & details) const
{
  UserMark::FillLogEvent(details);
  details.emplace("markType", "API");
  details.emplace("name", GetName());
}

/////////////////////////////////////////////////////////////////

SearchMarkPoint::SearchMarkPoint(search::AddressInfo const & info, m2::PointD const & ptOrg,
                                 UserMarkContainer * container)
  : UserMark(ptOrg, container)
  , m_info(info)
{
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

search::AddressInfo const & SearchMarkPoint::GetInfo() const
{
  return m_info;
}

void SearchMarkPoint::SetInfo(search::AddressInfo const & info)
{
  m_info = info;
}

feature::FeatureMetadata const & SearchMarkPoint::GetMetadata() const
{
  return m_metadata;
}

void SearchMarkPoint::SetMetadata(const feature::FeatureMetadata & metadata)
{
  m_metadata = metadata;
}

unique_ptr<UserMarkCopy> SearchMarkPoint::Copy() const
{
  return unique_ptr<UserMarkCopy>(
        new UserMarkCopy(new SearchMarkPoint(m_info, m_ptOrg, m_container)));
}

void SearchMarkPoint::FillLogEvent(UserMark::TEventContainer & details) const
{
  UserMark::FillLogEvent(details);
  details.emplace("markType", "SEARCH");
  details.emplace("name", m_info.GetPinName());
  details.emplace("type", m_info.GetPinType());
  details.emplace("metaData", m_metadata.Empty() ? "0" : "1");
}

/////////////////////////////////////////////////////////////////

PoiMarkPoint::PoiMarkPoint(UserMarkContainer * container)
  : SearchMarkPoint(m2::PointD(0.0, 0.0), container) {}

UserMark::Type PoiMarkPoint::GetMarkType() const
{
  return UserMark::Type::POI;
}

unique_ptr<UserMarkCopy> PoiMarkPoint::Copy() const
{
  return unique_ptr<UserMarkCopy>(new UserMarkCopy(this, false));
}

void PoiMarkPoint::FillLogEvent(UserMark::TEventContainer & details) const
{
  SearchMarkPoint::FillLogEvent(details);
  details.emplace("markType", "POI");
}

void PoiMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  m_ptOrg = ptOrg;
}

void PoiMarkPoint::SetName(string const & name)
{
  m_info.m_name = name;
}

/////////////////////////////////////////////////////////////////

MyPositionMarkPoint::MyPositionMarkPoint(UserMarkContainer * container)
  : PoiMarkPoint(container)
{
}

UserMark::Type MyPositionMarkPoint::GetMarkType() const
{
  return UserMark::Type::MY_POSITION;
}

void MyPositionMarkPoint::FillLogEvent(UserMark::TEventContainer & details) const
{
  PoiMarkPoint::FillLogEvent(details);
  details.emplace("markType", "MY_POSITION");
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

void DebugMarkPoint::FillLogEvent(UserMark::TEventContainer & details) const
{
  UserMark::FillLogEvent(details);
  details.emplace("markType", "DEBUG");
}
