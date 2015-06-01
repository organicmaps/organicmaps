#pragma once

#include "geometry/point2d.hpp"

#include "indexer/feature.hpp"

#include "search/result.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

class UserMarkContainer;
class PaintOverlayEvent;
class UserMarkDLCache;

namespace graphics
{
  class DisplayList;
}

class UserMarkCopy;

class UserMark
{
  UserMark(UserMark const &) = delete;
  UserMark& operator=(UserMark const &) = delete;

public:
  enum class Type
  {
    API,
    SEARCH,
    POI,
    BOOKMARK,
    MY_POSITION,
    DEBUG_MARK
  };

  UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
    : m_ptOrg(ptOrg), m_container(container)
  {
  }

  virtual ~UserMark() {}

  UserMarkContainer const * GetContainer() const { return m_container; }
  m2::PointD const & GetOrg() const { return m_ptOrg; }
  void GetLatLon(double & lat, double & lon) const
  {
    lon = MercatorBounds::XToLon(m_ptOrg.x);
    lat = MercatorBounds::YToLat(m_ptOrg.y);
  }
  virtual bool IsCustomDrawable() const { return false;}
  virtual Type GetMarkType() const = 0;
  virtual unique_ptr<UserMarkCopy> Copy() const = 0;
  // Need it to calculate POI rank from all taps to features via statistics.
  typedef map<string, string> TEventContainer;
  virtual void FillLogEvent(TEventContainer & details) const
  {
    double lat, lon;
    GetLatLon(lat, lon);
    details["lat"] = strings::to_string(lat);
    details["lon"] = strings::to_string(lon);
  }

protected:
  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;
};

class UserMarkCopy
{
public:
  UserMarkCopy(UserMark const * srcMark, bool needDestroy = true)
    : m_srcMark(srcMark)
    , m_needDestroy(needDestroy)
  {
  }

  ~UserMarkCopy()
  {
    if (m_needDestroy)
      delete m_srcMark;
  }

  UserMark const * GetUserMark() const { return m_srcMark; }

private:
  UserMark const * m_srcMark;
  bool m_needDestroy;
};

class ApiMarkPoint : public UserMark
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : UserMark(ptOrg, container)
  {
  }

  ApiMarkPoint(string const & name,
           string const & id,
           m2::PointD const & ptOrg,
           UserMarkContainer * container)
    : UserMark(ptOrg, container)
    , m_name(name)
    , m_id(id)
  {
  }

  UserMark::Type GetMarkType() const override { return UserMark::Type::API; }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const   { return m_id; }
  void SetID(string const & id)  { m_id = id; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(
        new UserMarkCopy(new ApiMarkPoint(m_name, m_id, m_ptOrg, m_container)));
  }

  virtual void FillLogEvent(TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details["markType"] = "API";
    details["name"] = GetName();
  }

private:
  string m_name;
  string m_id;
};

class DebugMarkPoint : public UserMark
{
public:
  DebugMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : UserMark(ptOrg, container)
  {
  }

  UserMark::Type GetMarkType() const override { return UserMark::Type::DEBUG_MARK; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(new UserMarkCopy(new DebugMarkPoint(m_ptOrg, m_container)));
  }

  virtual void FillLogEvent(TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details["markType"] = "DEBUG";
  }
};

class SearchMarkPoint : public UserMark
{
public:
  SearchMarkPoint(search::AddressInfo const & info,
           m2::PointD const & ptOrg,
           UserMarkContainer * container)
    : UserMark(ptOrg, container)
    , m_info(info)
  {
  }

  SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : UserMark(ptOrg, container)
  {
  }

  UserMark::Type GetMarkType() const override { return UserMark::Type::SEARCH; }

  search::AddressInfo const & GetInfo() const { return m_info; }
  void SetInfo(search::AddressInfo const & info) { m_info = info; }

  feature::FeatureMetadata const & GetMetadata() const { return m_metadata; }
  void SetMetadata(feature::FeatureMetadata const & metadata) { m_metadata = metadata; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(
        new UserMarkCopy(new SearchMarkPoint(m_info, m_ptOrg, m_container)));
  }

  virtual void FillLogEvent(TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details["markType"] = "SEARCH";
    details["name"] = m_info.GetPinName();
    details["type"] = m_info.GetPinType();
    details["metaData"] = m_metadata.Empty() ? "0" : "1";
  }

protected:
  search::AddressInfo m_info;
  feature::FeatureMetadata m_metadata;
};

class PoiMarkPoint : public SearchMarkPoint
{
public:
  PoiMarkPoint(UserMarkContainer * container)
    : SearchMarkPoint(m2::PointD(0.0, 0.0), container) {}

  UserMark::Type GetMarkType() const override { return UserMark::Type::POI; }
  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(new UserMarkCopy(this, false));
  }
  virtual void FillLogEvent(TEventContainer & details) const override
  {
    SearchMarkPoint::FillLogEvent(details);
    details["markType"] = "POI";
  }

  void SetPtOrg(m2::PointD const & ptOrg) { m_ptOrg = ptOrg; }
  void SetName(string const & name) { m_info.m_name = name; }
};

class MyPositionMarkPoint : public PoiMarkPoint
{
public:
  MyPositionMarkPoint(UserMarkContainer * container) : PoiMarkPoint(container) {}

  UserMark::Type GetMarkType() const override { return UserMark::Type::MY_POSITION; }
  virtual void FillLogEvent(TEventContainer & details) const override
  {
    PoiMarkPoint::FillLogEvent(details);
    details["markType"] = "MY_POSITION";
  }
};

class ICustomDrawable : public UserMark
{
public:
  ICustomDrawable(m2::PointD const & ptOrg, UserMarkContainer * container) : UserMark(ptOrg, container) {}
  bool IsCustomDrawable() const { return true; }
  virtual graphics::DisplayList * GetDisplayList(UserMarkDLCache * cache) const = 0;
  virtual double GetAnimScaleFactor() const = 0;
  virtual m2::PointD const & GetPixelOffset() const = 0;
};
