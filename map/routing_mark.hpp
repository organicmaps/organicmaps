#pragma once

#include "map/user_mark_container.hpp"

#include <string>

enum class RouteMarkType : uint8_t
{
  Start = 0,
  Intermediate = 1,
  Finish = 2
};

class RouteMarkPoint : public UserMark
{
public:
  RouteMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~RouteMarkPoint() {}

  bool IsVisible() const override;
  void SetIsVisible(bool isVisible);

  dp::Anchor GetAnchor() const override;

  std::string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override { return Type::ROUTING; }

  RouteMarkType GetRoutePointType() const { return m_pointType; }
  void SetRoutePointType(RouteMarkType type) { m_pointType = type; }

  void SetIntermediateIndex(int8_t index) { m_intermediateIndex = index; }
  int8_t GetIntermediateIndex() const { return m_intermediateIndex; }

  void SetIsMyPosition(bool isMyPosition);
  bool IsMyPosition() const;

private:
  RouteMarkType m_pointType;
  int8_t m_intermediateIndex = 0;
  bool m_isVisible = true;
  bool m_isMyPosition = false;
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

  RouteMarkPoint * GetRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  RouteMarkPoint * AddRoutePoint(m2::PointD const & ptOrg, RouteMarkType type, int8_t intermediateIndex = 0);
  bool RemoveRoutePoint(RouteMarkType type, int8_t intermediateIndex = 0);
  bool MoveRoutePoint(RouteMarkType currentType, int8_t currentIntermediateIndex,
                      RouteMarkType destType, int8_t destIntermediateIndex);

private:
  using TRoutePointCallback = function<void (RouteMarkPoint * mark)>;
  void ForEachIntermediatePoint(TRoutePointCallback const & fn);

  UserMarksController & m_routeMarks;
};
