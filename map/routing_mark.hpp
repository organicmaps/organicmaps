#pragma once

#include "map/user_mark_container.hpp"

#include <string>

enum class RouteMarkType : uint8_t
{
  // Do not change the order!
  Start = 0,
  Intermediate = 1,
  Finish = 2
};

struct RouteMarkData
{
  std::string m_title;
  std::string m_subTitle;
  RouteMarkType m_pointType = RouteMarkType::Start;
  size_t m_intermediateIndex = 0;
  bool m_isVisible = true;
  bool m_isMyPosition = false;
  bool m_isPassed = false;
  m2::PointD m_position;
};

class RouteMarkPoint : public UserMark
{
public:
  RouteMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~RouteMarkPoint() {}

  bool IsVisible() const override { return m_markData.m_isVisible; }
  void SetIsVisible(bool isVisible) { m_markData.m_isVisible = isVisible; }

  dp::Anchor GetAnchor() const override;
  df::RenderState::DepthLayer GetDepthLayer() const override;

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  UserMark::Type GetMarkType() const override { return Type::ROUTING; }
  bool IsAvailableForSearch() const override { return !IsPassed(); }

  RouteMarkType GetRoutePointType() const { return m_markData.m_pointType; }
  void SetRoutePointType(RouteMarkType type);

  void SetIntermediateIndex(size_t index);
  size_t GetIntermediateIndex() const { return m_markData.m_intermediateIndex; }

  void SetRoutePointFullType(RouteMarkType type, size_t intermediateIndex);
  bool IsEqualFullType(RouteMarkType type, size_t intermediateIndex) const;

  void SetIsMyPosition(bool isMyPosition);
  bool IsMyPosition() const { return m_markData.m_isMyPosition; }

  void SetPassed(bool isPassed);
  bool IsPassed() const { return m_markData.m_isPassed; }

  RouteMarkData const & GetMarkData() const { return m_markData; }
  void SetMarkData(RouteMarkData && data);

  drape_ptr<TitlesInfo> GetTitleDecl() const override;

  bool HasSymbolPriority() const override { return false; }
  bool HasTitlePriority() const override { return true; }

  void SetFollowingMode(bool enabled);

private:
  RouteMarkData m_markData;
  dp::TitleDecl m_titleDecl;
  bool m_followingMode = false;
};

class RoutePointsLayout
{
public:
  static size_t const kMaxIntermediatePointsCount;

  RoutePointsLayout(UserMarksController & routeMarks);

  RouteMarkPoint * AddRoutePoint(RouteMarkData && data);
  RouteMarkPoint * GetRoutePoint(RouteMarkType type, size_t intermediateIndex = 0);
  RouteMarkPoint * GetMyPositionPoint();
  std::vector<RouteMarkPoint *> GetRoutePoints();
  size_t GetRoutePointsCount() const;
  bool RemoveRoutePoint(RouteMarkType type, size_t intermediateIndex = 0);
  void RemoveRoutePoints();
  void RemoveIntermediateRoutePoints();
  bool MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                      RouteMarkType destType, size_t destIntermediateIndex);
  void PassRoutePoint(RouteMarkType type, size_t intermediateIndex = 0);
  void SetFollowingMode(bool enabled);
  void NotifyChanges();

private:
  using TRoutePointCallback = function<void (RouteMarkPoint * mark)>;
  void ForEachIntermediatePoint(TRoutePointCallback const & fn);
  RouteMarkPoint * GetRouteMarkForEdit(size_t index);
  RouteMarkPoint const * GetRouteMark(size_t index);

  UserMarksController & m_routeMarks;
};

class TransitMark : public UserMark
{
public:
  TransitMark(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~TransitMark() {}

  dp::Anchor GetAnchor() const override { return dp::Center; }
  df::RenderState::DepthLayer GetDepthLayer() const override { return df::RenderState::TransitMarkLayer; }
  UserMark::Type GetMarkType() const override { return Type::TRANSIT; }

  bool HasSymbolPriority() const override { return !m_symbolNames.empty() || !m_coloredSymbols.empty(); }
  bool HasTitlePriority() const override { return true; }

  void SetFeatureId(FeatureID featureId);
  FeatureID GetFeatureID() const override { return m_featureId; }

  void SetPriority(Priority priority);
  uint16_t GetPriority() const override { return static_cast<uint16_t>(m_priority); }

  void SetMinZoom(int minZoom);
  int GetMinZoom() const override { return m_minZoom; }

  void SetSymbolSizes(SymbolSizesZoomInfo const & symbolSizes);
  drape_ptr<SymbolSizesZoomInfo> GetSymbolSizes() const override;

  void SetColoredSymbols(ColoredSymbolZoomInfo const & symbolParams);
  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override;

  void SetSymbolNames(SymbolNameZoomInfo const & symbolNames);
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;

  void AddTitle(dp::TitleDecl const & titleDecl);
  drape_ptr<TitlesInfo> GetTitleDecl() const override;

  static void GetDefaultTransitTitle(dp::TitleDecl & titleDecl);

private:
  int m_minZoom = 1;
  Priority m_priority;
  FeatureID m_featureId;
  TitlesInfo m_titles;
  SymbolSizesZoomInfo m_symbolSizes;
  SymbolNameZoomInfo m_symbolNames;
  ColoredSymbolZoomInfo m_coloredSymbols;
};