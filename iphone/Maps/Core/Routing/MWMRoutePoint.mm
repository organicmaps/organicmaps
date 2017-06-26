#import "MWMRoutePoint.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "MWMRoutePoint+CPP.h"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

@interface MWMRoutePoint ()

@property(nonatomic, readonly) m2::PointD point;

@end

@implementation MWMRoutePoint

- (instancetype)initWithLastLocationAndType:(MWMRoutePointType)type
{
  auto lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return nil;

  self = [super init];
  if (self)
  {
    _point = lastLocation.mercator;
    _name = L(@"p2p_your_location");
    _isMyPosition = YES;
    _type = type;
  }
  return self;
}

- (instancetype)initWithURLSchemeRoutePoint:(url_scheme::RoutePoint const &)point
                                       type:(MWMRoutePointType)type
{
  self = [super init];
  if (self)
  {
    _point = point.m_org;
    _name = @(point.m_name.c_str());
    _isMyPosition = NO;
    _type = type;
  }
  return self;
}

- (instancetype)initWithRouteMarkData:(RouteMarkData const &)point
{
  self = [super init];
  if (self)
  {
    _point = point.m_position;
    _name = @(point.m_name.c_str());
    _isMyPosition = point.m_isMyPosition;
    switch (point.m_pointType)
    {
    case RouteMarkType::Start: _type = MWMRoutePointTypeStart; break;
    case RouteMarkType::Intermediate: _type = MWMRoutePointTypeIntermediate; break;
    case RouteMarkType::Finish: _type = MWMRoutePointTypeFinish; break;
    }
  }
  return self;
}

- (instancetype)initWithPoint:(m2::PointD const &)point type:(MWMRoutePointType)type
{
  self = [super init];
  if (self)
  {
    _point = point;
    switch (type)
    {
    case MWMRoutePointTypeStart: _name = @"Source"; break;
    case MWMRoutePointTypeIntermediate: _name = @"Intermediate"; break;
    case MWMRoutePointTypeFinish: _name = @"Destination"; break;
    }
    _isMyPosition = NO;
    _type = type;
  }
  return self;
}

- (double)latitude { return MercatorBounds::YToLat(self.point.y); }
- (double)longitude { return MercatorBounds::XToLon(self.point.x); }
- (RouteMarkData)routeMarkData
{
  RouteMarkData pt;
  switch (self.type)
  {
  case MWMRoutePointTypeStart: pt.m_pointType = RouteMarkType::Start; break;
  case MWMRoutePointTypeIntermediate: pt.m_pointType = RouteMarkType::Intermediate; break;
  case MWMRoutePointTypeFinish: pt.m_pointType = RouteMarkType::Finish; break;
  }
  pt.m_position = self.point;
  pt.m_isMyPosition = static_cast<bool>(self.isMyPosition);
  return pt;
}

@end
