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
  static uint16_t constexpr kDefaultUserMarkProirity = 0xFFFF;

  enum class Type
  {
    API,
    SEARCH,
    POI,
    BOOKMARK,
    MY_POSITION,
    ROUTING,
    LOCAL_ADS,
    DEBUG_MARK
  };

  UserMark(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~UserMark() {}

  // df::UserPointMark overrides.
  bool IsDirty() const override { return m_isDirty; }
  void AcceptChanges() const override { m_isDirty = false; }
  bool IsVisible() const override { return true; }
  m2::PointD const & GetPivot() const override;
  m2::PointD GetPixelOffset() const override;
  dp::Anchor GetAnchor() const override;
  float GetDepth() const override;
  df::RenderState::DepthLayer GetDepthLayer() const override;
  drape_ptr<dp::TitleDecl> GetTitleDecl() const override { return nullptr; }
  uint16_t GetPriority() const override { return kDefaultUserMarkProirity; }
  bool HasSymbolPriority() const override { return false; }
  bool HasTitlePriority() const override { return false; }
  int GetMinZoom() const override { return 1; }
  FeatureID GetFeatureID() const override { return FeatureID(); }
  bool HasCreationAnimation() const override { return false; }

  UserMarkContainer const * GetContainer() const;
  ms::LatLon GetLatLon() const;
  virtual Type GetMarkType() const = 0;
  virtual bool IsAvailableForSearch() const { return true; }

protected:
  void SetDirty() { m_isDirty = true; }

  m2::PointD m_ptOrg;
  mutable UserMarkContainer * m_container;

private:
  mutable bool m_isDirty = true;
};

enum SearchMarkType
{
  DefaultSearchMark = 0,
  BookingSearchMark,
  LocalAdsSearchMark,

  SearchMarkTypesCount
};

class SearchMarkPoint : public UserMark
{
public:
  SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;

  FeatureID const & GetFoundFeature() const { return m_foundFeatureID; }
  void SetFoundFeature(FeatureID const & feature);

  string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(string const & name);

  string const & GetCustomSymbol() const { return m_customSymbol; }
  void SetCustomSymbol(string const & symbol);

protected:
  FeatureID m_foundFeatureID;
  // Used to pass exact search result matched string into a place page.
  string m_matchedName;
  string m_customSymbol;
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

  void SetUserPosition(m2::PointD const & pt, bool hasPosition)
  {
    SetPtOrg(pt);
    m_hasPosition = hasPosition;
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
