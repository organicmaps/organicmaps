#pragma once

#include "../geometry/point2d.hpp"

#include "../search/result.hpp"

#include "../std/string.hpp"
#include "../std/noncopyable.hpp"

class UserMarkContainer;
class PaintOverlayEvent;
class UserMarkDLCache;

namespace graphics
{
  class DisplayList;
}

class UserMark : private noncopyable
{
public:
  enum Type
  {
    API,
    SEARCH,
    POI,
    BOOKMARK
  };

  UserMark(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~UserMark();

  UserMarkContainer const * GetContainer() const;
  m2::PointD const & GetOrg() const;
  void GetLatLon(double & lat, double & lon) const;
  virtual bool IsCustomDrawable() const { return false;}
  virtual Type GetMarkType() const = 0;

protected:
  friend class BookmarkManager;
  void Activate() const;

protected:
  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;
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

  UserMark::Type GetMarkType() const { return API; }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const   { return m_id; }
  void SetID(string const & id)  { m_id = id; }

private:
  string m_name;
  string m_id;
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

  UserMark::Type GetMarkType() const { return SEARCH; }

  search::AddressInfo const & GetInfo() const { return m_info; }
  void SetInfo(search::AddressInfo const & info) { m_info = info; }

private:
  search::AddressInfo m_info;
};

class PoiMarkPoint : public SearchMarkPoint
{
public:
  PoiMarkPoint(UserMarkContainer * container)
    : SearchMarkPoint(m2::PointD(0.0, 0.0), container) {}

  UserMark::Type GetMarkType() const { return POI; }

  void SetPtOrg(m2::PointD const & ptOrg) { m_ptOrg = ptOrg; }
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
