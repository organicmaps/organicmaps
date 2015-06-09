#pragma once

#include "search/result.hpp"

#include "drape_frontend/user_marks_provider.hpp"

#include "indexer/feature.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"


class UserMarkContainer;
class UserMarkCopy;

class UserMark : public df::UserPointMark
{
  DISALLOW_COPY_AND_MOVE(UserMark)
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

  UserMark(m2::PointD const & ptOrg, UserMarkContainer * container);

  virtual ~UserMark() {}

  ///////////////////////////////////////////////////////
  /// df::UserPointMark
  m2::PointD const & GetPivot() const override;
  dp::Anchor GetAnchor() const override;
  float GetDepth() const override;
  bool RunCreationAnim() const override;
  ///////////////////////////////////////////////////////

  UserMarkContainer const * GetContainer() const;
  void GetLatLon(double & lat, double & lon) const;
  virtual Type GetMarkType() const = 0;
  virtual unique_ptr<UserMarkCopy> Copy() const = 0;
  // Need it to calculate POI rank from all taps to features via statistics.
  using TEventContainer = map<string, string>;
  virtual void FillLogEvent(TEventContainer & details) const;

protected:
  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;
};

class UserMarkCopy
{
public:
  UserMarkCopy(UserMark const * srcMark, bool needDestroy = true);
  ~UserMarkCopy();

  UserMark const * GetUserMark() const;

private:
  UserMark const * m_srcMark;
  bool m_needDestroy;
};

class ApiMarkPoint : public UserMark
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  ApiMarkPoint(string const & name,
           string const & id,
           m2::PointD const & ptOrg,
           UserMarkContainer * container);

  string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;

  string const & GetName() const;
  void SetName(string const & name);

  string const & GetID() const;
  void SetID(string const & id);

  unique_ptr<UserMarkCopy> Copy() const override;

  virtual void FillLogEvent(TEventContainer & details) const override;

private:
  string m_name;
  string m_id;
};

class SearchMarkPoint : public UserMark
{
public:
  SearchMarkPoint(search::AddressInfo const & info,
           m2::PointD const & ptOrg,
           UserMarkContainer * container);

  SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;

  search::AddressInfo const & GetInfo() const;
  void SetInfo(search::AddressInfo const & info);

  feature::Metadata const & GetMetadata() const;
  void SetMetadata(feature::Metadata && metadata);

  unique_ptr<UserMarkCopy> Copy() const override;

  virtual void FillLogEvent(TEventContainer & details) const override;

protected:
  search::AddressInfo m_info;
  feature::Metadata m_metadata;
};

class PoiMarkPoint : public SearchMarkPoint
{
public:
  PoiMarkPoint(UserMarkContainer * container);
  UserMark::Type GetMarkType() const override;
  unique_ptr<UserMarkCopy> Copy() const override;

  void SetPtOrg(m2::PointD const & ptOrg);
  void SetName(string const & name);
};

class MyPositionMarkPoint : public PoiMarkPoint
{
public:
  MyPositionMarkPoint(UserMarkContainer * container);

  UserMark::Type GetMarkType() const override;
};

class DebugMarkPoint : public UserMark
{
public:
  DebugMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;

  Type GetMarkType() const override { return UserMark::Type::DEBUG_MARK; }
  unique_ptr<UserMarkCopy> Copy() const override;
};
