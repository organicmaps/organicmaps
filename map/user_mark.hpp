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

class UserMark : public df::UserPointMark
{
public:
  static uint16_t constexpr kDefaultUserMarkPriority = 0xFFFF;

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
  uint16_t GetPriority() const override { return kDefaultUserMarkPriority; }
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

  DISALLOW_COPY_AND_MOVE(UserMark);
};

class PoiMarkPoint : public UserMark
{
public:
  explicit PoiMarkPoint(UserMarkContainer * container);

  string GetSymbolName() const override { return {}; }
  UserMark::Type GetMarkType() const override;

  void SetPtOrg(m2::PointD const & ptOrg);
};

class MyPositionMarkPoint : public PoiMarkPoint
{
public:
  explicit MyPositionMarkPoint(UserMarkContainer * container);

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
