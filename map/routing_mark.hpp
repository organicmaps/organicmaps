#pragma once

#include "map/user_mark_container.hpp"

#include <string>

enum class RouteMarkType : uint8_t
{
  Start = 0,
  Intermediate = 1,
  Finish = 2
};

struct RouteMarkData
{
  std::string m_name;
  RouteMarkType m_pointType = RouteMarkType::Start;
  int8_t m_intermediateIndex = 0;
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
  dp::GLState::DepthLayer GetDepthLayer() const override;

  std::string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override { return Type::ROUTING; }

  RouteMarkType GetRoutePointType() const { return m_markData.m_pointType; }
  void SetRoutePointType(RouteMarkType type);

  void SetIntermediateIndex(int8_t index);
  int8_t GetIntermediateIndex() const { return m_markData.m_intermediateIndex; }

  void SetIsMyPosition(bool isMyPosition);
  bool IsMyPosition() const { return m_markData.m_isMyPosition; }

  void SetPassed(bool isPassed);
  bool IsPassed() const { return m_markData.m_isPassed; }

  RouteMarkData const & GetMarkData() const { return m_markData; }
  void SetMarkData(RouteMarkData && data);

  drape_ptr<dp::TitleDecl> GetTitleDecl() const override;

  bool SymbolHasPriority() const override { return false; }
  bool TitleHasPriority() const override { return true; }

private:
  RouteMarkData m_markData;
  dp::TitleDecl m_titleDecl;
};

class RouteUserMarkContainer : public UserMarkContainer
{
public:
  RouteUserMarkContainer(double layerDepth, Framework & fm);
protected:
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};

class RoutePointsLayout
{
public:
  static int8_t const kMaxIntermediatePointsCount;

  RoutePointsLayout(UserMarksController & routeMarks);

  RouteMarkPoint * AddRoutePoint(RouteMarkData && data);
  RouteMarkPoint * GetRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  std::vector<RouteMarkPoint *> GetRoutePoints();
  size_t GetRoutePointsCount() const;
  bool RemoveRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  void RemoveIntermediateRoutePoints();
  bool MoveRoutePoint(RouteMarkType currentType, int8_t currentIntermediateIndex,
                      RouteMarkType destType, int8_t destIntermediateIndex);
  void PassRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);

private:
  using TRoutePointCallback = function<void (RouteMarkPoint * mark)>;
  void ForEachIntermediatePoint(TRoutePointCallback const & fn);

  UserMarksController & m_routeMarks;
};
