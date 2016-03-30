#pragma once

#include "drape_frontend/user_marks_provider.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"


class UserMarkContainer;
class UserMarkCopy;

class UserMark : public df::UserPointMark
{
  DISALLOW_COPY_AND_MOVE(UserMark);
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
  m2::PointD const & GetPixelOffset() const override;
  dp::Anchor GetAnchor() const override;
  float GetDepth() const override;
  bool RunCreationAnim() const override;
  ///////////////////////////////////////////////////////

  UserMarkContainer const * GetContainer() const;
  ms::LatLon GetLatLon() const;
  virtual Type GetMarkType() const = 0;
  // Need it to calculate POI rank from all taps to features via statistics.
  using TEventContainer = map<string, string>;

protected:
  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;
};

class SearchMarkPoint : public UserMark
{
public:
  SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;
  // TODO: Do not use usermarks to store any significant information for UI/core.
  // Refactor them out to only display some layers on a map.
  FeatureID m_foundFeatureID;
  // Used to pass exact search result matched string into a place page.
  string m_matchedName;
};

class PoiMarkPoint : public SearchMarkPoint
{
public:
  PoiMarkPoint(UserMarkContainer * container);
  UserMark::Type GetMarkType() const override;

  void SetPtOrg(m2::PointD const & ptOrg);
};

class MyPositionMarkPoint : public PoiMarkPoint
{
public:
  MyPositionMarkPoint(UserMarkContainer * container);

  UserMark::Type GetMarkType() const override;

  void SetUserPosition(m2::PointD const & pt)
  {
    SetPtOrg(pt);
    m_hasPosition = true;
  }
  bool HasPosition() const { return m_hasPosition; }

private:
  bool m_hasPosition = false;
};

class DebugMarkPoint : public UserMark
{
public:
  DebugMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;

  Type GetMarkType() const override { return UserMark::Type::DEBUG_MARK; }
};

string DebugPrint(UserMark::Type type);
