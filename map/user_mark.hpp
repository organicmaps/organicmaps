#pragma once

#include "../geometry/point2d.hpp"

#include "../std/string.hpp"
#include "../std/noncopyable.hpp"

class UserMarkContainer;

class UserCustomData
{
public:
  enum Type
  {
    EMPTY,
    POI,
    SEARCH,
    API,
    BOOKMARK
  };

  virtual ~UserCustomData() {}

  virtual Type GetType() const = 0;
};

class EmptyCustomData : public UserCustomData
{
public:
  virtual Type GetType() const { return EMPTY; }
};

class BaseCustomData : public UserCustomData
{
public:
  BaseCustomData(string const & name, string const & typeName, string address)
    : m_name(name)
    , m_typeName(typeName)
    , m_address(address) {}

  string const & GetName() const { return m_name; }
  string const & GetTypeName() const { return m_typeName; }
  string const & GetAddress() const { return m_address; }

private:
  string m_name;
  string m_typeName;
  string m_address;
};

class PoiCustomData : public BaseCustomData
{
  typedef BaseCustomData base_t;
public:
  PoiCustomData(string const & name, string const & typeName, string address)
   : base_t(name, typeName, address) {}

  virtual Type GetType() const { return POI; }
};

class SearchCustomData : public BaseCustomData
{
  typedef BaseCustomData base_t;
public:
  SearchCustomData(string const & name, string const & typeName, string address)
    : base_t(name, typeName, address) {}

  virtual Type GetType() const { return SEARCH; }
};

class ApiCustomData : public UserCustomData
{
  typedef UserCustomData base_t;
public:
  ApiCustomData(string const & name, string const & id)
    : m_name(name)
    , m_id(id) {}

  virtual Type GetType() const { return API; }

  string const & GetName() const { return m_name; }
  string const & GetID() const {return m_id; }

private:
  string m_name;
  string m_id;
};

class UserMark : private noncopyable
{
public:
  UserMark(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~UserMark();

  UserMarkContainer const * GetContainer() const;
  m2::PointD const & GetOrg() const;
  void GetLatLon(double & lat, double & lon) const;

  // custom data must be allocated in head
  // memory where allocated custom data will be deallocated by UserMark
  void InjectCustomData(UserCustomData * customData);
  UserCustomData const & GetCustomData() const;

protected:
  friend class BookmarkManager;
  void Activate() const;
  void Diactivate() const;
  UserCustomData & GetCustomData();

protected:
  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;
  UserCustomData * m_customData;
};
