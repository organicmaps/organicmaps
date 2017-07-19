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
    _title = L(@"p2p_your_location");
    _subtitle = L(@"");
    _isMyPosition = YES;
    _type = type;
    _intermediateIndex = 0;
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
    _title = @(point.m_name.c_str());
    _subtitle = L(@"");
    _isMyPosition = NO;
    _type = type;
    _intermediateIndex = 0;
  }
  return self;
}

- (instancetype)initWithRouteMarkData:(RouteMarkData const &)point
{
  self = [super init];
  if (self)
  {
    _point = point.m_position;
    _title = @(point.m_title.c_str());
    _subtitle = @(point.m_subTitle.c_str());
    _isMyPosition = point.m_isMyPosition;
    _intermediateIndex = point.m_intermediateIndex;
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
    case MWMRoutePointTypeStart: _title = @"Source"; break;
    case MWMRoutePointTypeIntermediate: _title = @"Intermediate"; break;
    case MWMRoutePointTypeFinish: _title = @"Destination"; break;
    }
    _subtitle = L(@"");
    _isMyPosition = NO;
    _type = type;
    _intermediateIndex = 0;
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
  pt.m_title = self.title.UTF8String;
  pt.m_subTitle = self.subtitle.UTF8String;
  pt.m_intermediateIndex = self.intermediateIndex;
  return pt;
}

@end
